#include "Cursor.h"

#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <range/v3/all.hpp>

#include "Core.h"
#include "ImGuiExtensions.h"

namespace GW2Clarity
{

Cursor::Cursor(ComPtr<ID3D11Device>& dev) : activateCursor_("activate_cursor", "Toggle Cursor", "General") {
    Load();

    activateCursor_.callback([&](Activated a) {
        if(a == Activated::Yes)
            visible_ = !visible_;
        return PassToGame::Allow;
    });

    auto& sm = ShaderManager::i();
    cursorCB_ = sm.MakeConstantBuffer<CursorData>();
    screenSpaceVS_ = sm.GetShader(L"ScreenQuad.hlsl", D3D11_SHVER_VERTEX_SHADER, "ScreenQuad", { { "CURSOR_HLSL" } });
    auto makePS = [&](const char* ep) {
        return sm.GetShader(L"Cursor.hlsl", D3D11_SHVER_PIXEL_SHADER, ep);
    };

    const auto circlePS = makePS("Circle");
    const auto squarePS = makePS("Square");
    const auto crossPS = makePS("Cross");
    cursorPS_[get_index<Layer::Circle, decltype(Layer::type)>()] = circlePS;
    cursorPS_[get_index<Layer::Square, decltype(Layer::type)>()] = squarePS;
    cursorPS_[get_index<Layer::Cross, decltype(Layer::type)>()] = crossPS;

    CD3D11_BLEND_DESC blendDesc(D3D11_DEFAULT);
    blendDesc.RenderTarget[0].BlendEnable = true;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    GW2_CHECKED_HRESULT(dev->CreateBlendState(&blendDesc, defaultBlend_.GetAddressOf()));

    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_SUBTRACT;
    GW2_CHECKED_HRESULT(dev->CreateBlendState(&blendDesc, invertBlend_.GetAddressOf()));

    SettingsMenu::i().AddImplementer(this);
}

Cursor::~Cursor() = default;

void Cursor::Draw(ComPtr<ID3D11DeviceContext>& ctx) {
    if(!SettingsMenu::i().isVisible())
        layerSelector_.Deselect();

    if(!visible_ && !layerSelector_.selected())
        return;

    auto& cb = *cursorCB_;

    const auto& io = ImGui::GetIO();
    auto mp = FromImGui(io.MousePos) / Core::i().screenDims();

    if(glm::any(glm::lessThan(mp, vec2(0.f))) || glm::any(glm::greaterThan(mp, vec2(1.f))))
        return;

    for(auto& l : layers_) {
        // if(l.invert)
        //     ctx->OMSetBlendState(invertBlend_.Get(), nullptr, 0xffffffff);
        // else
        //     ctx->OMSetBlendState(defaultBlend_.Get(), nullptr, 0xffffffff);

        if(const auto* c = std::get_if<Layer::Cross>(&l.type); c && c->fullscreen)
            l.dims = vec2(f32(std::max(Core::i().screenWidth(), Core::i().screenHeight())) * 2.f);

        cb->colorFill = l.colorFill;
        cb->colorBorder = l.colorBorder;
        f32 div = std::min(l.dims.x, l.dims.y);
        cb->parameters = vec4(l.edgeThickness * 1.f / div, 0.f, 0.f, 0.f);
        std::visit(Overloaded { [&](const Layer::Circle&) {},
                                [&](const Layer::Cross& c) {
                                    cb->parameters.y = c.crossThickness / div;
                                    cb->parameters.z = c.angle / 180.f * std::numbers::pi_v<f32>;
                                },
                                [&](const Layer::Square& s) {
                                    cb->parameters.z = s.angle / 180.f * std::numbers::pi_v<f32>;
                                } },
                   l.type);
        cb->dimensions = vec4(mp, l.dims / Core::i().screenDims());
        cb.Update(ctx.Get());

        ShaderManager::i().SetConstantBuffers(ctx.Get(), cb);
        ShaderManager::i().SetShaders(ctx.Get(), screenSpaceVS_, cursorPS_[l.type.index()]);

        DrawScreenQuad(ctx.Get());
    }
}

void Cursor::DrawMenu(Keybind** currentEditedKeybind) {
    if(layerSelector_.Draw()) {
        layers_.emplace_back();
        save_();
    }

    if(auto* i = layerSelector_.selectedItem()) {
        auto& editLayer = *i;
        if(!editLayer.name.empty())
            ImGuiTitle(std::format("Editing Cursor Layer '{}'", editLayer.name).c_str(), 0.75f);
        else
            ImGuiTitle("New Cursor Layer", 0.75f);

        save_(ImGui::InputText("Name##NewLayer", &editLayer.name));

        auto currTypeName = std::visit(Overloaded { [](const Layer::Circle&) { return "Circle"; },
                                                    [](const Layer::Square&) { return "Square"; },
                                                    [](const Layer::Cross&) {
                                                        return "Cross";
                                                    } },
                                       editLayer.type);

        if(auto _ = UI::Scoped::Combo("Type", currTypeName)) {
            const auto currIndex = editLayer.type.index();
            const auto isCircle = get_index<Layer::Circle, decltype(editLayer.type)>() == currIndex;
            const auto isSquare = get_index<Layer::Square, decltype(editLayer.type)>() == currIndex;
            const auto isCross = get_index<Layer::Cross, decltype(editLayer.type)>() == currIndex;
            if(save_(ImGui::Selectable("Circle", isCircle) && !isCircle)) {
                editLayer.type = Layer::Circle {};
            }

            if(save_(ImGui::Selectable("Square", isSquare) && !isSquare)) {
                editLayer.type = Layer::Square {};
            }

            if(save_(ImGui::Selectable("Cross", isCross) && !isCross)) {
                editLayer.type = Layer::Cross {};
            }
        }

        save_(ImGui::ColorEdit4("Border Color & Transparency",
                                glm::value_ptr(editLayer.colorBorder),
                                ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreview));

        save_(ImGui::ColorEdit4("Fill Color & Transparency",
                                glm::value_ptr(editLayer.colorFill),
                                ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreview));

        save_(ImGui::DragFloat("Border Thickness", &editLayer.edgeThickness, 0.05f, 0.f, 100.f));

        std::visit(Overloaded { [](Layer::Circle&) {},
                                [this](Layer::Square& s) { save_(ImGui::DragFloat("Angle", &s.angle, 0.1f, 0.f, 360.f)); },
                                [this, &editLayer](Layer::Cross& c) {
                                    save_(ImGui::DragFloat("Angle", &c.angle, 0.1f, 0.f, 360.f));
                                    save_(ImGui::DragFloat(
                                        "Cross Thickness", &c.crossThickness, 0.05f, 1.f, std::min(editLayer.dims.x, editLayer.dims.y)));
                                    if(save_(ImGui::Checkbox("Full screen", &c.fullscreen)))
                                        if(!c.fullscreen)
                                            editLayer.dims = vec2(32.f);
                                } },
                   editLayer.type);

        if(!std::holds_alternative<Layer::Cross>(editLayer.type) || !std::get<Layer::Cross>(editLayer.type).fullscreen)
            save_(ImGui::DragFloat2("Cursor Size", glm::value_ptr(editLayer.dims), 0.2f, 1.f, ImGui::GetIO().DisplaySize.x * 2.f));
    }

    ImGui::Separator();

    ImGuiKeybindInput(activateCursor_, currentEditedKeybind, "Toggles cursor layers visibility.");

    if(save_.ShouldSave())
        Save();
}

void Cursor::Load() {
    using namespace nlohmann;
    layers_.clear();
    layerSelector_.Deselect();

    auto& cfg = JSONConfigurationFile::i();
    cfg.Reload();

    const auto& layers = cfg.json()["cursor_layers"];
    for(const auto& l : layers)
        layers_.push_back(l.get<Layer>());
}

void Cursor::Save() {
    using namespace nlohmann;

    auto& cfg = JSONConfigurationFile::i();

    auto& layers = cfg.json()["cursor_layers"];
    layers = json::array();
    for(const auto& l : layers_)
        layers.push_back(l);

    cfg.Save();
    save_.Saved();
}

} // namespace GW2Clarity
