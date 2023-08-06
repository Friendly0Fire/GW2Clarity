#include <ConfigurationFile.h>
#include <Core.h>
#include <Direct3D11Loader.h>
#include <GFXSettings.h>
#include <ImGuiPopup.h>
#include <Input.h>
#include <Log.h>
#include <MiscTab.h>
#include <MumbleLink.h>
#include <SettingsMenu.h>
#include <ShaderManager.h>
#include <UpdateCheck.h>
#include <Utility.h>
#include <Version.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>
#include <common/baseresource.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <shellapi.h>

#include <AppCore/Platform.h>
#include <Ultralight/Ultralight.h>
#include <windowsx.h>

KeyCombo GetSettingsKeyCombo()
{
    return { GetScanCodeFromVirtualKey('P'), Modifier::SHIFT | Modifier::ALT };
}

namespace GW2Clarity
{

class ClarityMiscTab : public ::MiscTab
{
public:
    void AdditionalGUI() override
    {}
};

void Core::InnerInitPreImGui()
{
    ClarityMiscTab::init<ClarityMiscTab>();
}

void Core::InnerInitPreFontImGui()
{
    auto& imio                   = ImGui::GetIO();
    auto  fontCfg                = ImFontConfig();
    fontCfg.FontDataOwnedByAtlas = false;

    if (const auto data = LoadResource(dllModule_, IDR_FONT_BLACK); data.data())
        fontBuffCounter_ = imio.Fonts->AddFontFromMemoryTTF(data.data(), int(data.size_bytes()), 128.f, &fontCfg);
}

void Core::InnerInitPostImGui()
{
    firstMessageShown_ = std::make_unique<ConfigurationOption<bool>>("", "first_message_shown_v1", "Core", false);

    buffs_             = std::make_unique<Buffs>(device_);
    styles_            = std::make_unique<Styles>(device_, buffs_.get());
    grids_             = std::make_unique<Grids>(device_, buffs_.get(), styles_.get());
    layouts_           = std::make_unique<Layouts>(device_, grids_.get());
    cursor_            = std::make_unique<Cursor>(device_);

    {
        auto& sm = ShaderManager::i();
        ulcb_    = sm.MakeConstantBuffer<glm::vec4>();
        ulvs_    = sm.GetShader(L"ScreenQuad.hlsl", D3D11_SHVER_VERTEX_SHADER, "ScreenQuad", { { "SIMPLE_HLSL" } });
        ulps_    = sm.GetShader(L"Ultralight.hlsl", D3D11_SHVER_PIXEL_SHADER, "Ultralight");

        CD3D11_SAMPLER_DESC samplerDesc(D3D11_DEFAULT);
        GW2_CHECKED_HRESULT(device_->CreateSamplerState(&samplerDesc, defaultSampler_.GetAddressOf()));

        CD3D11_BLEND_DESC blendDesc(D3D11_DEFAULT);
        blendDesc.RenderTarget[0].BlendEnable    = true;
        blendDesc.RenderTarget[0].SrcBlend       = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlend      = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        GW2_CHECKED_HRESULT(device_->CreateBlendState(&blendDesc, defaultBlend_.GetAddressOf()));

        ultex_    = MakeTexture<ID3D11Texture2D>(device_, screenWidth(), screenHeight(), 1, DXGI_FORMAT_R8G8B8A8_UNORM);

        ulupdmsg_ = RegisterWindowMessage(TEXT("GW2ClarityUltralightPaint"));
    }
}

void Core::InnerInternalInit()
{
    if (!buffLib_)
    {
        wchar_t fn[MAX_PATH];
        GetModuleFileName(dllModule(), fn, MAX_PATH);

        std::filesystem::path buffsPath = fn;
#ifdef _DEBUG
        buffsPath = buffsPath.remove_filename() / "getbuffsd.dll";
        buffLib_  = LoadLibrary(buffsPath.wstring().c_str());
        if (!buffLib_)
#endif
        {
            buffsPath = buffsPath.remove_filename() / "getbuffs.dll";
            buffLib_  = LoadLibrary(buffsPath.wstring().c_str());
        }
        if (buffLib_)
            getBuffs_ = (decltype(getBuffs_))GetProcAddress(buffLib_, "GetCurrentPlayerStackedBuffs");
        else
            getBuffs_ = nullptr;

        if (!buffLib_)
            LogError("Could not find getbuffs.dll!");
        if (!getBuffs_)
            LogError("Could not find get buffs callback!");
    }
}

void Core::InnerShutdown()
{
    FreeLibrary(buffLib_);
    buffLib_ = nullptr;

    CoUninitialize();
}

void Core::InnerFrequentUpdate()
{
    if (getBuffs_)
        buffs_->UpdateBuffsTable(getBuffs_());

    PostMessage(gameWindow(), ulupdmsg_, 0, 0);
}

std::optional<LRESULT> Core::OnInput(UINT msg, WPARAM& wParam, LPARAM& lParam)
{
    using namespace ultralight;

    if (!ulview_)
    {
        HMODULE hModule;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)GetSettingsKeyCombo, &hModule);
        char path[MAX_PATH];
        GetModuleFileNameA(hModule, path, MAX_PATH);
        std::filesystem::path dir(path);
        dir = dir.parent_path() / "data";

        Config config;

        Platform::instance().set_config(config);
        Platform::instance().set_font_loader(GetPlatformFontLoader());
        Platform::instance().set_file_system(GetPlatformFileSystem(dir.string().c_str()));
        Platform::instance().set_logger(GetDefaultLogger("ultralight.log"));
        ulrenderer_ = Renderer::Create();

        ViewConfig vc;
        vc.is_accelerated = false;
        vc.is_transparent = true;

        ulview_           = ulrenderer_->CreateView(screenWidth(), screenHeight(), vc, nullptr);
        ulview_->LoadHTML("<h1>Hello World!</h1><input type='text'></input>");
    }

    if (msg == ulupdmsg_)
    {
        if (ulview_->width() != screenWidth() || ulview_->height() != screenHeight())
            ulview_->Resize(screenWidth(), screenHeight());
        ulrenderer_->Update();
        ulrenderer_->Render();

        auto* surface = static_cast<BitmapSurface*>(ulview_->surface());
        if (!surface->dirty_bounds().IsEmpty())
        {
            auto  bitmap = surface->bitmap();

            auto* pixels = static_cast<std::byte*>(bitmap->LockPixels());

            ulheight_    = bitmap->height();
            ulstride_    = bitmap->row_bytes();
            if (ulbuf_.size() != ulstride_ * ulheight_)
                ulbuf_.resize(ulstride_ * ulheight_);
            uldirty_ = true;

            ulbuf_.assign(pixels, pixels + ulstride_ * ulheight_);

            bitmap->UnlockPixels();

            surface->ClearDirtyBounds();
        }
    }

    bool down = false;
    switch (msg)
    {
        case WM_MOUSEMOVE:
        {
            MouseEvent evt;
            evt.button = MouseEvent::kButton_None;
            evt.x      = (LONG)GET_X_LPARAM(lParam);
            evt.y      = (LONG)GET_Y_LPARAM(lParam);
            evt.type   = MouseEvent::kType_MouseMoved;
            ulview_->FireMouseEvent(evt);
            break;
        }
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
            down = true;
            [[fallthrough]];
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        {
            MouseEvent evt;
            evt.x = (LONG)GET_X_LPARAM(lParam);
            evt.y = (LONG)GET_Y_LPARAM(lParam);
            if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP)
                evt.button = MouseEvent::kButton_Left;
            else if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP)
                evt.button = MouseEvent::kButton_Right;
            else
                evt.button = MouseEvent::kButton_Middle;
            evt.type = down ? MouseEvent::kType_MouseDown : MouseEvent::kType_MouseUp;
            ulview_->FireMouseEvent(evt);
            if (ulview_->HasInputFocus())
                return 0;
            break;
        }
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
        {
            ScrollEvent evt;
            evt.delta_x = evt.delta_y                           = 0;
            (msg == WM_MOUSEHWHEEL ? evt.delta_x : evt.delta_y) = GET_WHEEL_DELTA_WPARAM(wParam);
            evt.type                                            = ScrollEvent::kType_ScrollByPixel;
            ulview_->FireScrollEvent(evt);
            if (ulview_->HasInputFocus())
                return 0;
            break;
        }
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            down = true;
            [[fallthrough]];
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            KeyEvent evt(down ? KeyEvent::kType_RawKeyDown : KeyEvent::kType_KeyUp, wParam, lParam, msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP);
            ulview_->FireKeyEvent(evt);
            if (ulview_->HasInputFocus())
                return 0;
            break;
        }
        case WM_CHAR:
        {
            KeyEvent evt(KeyEvent::kType_Char, wParam, lParam, false);
            ulview_->FireKeyEvent(evt);
            if (ulview_->HasInputFocus())
                return 0;
            break;
        }
    }

    return std::nullopt;
}

void Core::InnerUpdate()
{}

void Core::DisplayDeletionMenu(DeletionInfo&& info)
{
    confirmDeletionInfo_ = std::move(info);
    ImGui::OpenPopup(confirmDeletionPopupID_);
}

void Core::PostResizeSwapChain(unsigned int w, unsigned int h)
{
    BaseCore::PostResizeSwapChain(w, h);

    ultex_ = MakeTexture<ID3D11Texture2D>(device_, w, h, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
}

void Core::InnerDraw()
{
    if (!confirmDeletionPopupID_)
        confirmDeletionPopupID_ = ImGui::GetID(ConfirmDeletionPopupName);
    if (ImGui::BeginPopupModal(ConfirmDeletionPopupName))
    {
        ImGui::TextUnformatted(
            std::format("Are you sure you want to delete {} '{}'{}?", confirmDeletionInfo_.typeName, confirmDeletionInfo_.name, confirmDeletionInfo_.tail).c_str());
        if (ImGui::Button("Yes"))
        {
            switch (confirmDeletionInfo_.id.index())
            {
                case 0:
                    cursor_->Delete(std::get<char>(confirmDeletionInfo_.id));
                    break;
                case 1:
                    layouts_->Delete(std::get<short>(confirmDeletionInfo_.id));
                    break;
                case 2:
                {
                    auto id = std::get<Id>(confirmDeletionInfo_.id);
                    layouts_->GridDeleted(id);
                    grids_->Delete(id);
                    break;
                }
                case 3:
                {
                    auto id = std::get<uint>(confirmDeletionInfo_.id);
                    grids_->StyleDeleted(id);
                    styles_->Delete(id);
                    break;
                }
            }

            confirmDeletionInfo_ = {};
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("No"))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    if (!firstMessageShown_->value())
        ImGuiPopup("Welcome to GW2Clarity!")
            .Position({ 0.5f, 0.45f })
            .Size({ 0.35f, 0.2f })
            .Display(
                [&](const ImVec2& windowSize)
                {
                    ImGui::TextWrapped(
                        "Welcome to GW2Clarity! This addon provides extensive UI customization options to make it easier to parse gameplay and situations. "
                        "To begin, use the shortcut Shift+Alt+P to open the settings menu and take a moment to bind your keys. If you ever need further assistance, please visit "
                        "this project's website at");

                    ImGui::Spacing();
                    ImGui::SetCursorPosX(windowSize.x * 0.1f);

                    if (ImGui::Button("https://github.com/Friendly0Fire/GW2Clarity", ImVec2(windowSize.x * 0.8f, ImGui::GetFontSize() * 1.3f)))
                        ShellExecute(0, 0, L"https://github.com/Friendly0Fire/GW2Clarity", 0, 0, SW_SHOW);
                },
                [&]() { firstMessageShown_->value(true); });

    grids_->Draw(context_, layouts_->currentLayout(), layouts_->enableDefaultLayout());
    layouts_->Draw(context_);
    cursor_->Draw(context_);
    styles_->Draw(context_);

    {
        if (uldirty_)
            context_->UpdateSubresource(ultex_.texture.Get(), 0, nullptr, ulbuf_.data(), ulstride_, ulstride_ * ulheight_);
        uldirty_                         = false;

        ID3D11ShaderResourceView* srvs[] = { ultex_.srv.Get() };
        context_->PSSetShaderResources(0, 1, srvs);


        ID3D11SamplerState* samplers[] = { defaultSampler_.Get() };
        context_->PSSetSamplers(0, 1, samplers);

        context_->OMSetBlendState(defaultBlend_.Get(), nullptr, 0xffffffff);
        ID3D11RenderTargetView* rtvs[] = { backBufferRTV_.Get() };
        context_->OMSetRenderTargets(1, rtvs, nullptr);

        auto& cb = *ulcb_;
        *cb      = glm::vec4(0.5f, 0.5f, 1.f, 1.f);
        cb.Update(context_.Get());

        auto& sm = ShaderManager::i();
        sm.SetConstantBuffers(context_.Get(), cb);
        sm.SetShaders(context_.Get(), ulvs_, ulps_);

        DrawScreenQuad(context_.Get());
    }
}


} // namespace GW2Clarity