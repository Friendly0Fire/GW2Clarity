#include <Core.h>
#include <GridRenderer.h>
#include <ImGuiExtensions.h>
#include <SimpleIni.h>
#include <algorithm>
#include <cppcodec/base64_rfc4648.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <range/v3/all.hpp>
#include <shellapi.h>
#include <skyr/percent_encoding/percent_encode.hpp>
#include <variant>

namespace GW2Clarity
{

GridRenderer::GridRenderer(ComPtr<ID3D11Device>& dev)
{
    buffsAtlas_      = CreateTextureFromResource(dev.Get(), Core::i().dllModule(), IDR_BUFFS);
    numbersAtlas_    = CreateTextureFromResource(dev.Get(), Core::i().dllModule(), IDR_NUMBERS);

    auto& sm         = ShaderManager::i();

    gridCB_          = ShaderManager::i().MakeConstantBuffer<GridConstants>();
    screenSpaceVS_   = sm.GetShader(L"Grids.hlsl", D3D11_SHVER_VERTEX_SHADER, "Grids_VS");
    gridsPS_         = sm.GetShader(L"Grids.hlsl", D3D11_SHVER_PIXEL_SHADER, "Grids_PS");
    gridsFilteredPS_ = sm.GetShader(L"Grids.hlsl", D3D11_SHVER_PIXEL_SHADER, "FilteredGrids_PS");

    D3D11_BUFFER_DESC instanceBufferDesc;

    instanceBufferDesc.Usage               = D3D11_USAGE_DYNAMIC;
    instanceBufferDesc.ByteWidth           = sizeof(InstanceData) * instanceBufferSize_s;
    instanceBufferDesc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
    instanceBufferDesc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
    instanceBufferDesc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    instanceBufferDesc.StructureByteStride = sizeof(InstanceData);

    GW2_CHECKED_HRESULT(dev->CreateBuffer(&instanceBufferDesc, nullptr, instanceBuffer_.GetAddressOf()));

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;

    srvDesc.Format              = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension       = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements  = instanceBufferSize_s;

    GW2_CHECKED_HRESULT(dev->CreateShaderResourceView(instanceBuffer_.Get(), &srvDesc, instanceBufferView_.GetAddressOf()));

    CD3D11_BLEND_DESC blendDesc(D3D11_DEFAULT);
    blendDesc.RenderTarget[0].BlendEnable    = true;
    blendDesc.RenderTarget[0].SrcBlend       = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend      = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    GW2_CHECKED_HRESULT(dev->CreateBlendState(&blendDesc, defaultBlend_.GetAddressOf()));

    CD3D11_SAMPLER_DESC sampDesc(D3D11_DEFAULT);
    GW2_CHECKED_HRESULT(dev->CreateSamplerState(&sampDesc, defaultSampler_.GetAddressOf()));
}

} // namespace GW2Clarity