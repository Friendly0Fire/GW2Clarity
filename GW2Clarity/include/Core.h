#pragma once

#include <d3d11_1.h>
#include <dxgi.h>

#include "ConfigurationOption.h"
#include "Cursor.h"
//#include "Grids.h"
//#include "Layouts.h"
#include "Main.h"
#include "Resource.h"
#include "Singleton.h"

class ShaderManager;

namespace GW2Clarity
{

using GetBuffsCallback = StackedBuff*(__cdecl*)();

class Core : public BaseCore, public Singleton<Core>
{
public:
    [[nodiscard]] ImFont* fontBuffCounter() const {
        return fontBuffCounter_;
    }

    vec2 screenDims() const {
        return vec2(screenWidth_, screenHeight_);
    }

    const Texture2D& testImage() const {
        return testImage_;
    }

protected:
    void InnerDraw() override;
    void InnerUpdate() override;
    void InnerInitPreImGui() override;
    void InnerInitPreFontImGui() override;
    void InnerInitPostImGui() override;
    void InnerInternalInit() override;
    void InnerShutdown() override;
    void InnerFrequentUpdate() override;
    void MockInit() override;

    [[nodiscard]] u32 GetShaderArchiveID() const override {
        return IDR_SHADERS;
    }
    [[nodiscard]] const wchar_t* GetShaderDirectory() const override {
        return SHADERS_DIR;
    }
    [[nodiscard]] const wchar_t* GetGithubRepoSubUrl() const override {
        return L"Friendly0Fire/GW2Clarity";
    }

    std::unique_ptr<ConfigurationOption<bool>> firstMessageShown_;
    // std::unique_ptr<Styles> styles_;
    // std::unique_ptr<Buffs> buffs_;
    // std::unique_ptr<Grids> grids_;
    // std::unique_ptr<Layouts> layouts_;
    std::unique_ptr<Cursor> cursor_;
    HMODULE buffLib_ = nullptr;
    GetBuffsCallback getBuffs_ = nullptr;

    ImFont* fontBuffCounter_ = nullptr;

    Texture2D testImage_;
};
} // namespace GW2Clarity
