#include <Core.h>
#include <Direct3D11Loader.h>
#include <imgui.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>
#include <Input.h>
#include <ConfigurationFile.h>
#include <SettingsMenu.h>
#include <Utility.h>
#include <imgui_internal.h>
#include <shellapi.h>
#include <UpdateCheck.h>
#include <ImGuiPopup.h>
#include <MiscTab.h>
#include <MumbleLink.h>
#include <ShaderManager.h>
#include <GFXSettings.h>
#include <Log.h>
#include <Version.h>
#include <MiscTab.h>
#include <common/baseresource.h>

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
	{
	}
};

void Core::InnerInitPreImGui()
{
	ClarityMiscTab::init<ClarityMiscTab>();

}

void Core::InnerInitPreFontImGui()
{
	auto& imio = ImGui::GetIO();
	auto fontCfg = ImFontConfig();
	fontCfg.FontDataOwnedByAtlas = false;

	if (const auto data = LoadResource(dllModule_, IDR_FONT_BLACK); data.data())
		fontBuffCounter_ = imio.Fonts->AddFontFromMemoryTTF(data.data(), int(data.size_bytes()), 128.f, &fontCfg);
}

void Core::InnerInitPostImGui()
{
	firstMessageShown_ = std::make_unique<ConfigurationOption<bool>>("", "first_message_shown_v1", "Core", false);

	buffs_ = std::make_unique<Buffs>(device_);
}

void Core::InnerInternalInit()
{
	ULONG_PTR contextToken;
	if (CoGetContextToken(&contextToken) == CO_E_NOTINITIALIZED) {
		HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		if (hr != S_FALSE && hr != RPC_E_CHANGED_MODE && FAILED(hr))
			CriticalMessageBox(L"Could not initialize COM library: error code 0x%X.", hr);
	}

	if (!buffLib_)
	{
		wchar_t fn[MAX_PATH];
		GetModuleFileName(dllModule(), fn, MAX_PATH);

		std::filesystem::path buffsPath = fn;
#ifdef _DEBUG
		buffsPath = buffsPath.remove_filename() / "getbuffsd.dll";
		buffLib_ = LoadLibrary(buffsPath.wstring().c_str());
		if (!buffLib_)
#endif
		{
			buffsPath = buffsPath.remove_filename() / "getbuffs.dll";
			buffLib_ = LoadLibrary(buffsPath.wstring().c_str());
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
	if(getBuffs_)
		buffs_->UpdateBuffsTable(getBuffs_());
}

void Core::InnerUpdate()
{
}

void Core::InnerDraw()
{
	if (!firstMessageShown_->value())
		ImGuiPopup("Welcome to GW2Clarity!").Position({ 0.5f, 0.45f }).Size({ 0.35f, 0.2f }).Display([&](const ImVec2& windowSize)
			{
				ImGui::TextWrapped("Welcome to GW2Clarity! This addon provides extensive UI customization options to make it easier to parse gameplay and situations. "
					"To begin, use the shortcut Shift+Alt+P to open the settings menu and take a moment to bind your keys. If you ever need further assistance, please visit "
					"this project's website at");

				ImGui::Spacing();
				ImGui::SetCursorPosX(windowSize.x * 0.1f);

				if (ImGui::Button("https://github.com/Friendly0Fire/GW2Clarity", ImVec2(windowSize.x * 0.8f, ImGui::GetFontSize() * 1.3f)))
					ShellExecute(0, 0, L"https://github.com/Friendly0Fire/GW2Clarity", 0, 0, SW_SHOW);
			}, [&]() { firstMessageShown_->value(true); });

	buffs_->Draw(context_);
}

}