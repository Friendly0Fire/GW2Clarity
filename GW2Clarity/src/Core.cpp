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

    commonCB_          = ShaderManager::i().MakeConstantBuffer<CommonConstants>();

    grids_             = std::make_unique<Grids>(device_);
    sets_              = std::make_unique<Sets>(device_, grids_.get());
    cursor_            = std::make_unique<Cursor>(device_);
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
        grids_->UpdateBuffsTable(getBuffs_());
}

void Core::InnerUpdate()
{}

void Core::DisplayDeletionMenu(DeletionInfo&& info)
{
    confirmDeletionInfo_ = std::move(info);
    ImGui::OpenPopup(confirmDeletionPopupID_);
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
                    sets_->Delete(std::get<short>(confirmDeletionInfo_.id));
                    break;
                case 2:
                    auto id = std::get<Id>(confirmDeletionInfo_.id);
                    sets_->GridDeleted(id);
                    grids_->Delete(id);
                    break;
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

    commonCB_->screenSize = glm::vec4(screenWidth(), screenHeight(), 1.f / screenWidth(), 1.f / screenHeight());
    commonCB_.Update(context_.Get());
    grids_->Draw(context_, sets_->currentSet(), sets_->enableDefaultSet());
    sets_->Draw(context_);
    cursor_->Draw(context_);
}


} // namespace GW2Clarity