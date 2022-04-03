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

void RegisterArc();

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

void Core::InnerInitPostImGui()
{
	firstMessageShown_ = std::make_unique<ConfigurationOption<bool>>("", "first_message_shown_v1", "Core", false);

	buffs_ = std::make_unique<Buffs>(device_);
}

uintptr_t Core::CombatEvent(cbtevent* ev, ag* src, ag* dst, char* skillname, uint64_t id, uint64_t revision)
{
	if (ev && ev->value)
	{
		if (ev->is_buffremove && src && src->self)
			Core::i().RemoveBuff(ev->skillid, ev->result);
		else if (ev->buff && !ev->buff_dmg)
			Core::i().ApplyBuff(ev->skillid);
	}

	return 0;
}

void Core::ApplyBuff(int id)
{
	buffs_->ChangeBuff(id, 1);
}

void Core::RemoveBuff(int id, int count)
{
	buffs_->ChangeBuff(id, -count);
}

void Core::InnerInternalInit()
{
	ULONG_PTR contextToken;
	if (CoGetContextToken(&contextToken) == CO_E_NOTINITIALIZED) {
		HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		if (hr != S_FALSE && hr != RPC_E_CHANGED_MODE && FAILED(hr))
			CriticalMessageBox(L"Could not initialize COM library: error code 0x%X.", hr);
	}
}

void Core::InnerShutdown()
{
	CoUninitialize();
}

void Core::InnerUpdate()
{
	if (!arcInstalled_)
	{
		RegisterArc();
		arcInstalled_ = true;
	}
}

void Core::InnerDraw()
{
	if (!firstMessageShown_->value())
		ImGuiPopup("Welcome to GW2Clarity!").Position({ 0.5f, 0.45f }).Size({ 0.35f, 0.2f }).Display([&](const ImVec2& windowSize)
			{
				ImGui::TextWrapped("Welcome to GW2Clarity! This addon provides extensive UI customization options to make it easier to parse gameplay and situations. "
					"To begin, use the shortcut Shift+Alt+C to open the settings menu and take a moment to bind your keys. If you ever need further assistance, please visit "
					"this project's website at");

				ImGui::Spacing();
				ImGui::SetCursorPosX(windowSize.x * 0.1f);

				if (ImGui::Button("https://github.com/Friendly0Fire/GW2Clarity", ImVec2(windowSize.x * 0.8f, ImGui::GetFontSize() * 1.3f)))
					ShellExecute(0, 0, L"https://github.com/Friendly0Fire/GW2Clarity", 0, 0, SW_SHOW);
			}, [&]() { firstMessageShown_->value(true); });

	buffs_->Draw(context_);
}

}