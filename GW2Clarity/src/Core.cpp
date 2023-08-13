#include "Core.h"

#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>
#include <common/baseresource.h>
#include <shellapi.h>

#include "ConfigurationFile.h"
#include "Direct3D11Loader.h"
#include "GFXSettings.h"
#include "ImGuiPopup.h"
#include "Input.h"
#include "Log.h"
#include "MiscTab.h"
#include "MumbleLink.h"
#include "SettingsMenu.h"
#include "ShaderManager.h"
#include "UpdateCheck.h"
#include "Utility.h"
#include "Version.h"

KeyCombo GetSettingsKeyCombo() {
    return { GetScanCodeFromVirtualKey('P'), Modifier::Shift | Modifier::Alt };
}

namespace GW2Clarity
{

class ClarityMiscTab : public ::MiscTab
{
public:
    void AdditionalGUI() override {
    }
};

void Core::InnerInitPreImGui() {
    ClarityMiscTab::init<ClarityMiscTab>();
}

void Core::InnerInitPreFontImGui() {
    ImGui::SetColorEditOptions(ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreview);
    auto& imio = ImGui::GetIO();
    auto fontCfg = ImFontConfig();
    fontCfg.FontDataOwnedByAtlas = false;

    if(const auto data = LoadResource(dllModule_, IDR_FONT_BLACK); data.data())
        fontBuffCounter_ = imio.Fonts->AddFontFromMemoryTTF(data.data(), i32(data.size_bytes()), 128.f, &fontCfg);
}

void Core::InnerInitPostImGui() {
    firstMessageShown_ = std::make_unique<ConfigurationOption<bool>>("", "first_message_shown_v1", "Core", false);
    testImage_ = CreateTextureFromResource(device_.Get(), dllModule(), IDR_TEST_IMAGE);

    // buffs_ = std::make_unique<Buffs>(device_);
    // styles_ = std::make_unique<Styles>(device_, buffs_.get());
    // grids_ = std::make_unique<Grids>(device_, buffs_.get(), styles_.get());
    // layouts_ = std::make_unique<Layouts>(device_, grids_.get());
    cursor_ = std::make_unique<Cursor>(device_);
}

void Core::InnerInternalInit() {
    if(!buffLib_) {
        wchar_t fn[MAX_PATH];
        GetModuleFileName(dllModule(), fn, MAX_PATH);

        std::filesystem::path buffsPath = fn;
#ifdef _DEBUG
        buffsPath = buffsPath.remove_filename() / "getbuffsd.dll";
        buffLib_ = LoadLibrary(buffsPath.wstring().c_str());
        if(!buffLib_)
#endif
        {
            buffsPath = buffsPath.remove_filename() / "getbuffs.dll";
            buffLib_ = LoadLibrary(buffsPath.wstring().c_str());
        }
        if(buffLib_)
            getBuffs_ = (decltype(getBuffs_))GetProcAddress(buffLib_, "GetCurrentPlayerStackedBuffs");
        else
            getBuffs_ = nullptr;

        if(!buffLib_)
            LogError("Could not find getbuffs.dll!");
        if(!getBuffs_)
            LogError("Could not find get buffs callback!");
    }
}

void Core::InnerShutdown() {
    FreeLibrary(buffLib_);
    buffLib_ = nullptr;

    CoUninitialize();
}

void Core::InnerFrequentUpdate() {
    // if(getBuffs_)
    //     buffs_->UpdateBuffsTable(getBuffs_());
}

void Core::MockInit() {
    SettingsMenu::i().MakeVisible();
}

void Core::InnerUpdate() {
}

void Core::InnerDraw() {
    if(!firstMessageShown_->value())
        ImGuiPopup("Welcome to GW2Clarity!")
            .Position({ 0.5f, 0.45f })
            .Size({ 0.35f, 0.2f })
            .Display(
                [&](const ImVec2& windowSize) {
                    ImGui::TextWrapped(
                        "Welcome to GW2Clarity! This addon provides extensive UI customization options to make it easier to parse gameplay "
                        "and situations. "
                        "To begin, use the shortcut Shift+Alt+P to open the settings menu and take a moment to bind your keys. If you ever "
                        "need further assistance, please visit "
                        "this project's website at");

                    ImGui::Spacing();
                    ImGui::SetCursorPosX(windowSize.x * 0.1f);

                    if(ImGui::Button("https://github.com/Friendly0Fire/GW2Clarity",
                                     ImVec2(windowSize.x * 0.8f, ImGui::GetFontSize() * 1.3f)))
                        ShellExecute(0, 0, L"https://github.com/Friendly0Fire/GW2Clarity", 0, 0, SW_SHOW);
                },
                [&]() { firstMessageShown_->value(true); });

    // grids_->Draw(context_, layouts_->currentLayout(), layouts_->enableDefaultLayout());
    // layouts_->Draw(context_);
    cursor_->Draw(context_);
    // styles_->Draw(context_);
}

} // namespace GW2Clarity
