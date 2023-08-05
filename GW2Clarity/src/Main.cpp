#include <Core.h>
#include <Direct3D11Loader.h>
#include <Main.h>
#include <Tag.h>
#include <Version.h>
#include <gw2al_api.h>
#include <gw2al_d3d9_wrapper.h>
#include <imgui.h>

const char* GetAddonName()
{
    return "GW2Clarity";
}
const wchar_t* GetAddonNameW()
{
    return L"GW2Clarity";
}
const char* GetAddonVersionString()
{
    return GW2CLARITY_VER;
}
const semver::version& GetAddonVersion()
{
    return CurrentVersion;
}
BaseCore& GetBaseCore()
{
    return GW2Clarity::Core::i();
}

gw2al_addon_dsc gAddonDeps[] = {
    GW2AL_CORE_DEP_ENTRY, D3D_WRAPPER_DEP_ENTRY, {0, 0, 0, 0, 0, 0}
};

gw2al_addon_dsc  gAddonDsc = { L"gw2clarity", L"GUI customization overlay", 2, 2, 1, gAddonDeps };

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

std::ofstream g_logStream;


bool WINAPI   DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved)
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

#include <Shlwapi.h>
#include <delayimp.h>

bool    loadedModules     = false;
HMODULE appCoreMod        = nullptr;
HMODULE ultralightMod     = nullptr;
HMODULE ultralightCoreMod = nullptr;
HMODULE webCoreMod        = nullptr;

void    LoadUltralightDLLs()
{
    if (loadedModules)
        return;

    HMODULE hModule;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)LoadUltralightDLLs, &hModule);
    char path[MAX_PATH];
    GetModuleFileNameA(hModule, path, MAX_PATH);
    std::filesystem::path dir(path);
    dir               = dir.parent_path();

    // Load DLL without any dependencies first
    ultralightCoreMod = LoadLibraryW((dir / "UltralightCore.dll").c_str());

    // Depends on the above
    webCoreMod        = LoadLibraryW((dir / "WebCore.dll").c_str());

    // Depends on the two above
    ultralightMod     = LoadLibraryW((dir / "Ultralight.dll").c_str());

    // Depends on all of the above
    appCoreMod        = LoadLibraryW((dir / "AppCore.dll").c_str());

    loadedModules     = true;
}

HMODULE LoadLocalDLL(LPCSTR name)
{
    LoadUltralightDLLs();

    if (StrCmpICA(name, "appcore.dll") == 0)
        return std::exchange(appCoreMod, nullptr);
    if (StrCmpICA(name, "ultralight.dll") == 0)
        return std::exchange(ultralightMod, nullptr);
    if (StrCmpICA(name, "ultralightcore.dll") == 0)
        return std::exchange(ultralightCoreMod, nullptr);
    if (StrCmpICA(name, "webcore.dll") == 0)
        return std::exchange(webCoreMod, nullptr);
}

FARPROC WINAPI delayHook(unsigned dliNotify, PDelayLoadInfo pdli)
{
    if (dliNotify == dliNotePreLoadLibrary)
    {
        return (FARPROC)LoadLocalDLL(pdli->szDll);
    }

    return NULL;
}

extern "C" const PfnDliHook __pfnDliNotifyHook2  = delayHook;
extern "C" const PfnDliHook __pfnDliFailureHook2 = delayHook;