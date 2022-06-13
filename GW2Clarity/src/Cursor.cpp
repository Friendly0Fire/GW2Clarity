#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>

#include <Cursor.h>
#include <Core.h>
#include <imgui.h>
#include <ImGuiExtensions.h>

namespace GW2Clarity
{

Cursor::Cursor(ComPtr<ID3D11Device>& dev) {
	auto& sm = ShaderManager::i();
	cursorCB_ = sm.MakeConstantBuffer<CursorData>();
	screenSpaceVS_ = sm.GetShader(L"ScreenQuad.hlsl", D3D11_SHVER_VERTEX_SHADER, "ScreenQuad");
	cursorImagePS_ = sm.GetShader(L"Cursor.hlsl", D3D11_SHVER_VERTEX_SHADER, "Image");
	cursorCrossPS_ = sm.GetShader(L"Cursor.hlsl", D3D11_SHVER_VERTEX_SHADER, "Cross");

	auto dev = Core::i().device();

	CD3D11_BLEND_DESC blendDesc(D3D11_DEFAULT);
	blendDesc.RenderTarget[0].BlendEnable = true;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	GW2_HASSERT(dev->CreateBlendState(&blendDesc, defaultBlend_.GetAddressOf()));

	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_REV_SUBTRACT;
	GW2_HASSERT(dev->CreateBlendState(&blendDesc, invertBlend_.GetAddressOf()));

	CD3D11_SAMPLER_DESC sampDesc(D3D11_DEFAULT);
	GW2_HASSERT(dev->CreateSamplerState(&sampDesc, defaultSampler_.GetAddressOf()));
}

Cursor::~Cursor() = default;

void Cursor::Draw(ComPtr<ID3D11DeviceContext>& ctx) {
	
}

void Cursor::DrawMenu(Keybind** currentEditedKeybind) {
	
}

}
