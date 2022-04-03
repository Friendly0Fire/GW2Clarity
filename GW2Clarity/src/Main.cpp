#include <Main.h>
#include <Core.h>
#include <Direct3D11Loader.h>
#include <gw2al_api.h>
#include <gw2al_d3d9_wrapper.h>
#include <Tag.h>
#include <Version.h>
#include <imgui.h>

const char* GetAddonName() { return "GW2Clarity"; }
const wchar_t* GetAddonNameW() { return L"GW2Clarity"; }
const char* GetAddonVersionString() { return GW2CLARITY_VER; }
const semver::version& GetAddonVersion() { return CurrentVersion; }
BaseCore& GetBaseCore()
{
	return GW2Clarity::Core::i();
}

gw2al_addon_dsc gAddonDeps[] = {
	GW2AL_CORE_DEP_ENTRY,
	D3D_WRAPPER_DEP_ENTRY,
	{0,0,0,0,0,0}
};

gw2al_addon_dsc gAddonDsc = {
	L"gw2clarity",
	L"GUI customization overlay",
	2,
	2,
	1,
	gAddonDeps
};

gw2al_addon_dsc* gw2addon_get_description()
{
	return &gAddonDsc;
}

gw2al_api_ret gw2addon_load(gw2al_core_vtable* core_api)
{
	Direct3D11Loader::reset();
	Direct3D11Loader::i().Init(core_api);
	return GW2AL_OK;
}

gw2al_api_ret gw2addon_unload(int gameExiting)
{
	return GW2AL_OK;
}

/* arcdps export table */
typedef struct arcdps_exports {
	uintptr_t size; /* size of exports table */
	uint32_t sig; /* pick a number between 0 and uint32_t max that isn't used by other modules */
	uint32_t imguivers; /* set this to IMGUI_VERSION_NUM. if you don't use imgui, 18000 (as of 2021-02-02) */
	const char* out_name; /* name string */
	const char* out_build; /* build string */
	void* wnd_nofilter; /* wndproc callback, fn(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam), return assigned to umsg */
	void* combat; /* combat event callback, fn(cbtevent* ev, ag* src, ag* dst, char* skillname, uint64_t id, uint64_t revision) */
	void* imgui; /* ::present callback, before imgui::render, fn(uint32_t not_charsel_or_loading) */
	void* options_end; /* ::present callback, appending to the end of options window in arcdps, fn() */
	void* combat_local;  /* combat event callback like area but from chat log, fn(cbtevent* ev, ag* src, ag* dst, char* skillname, uint64_t id, uint64_t revision) */
	void* wnd_filter; /* wndproc callback like wnd_nofilter above, input filered using modifiers */
	void* options_windows; /* called once per 'window' option checkbox, with null at the end, non-zero return disables arcdps drawing that checkbox, fn(char* windowname) */
} arcdps_exports;
using Arc_addextension = int32_t(*)(arcdps_exports*, uint32_t, HINSTANCE);

uintptr_t ArcWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return uMsg;
}

arcdps_exports* GetArcExports()
{
	static arcdps_exports exports{
			sizeof(arcdps_exports),
			0xf1ed1f1e,
			18000,
			GetAddonName(),
			GetAddonVersionString(),
			nullptr,
			GW2Clarity::Core::CombatEvent,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr
	};

	return &exports;
}

uintptr_t OnArcRelease() {
	return 0;
}

void* get_init_addr(char* arcversion, ImGuiContext* imguictx, void* id3dptr, HANDLE arcdll, void* mallocfn, void* freefn, uint32_t d3dversion)
{
	return GetArcExports;
}

void* get_release_addr()
{
	return OnArcRelease;
}

void RegisterArc()
{
	auto arc = LoadLibrary(L"gw2addon_arcdps.dll");
	if (arc)
	{
		auto registerSelf = (Arc_addextension)GetProcAddress(arc, "addextension");
		int32_t r = registerSelf(GetArcExports(), sizeof(arcdps_exports), GW2Clarity::Core::i().dllModule());
		GW2_ASSERT(r == 1);
	}
	FreeLibrary(arc);
}

std::ofstream g_logStream;


bool WINAPI DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
	    g_logStream = std::ofstream("gw2clarity.log");

		GW2Clarity::Core::Init(hModule);
		break;
	case DLL_PROCESS_DETACH:
		break;
	}

	return true;
}