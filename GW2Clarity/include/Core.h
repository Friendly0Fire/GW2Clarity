#pragma once

#include <ConfigurationOption.h>
#include <Cursor.h>
#include <Defs.h>
#include <Direct3D11Loader.h>
#include <Grids.h>
#include <Layouts.h>
#include <Main.h>
#include <Singleton.h>
#include <d3d11_1.h>
#include <dxgi.h>
#include <variant>

#include <AppCore/AppCore.h>

class ShaderManager;

namespace GW2Clarity
{

using GetBuffsCallback = StackedBuff*(__cdecl*)();

class Core : public BaseCore, public Singleton<Core>
{
public:
    [[nodiscard]] ImFont* fontBuffCounter() const
    {
        return fontBuffCounter_;
    }
    [[nodiscard]] ImGuiID confirmDeletionPopupID() const
    {
        return confirmDeletionPopupID_;
    }

    struct DeletionInfo
    {
        std::string_view                    name, typeName, tail;
        std::variant<char, short, Id, uint> id;
    };

    void      DisplayDeletionMenu(DeletionInfo&& id);

    glm::vec2 screenDims() const
    {
        return glm::vec2(screenWidth_, screenHeight_);
    }

    void PostResizeSwapChain(unsigned int w, unsigned int h) override;

protected:
    void                   InnerDraw() override;
    void                   InnerUpdate() override;
    void                   InnerInitPreImGui() override;
    void                   InnerInitPreFontImGui() override;
    void                   InnerInitPostImGui() override;
    void                   InnerInternalInit() override;
    void                   InnerShutdown() override;
    void                   InnerFrequentUpdate() override;

    std::optional<LRESULT> OnInput(UINT msg, WPARAM& wParam, LPARAM& lParam) override;

    [[nodiscard]] uint     GetShaderArchiveID() const override
    {
        return IDR_SHADERS;
    }
    [[nodiscard]] const wchar_t* GetShaderDirectory() const override
    {
        return SHADERS_DIR;
    }
    [[nodiscard]] const wchar_t* GetGithubRepoSubUrl() const override
    {
        return L"Friendly0Fire/GW2Clarity";
    }

    std::unique_ptr<ConfigurationOption<bool>> firstMessageShown_;
    std::unique_ptr<Styles>                    styles_;
    std::unique_ptr<Buffs>                     buffs_;
    std::unique_ptr<Grids>                     grids_;
    std::unique_ptr<Layouts>                   layouts_;
    std::unique_ptr<Cursor>                    cursor_;
    ultralight::RefPtr<ultralight::Renderer>   ulrenderer_;
    ultralight::RefPtr<ultralight::View>       ulview_;
    Texture2D                                  ultex_;
    ConstantBufferSPtr<glm::vec4>              ulcb_;
    ShaderId                                   ulps_, ulvs_;
    std::vector<std::byte>                     ulbuf_;
    uint                                       ulupdmsg_;
    uint                                       ulstride_ = 0, ulheight_ = 0;
    bool                                       uldirty_ = false;
    ComPtr<ID3D11SamplerState>                 defaultSampler_;
    ComPtr<ID3D11BlendState>                   defaultBlend_;
    HMODULE                                    buffLib_                 = nullptr;
    GetBuffsCallback                           getBuffs_                = nullptr;

    ImFont*                                    fontBuffCounter_         = nullptr;

    static inline const char*                  ConfirmDeletionPopupName = "Confirm Deletion";
    ImGuiID                                    confirmDeletionPopupID_  = 0;
    DeletionInfo                               confirmDeletionInfo_;
};
} // namespace GW2Clarity