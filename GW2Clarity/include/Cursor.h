#pragma once

#include <Main.h>
#include <Graphics.h>
#include <glm/glm.hpp>
#include <map>
#include <ConfigurationFile.h>
#include <mutex>
#include <span>
#include <imgui.h>
#include <set>
#include <ActivationKeybind.h>
#include <SettingsMenu.h>
#include <ShaderManager.h>

namespace GW2Clarity
{

class Cursor : public SettingsMenu::Implementer
{
public:
	explicit Cursor(ComPtr<ID3D11Device>& dev);
	virtual ~Cursor();

	void Draw(ComPtr<ID3D11DeviceContext>& ctx);
	void DrawMenu(Keybind** currentEditedKeybind) override;

	const char* GetTabName() const override { return "Cursor"; }

private:
	struct CursorData {
		glm::vec4 dimensions;
		glm::vec4 parameters;
		glm::vec4 color;
	};
	ConstantBuffer<CursorData> cursorCB_;
	ShaderId screenSpaceVS_;
	ShaderId cursorImagePS_, cursorCrossPS_;

	ComPtr<ID3D11BlendState> defaultBlend_, invertBlend_;
	ComPtr<ID3D11SamplerState> defaultSampler_;

	struct ImageParams {
		float alphaWeight = 1.f;
	};
	struct CrossParams {
		float thickness = 4.f;
	};

	struct Layer {
		glm::vec2 dimensions { 32.f };
		glm::vec4 color { 1.f };
		bool invert = false;

		std::variant<ImageParams, CrossParams> parameters;
	};

	std::vector<Layer> layers_;
};
}
