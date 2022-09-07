#include <Core.h>
#include <GridRenderer.h>

namespace GW2Clarity
{

BaseGridRenderer::BaseGridRenderer(ComPtr<ID3D11Device>& dev, const Buffs* buffs, size_t sz)
    : buffs_(buffs)
    , bufferSize_(sz * sizeof(InstanceData))
{
    auto& sm               = ShaderManager::i();

    gridCB_                = ShaderManager::i().MakeConstantBuffer<GridConstants>();
    screenSpaceVS_         = sm.GetShader(L"Grids.hlsl", D3D11_SHVER_VERTEX_SHADER, "Grids_VS");
    screenSpaceNoExpandVS_ = sm.GetShader(L"Grids.hlsl", D3D11_SHVER_VERTEX_SHADER, "GridsNoExpand_VS");
    gridsPS_               = sm.GetShader(L"Grids.hlsl", D3D11_SHVER_PIXEL_SHADER, "Grids_PS");
    gridsFilteredPS_       = sm.GetShader(L"Grids.hlsl", D3D11_SHVER_PIXEL_SHADER, "FilteredGrids_PS");

    D3D11_BUFFER_DESC instanceBufferDesc;

    instanceBufferDesc.Usage               = D3D11_USAGE_DYNAMIC;
    instanceBufferDesc.ByteWidth           = UINT(sizeof(InstanceData) * sz);
    instanceBufferDesc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
    instanceBufferDesc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
    instanceBufferDesc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    instanceBufferDesc.StructureByteStride = sizeof(InstanceData);

    GW2_CHECKED_HRESULT(dev->CreateBuffer(&instanceBufferDesc, nullptr, instanceBuffer_.GetAddressOf()));

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;

    srvDesc.Format              = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension       = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements  = UINT(sz);

    GW2_CHECKED_HRESULT(dev->CreateShaderResourceView(instanceBuffer_.Get(), &srvDesc, instanceBufferView_.GetAddressOf()));

    CD3D11_BLEND_DESC blendDesc(D3D11_DEFAULT);
    blendDesc.RenderTarget[0].BlendEnable    = true;
    blendDesc.RenderTarget[0].SrcBlend       = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend      = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    GW2_CHECKED_HRESULT(dev->CreateBlendState(&blendDesc, defaultBlend_.GetAddressOf()));

    CD3D11_SAMPLER_DESC samplerDesc(D3D11_DEFAULT);
    GW2_CHECKED_HRESULT(dev->CreateSamplerState(&samplerDesc, defaultSampler_.GetAddressOf()));
}

void BaseGridRenderer::Draw(ComPtr<ID3D11DeviceContext>& ctx, std::span<InstanceData> data, bool betterFiltering, RenderTarget* rt, bool expandVS)
{
    D3D11_MAPPED_SUBRESOURCE map;
    ctx->Map(instanceBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
    memcpy_s(map.pData, bufferSize_, data.data(), data.size_bytes());
    ctx->Unmap(instanceBuffer_.Get(), 0);

    ID3D11ShaderResourceView* srvs[] = { instanceBufferView_.Get(), buffs_->buffsAtlas().srv.Get(), buffs_->numbersAtlas().srv.Get() };
    ctx->VSSetShaderResources(0, 3, srvs);
    ctx->PSSetShaderResources(0, 3, srvs);
    ctx->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    D3D11_VIEWPORT oldVP;
    auto&          sm = ShaderManager::i();
    if (rt)
    {
        UINT numVPs = 1;
        ctx->RSGetViewports(&numVPs, &oldVP);

        D3D11_TEXTURE2D_DESC desc;
        rt->texture->GetDesc(&desc);
        gridCB_->screenSize = glm::vec4(desc.Width, desc.Height, 1.f / desc.Width, 1.f / desc.Height);

        // Setup viewport
        D3D11_VIEWPORT vp;
        memset(&vp, 0, sizeof(D3D11_VIEWPORT));
        vp.Width    = FLOAT(desc.Width);
        vp.Height   = FLOAT(desc.Height);
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = vp.TopLeftY = 0;
        ctx->RSSetViewports(1, &vp);
    }
    else
        gridCB_->screenSize = glm::vec4(Core::i().screenDims(), 1.f / Core::i().screenDims());
    gridCB_->atlasUVSize   = buffs_->buffsAtlasUVSize();
    gridCB_->numbersUVSize = buffs_->numbersAtlasUVSize();
    gridCB_.Update(ctx.Get());
    sm.SetConstantBuffers(ctx.Get(), gridCB_);
    sm.SetShaders(ctx.Get(), expandVS ? screenSpaceVS_ : screenSpaceNoExpandVS_, betterFiltering ? gridsFilteredPS_ : gridsPS_);

    ID3D11SamplerState* samplers[] = { defaultSampler_.Get() };
    ctx->PSSetSamplers(0, 1, samplers);

    ctx->OMSetBlendState(defaultBlend_.Get(), nullptr, 0xffffffff);

    if (rt)
    {
        ID3D11RenderTargetView* rtvs[] = { rt->rtv.Get() };
        ctx->OMSetRenderTargets(1, rtvs, nullptr);
    }

    ctx->DrawInstanced(4, UINT(data.size()), 0, 0);

    if (rt)
    {
        ID3D11RenderTargetView* rtvs[] = { Core::i().backBufferRTV().Get() };
        ctx->OMSetRenderTargets(1, rtvs, nullptr);
        ctx->RSSetViewports(1, &oldVP);
    }
}

} // namespace GW2Clarity