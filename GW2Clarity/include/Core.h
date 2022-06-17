#pragma once

#include <Main.h>
#include <Singleton.h>
#include <Defs.h>
#include <d3d11_1.h>
#include <dxgi.h>
#include <Direct3D11Loader.h>
#include <ConfigurationOption.h>
#include <Grids.h>
#include <Sets.h>
#include <Cursor.h>
#include <variant>

class ShaderManager;

namespace GW2Clarity
{

using GetBuffsCallback = StackedBuff*(__cdecl*)();

class Core : public BaseCore, public Singleton<Core>
{
public:
	[[nodiscard]] ImFont* fontBuffCounter() const { return fontBuffCounter_; }
	[[nodiscard]] ImGuiID confirmDeletionPopupID() const { return confirmDeletionPopupID_; }

	struct DeletionInfo {
		std::string_view name, typeName, tail;
		std::variant<char, short, Id> id;
	};

	void DisplayDeletionMenu(DeletionInfo&& id);
	void DisplayErrorPopup(const char* message);

	glm::vec2 screenDims() const { return glm::vec2(screenWidth_, screenHeight_); }

protected:
	void InnerDraw() override;
	void InnerUpdate() override;
	void InnerInitPreImGui() override;
	void InnerInitPreFontImGui() override;
	void InnerInitPostImGui() override;
	void InnerInternalInit() override;
	void InnerShutdown() override;
	void InnerFrequentUpdate() override;

	[[nodiscard]] uint GetShaderArchiveID() const override { return IDR_SHADERS; }
	[[nodiscard]] const wchar_t* GetShaderDirectory() const override { return SHADERS_DIR; }
	[[nodiscard]] const wchar_t* GetGithubRepoSubUrl() const override { return L"Friendly0Fire/GW2Clarity"; }

	std::unique_ptr<ConfigurationOption<bool>> firstMessageShown_;
	std::unique_ptr<Grids> grids_;
	std::unique_ptr<Sets> sets_;
	std::unique_ptr<Cursor> cursor_;
	HMODULE buffLib_ = nullptr;
	GetBuffsCallback getBuffs_ = nullptr;

	ImFont* fontBuffCounter_ = nullptr;
	
	static inline const char* ConfirmDeletionPopupName = "Confirm Deletion";
	ImGuiID                   confirmDeletionPopupID_  = 0;
	DeletionInfo              confirmDeletionInfo_;
	
	
	static inline const char* ErrorPopupName = "GW2Clarity Error";
	ImGuiID                   errorPopupID_  = 0;
	std::string				  errorPopupMessage_;
};
}