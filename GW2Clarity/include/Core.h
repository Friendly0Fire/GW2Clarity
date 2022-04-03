#pragma once

#include <Main.h>
#include <Singleton.h>
#include <Defs.h>
#include <d3d11_1.h>
#include <dxgi.h>
#include <Direct3D11Loader.h>
#include <ConfigurationOption.h>
#include <Buffs.h>

class ShaderManager;

namespace GW2Clarity
{

class Core : public BaseCore, public Singleton<Core>
{
public:
	static uintptr_t CombatEvent(cbtevent* ev, ag* src, ag* dst, char* skillname, uint64_t id, uint64_t revision);
	void ApplyBuff(int id);
	void RemoveBuff(int id, int count);
protected:
	void InnerDraw() override;
	void InnerUpdate() override;
	void InnerInitPreImGui() override;
	void InnerInitPostImGui() override;
	void InnerInternalInit() override;
	void InnerShutdown() override;

	uint GetShaderArchiveID() const override { return IDR_SHADERS; }
	const wchar_t* GetShaderDirectory() const override { return SHADERS_DIR; }
	const wchar_t* GetGithubRepoSubUrl() const override { return L"Friendly0Fire/GW2Clarity"; }

	std::unique_ptr<ConfigurationOption<bool>> firstMessageShown_;
	std::unique_ptr<Buffs> buffs_;
	bool arcInstalled_ = false;
};
}