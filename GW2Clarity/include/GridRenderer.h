#pragma once

#include <ActivationKeybind.h>
#include <Graphics.h>
#include <Main.h>
#include <Sets.h>
#include <ShaderManager.h>
#include <glm/glm.hpp>

namespace GW2Clarity
{
struct GridInstanceData
{
    glm::vec4 posDims;
    glm::vec2 uv;
    glm::vec2 numberUV;
    glm::vec4 tint;
    glm::vec4 borderColor;
    glm::vec4 glowColor;
    float     borderThickness;
    float     glowSize;
    bool      showNumber;
    int       _;
};

class GridRenderer
{
    using InstanceData = GridInstanceData;

public:
    GridRenderer(ComPtr<ID3D11Device>& dev);
    GridRenderer(const GridRenderer&)            = delete;
    GridRenderer(GridRenderer&&)                 = delete;
    GridRenderer& operator=(const GridRenderer&) = delete;
    GridRenderer& operator=(GridRenderer&&)      = delete;
    ~GridRenderer()                              = default;

    void                      Draw(ComPtr<ID3D11DeviceContext>& ctx);

    [[nodiscard]] const auto& buffsAtlas() const
    {
        return buffsAtlas_;
    }

protected:
    Texture2D buffsAtlas_;
    Texture2D numbersAtlas_;

    ShaderId  screenSpaceVS_;
    ShaderId  gridsPS_;
    ShaderId  gridsFilteredPS_;

    struct GridConstants
    {
        glm::vec4 screenSize;
        glm::vec2 atlasUVSize;
        glm::vec2 numbersUVSize;
    };
    ConstantBuffer<GridConstants>                  gridCB_;

    ComPtr<ID3D11Buffer>                           instanceBuffer_;
    ComPtr<ID3D11ShaderResourceView>               instanceBufferView_;
    ComPtr<ID3D11BlendState>                       defaultBlend_;
    ComPtr<ID3D11SamplerState>                     defaultSampler_;

    static constexpr size_t                        instanceBufferSize_s = 1024;
    std::array<InstanceData, instanceBufferSize_s> instanceBufferSource_;
    uint                                           instanceBufferCount_ = 0;
};
} // namespace GW2Clarity