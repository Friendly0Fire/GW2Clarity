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
    cursorPS_ = { makePS("Circle"), makePS("Square"), makePS("Cross"), makePS("Smooth") };

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
        selectedLayerId_ = UnselectedSubId;

    if(!visible_ && selectedLayerId_ == UnselectedSubId)
        return;

    auto& cb = *cursorCB_;

    const auto& io = ImGui::GetIO();
    auto mp = FromImGui(io.MousePos) / Core::i().screenDims();

    if(glm::any(glm::lessThan(mp, glm::vec2(0.f))) || glm::any(glm::greaterThan(mp, glm::vec2(1.f))))
        return;

    for(auto& l : layers_) {
        if(l.invert)
            ctx->OMSetBlendState(invertBlend_.Get(), nullptr, 0xffffffff);
        else
            ctx->OMSetBlendState(defaultBlend_.Get(), nullptr, 0xffffffff);

        if(l.fullscreen)
            l.dims = glm::vec2(float(std::max(Core::i().screenWidth(), Core::i().screenHeight())) * 2.f);

        cb->color1 = l.color1;
        cb->color2 = l.color2;
        float div = std::min(l.dims.x, l.dims.y);
        cb->parameters = glm::vec4(l.edgeThickness * (l.type == CursorType::SMOOTH ? 1.f : 1.f / div), l.secondaryThickness / div,
                                   l.angle / 180.f * M_PI, 0.f);
        cb->dimensions = glm::vec4(mp, l.dims / Core::i().screenDims());
        cb.Update(ctx.Get());

        ShaderManager::i().SetConstantBuffers(ctx.Get(), cb);
        ShaderManager::i().SetShaders(ctx.Get(), screenSpaceVS_, cursorPS_[int(l.type)]);

        DrawScreenQuad(ctx.Get());
    }
}

void Cursor::DrawMenu(Keybind** currentEditedKeybind) {
    if(ImGui::BeginListBox("##LayersList", ImVec2(-FLT_MIN, 0.f))) {
        char newCurrentHovered = currentHoveredLayer_;
        for(auto&& [lid, l] : layers_ | ranges::views::enumerate) {
            if(ImGui::Selectable(std::format("{}##Layer", l.name).c_str(), selectedLayerId_ == lid || currentHoveredLayer_ == lid,
                                 ImGuiSelectableFlags_AllowItemOverlap)) {
                selectedLayerId_ = char(lid);
            }

            if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) || ImGui::IsItemActive()) {
                auto& style = ImGui::GetStyle();
                auto orig = style.Colors[ImGuiCol_Button];
                style.Colors[ImGuiCol_Button] *= ImVec4(0.5f, 0.5f, 0.5f, 1.f);
                if(ImGuiClose(std::format("CloseLayer{}", lid).c_str(), 0.75f, false)) {
                    selectedLayerId_ = char(lid);
                    Core::i().DisplayDeletionMenu({ l.name, "cursor layer", "", selectedLayerId_ });
                }
                if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
                    newCurrentHovered = char(lid);

                style.Colors[ImGuiCol_Button] = orig;
            }
        }
        currentHoveredLayer_ = newCurrentHovered;
        ImGui::EndListBox();
    }
    if(ImGui::Button("New layer")) {
        layers_.emplace_back();
        selectedLayerId_ = char(layers_.size()) - 1;
        needsSaving_ = true;
    }

    auto saveCheck = [this](bool changed) {
        needsSaving_ = needsSaving_ || changed;
        return changed;
    };

    if(selectedLayerId_ != UnselectedSubId) {
        auto& editLayer = layers_[selectedLayerId_];
        if(!editLayer.name.empty())
            ImGuiTitle(std::format("Editing Cursor Layer '{}'", editLayer.name).c_str(), 0.75f);
        else
            ImGuiTitle("New Cursor Layer", 0.75f);

        saveCheck(ImGui::InputText("Name##NewLayer", &editLayer.name));
        saveCheck(ImGui::Combo("Type", (int*)&editLayer.type, "Circle\0Square\0Cross\0Gaussian"));

        if(editLayer.type != CursorType::SMOOTH)
            saveCheck(ImGui::ColorEdit4("Border Color", glm::value_ptr(editLayer.color1),
                                        ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs));

        saveCheck(
            ImGui::ColorEdit4("Fill Color", glm::value_ptr(editLayer.color2), ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs));
        saveCheck(ImGui::Checkbox("Invert Colors", &editLayer.invert));

        if(editLayer.type != CursorType::SMOOTH)
            saveCheck(ImGui::DragFloat("Border Thickness", &editLayer.edgeThickness, 0.05f, 0.f, 100.f));
        else
            saveCheck(ImGui::DragFloat("Falloff", &editLayer.edgeThickness, 0.01f, 0.f, 1.f));

        switch(editLayer.type) {
        case CursorType::CIRCLE:
        case CursorType::SQUARE:
            break;
        case CursorType::CROSS:
            saveCheck(ImGui::DragFloat("Cross Thickness", &editLayer.secondaryThickness, 0.05f, 1.f,
                                       std::min(editLayer.dims.x, editLayer.dims.y)));
            saveCheck(ImGui::DragFloat("Cross Angle", &editLayer.angle, 0.1f, 0.f, 360.f));
            break;
        default:;
        }

        if(saveCheck(ImGui::Checkbox("Full screen", &editLayer.fullscreen)))
            if(!editLayer.fullscreen)
                editLayer.dims = glm::vec2(32.f);

        if(!editLayer.fullscreen)
            saveCheck(ImGui::DragFloat2("Cursor Size", glm::value_ptr(editLayer.dims), 0.2f, 1.f, ImGui::GetIO().DisplaySize.x * 2.f));
    }

    ImGui::Separator();

    ImGuiKeybindInput(activateCursor_, currentEditedKeybind, "Toggles cursor layers visibility.");

    mstime currentTime = TimeInMilliseconds();
    if(needsSaving_ && lastSaveTime_ + SaveDelay <= currentTime)
        Save();
}

void Cursor::Delete(char id) {
    if(selectedLayerId_ == id)
        selectedLayerId_ = UnselectedSubId;
    else if(selectedLayerId_ > id)
        selectedLayerId_--;

    layers_.erase(layers_.begin() + id);

    needsSaving_ = true;
}

void Cursor::Load() {
    using namespace nlohmann;
    layers_.clear();
    selectedLayerId_ = UnselectedSubId;

    auto& cfg = JSONConfigurationFile::i();
    cfg.Reload();

    auto maybe_at = []<typename D>(const json& j, const char* n, const D& def,
                                   const std::variant<std::monostate, std::function<D(const json&)>>& cvt = {}) {
        auto it = j.find(n);
        if(it == j.end())
            return def;
        if(cvt.index() == 0)
            return static_cast<D>(*it);
        else
            return std::get<1>(cvt)(*it);
    };

    auto getvec4 = [](const json& j) {
        return glm::vec4(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>());
    };
    auto getvec2 = [](const json& j) {
        return glm::vec2(j[0].get<float>(), j[1].get<float>());
    };

    const auto& layers = cfg.json()["cursor_layers"];
    for(const auto& lIn : layers) {
        Layer l {};
        l.name = lIn["name"];
        l.color1 = maybe_at(lIn, "color1", glm::vec4(1.f), { getvec4 });
        l.color2 = maybe_at(lIn, "color2", glm::vec4(1.f), { getvec4 });
        l.invert = maybe_at(lIn, "invert", false);
        l.fullscreen = maybe_at(lIn, "fullscreen", false);
        if(!l.fullscreen)
            l.dims = maybe_at(lIn, "dims", glm::vec2(32.f), { getvec2 });
        l.edgeThickness = maybe_at(lIn, "edge_thickness", 1.f);
        l.secondaryThickness = maybe_at(lIn, "secondary_thickness", 4.f);
        l.angle = maybe_at(lIn, "angle", 0.f);
        l.type = CursorType(maybe_at(lIn, "type", 0));

        layers_.push_back(l);
    }
}

void Cursor::Save() {
    using namespace nlohmann;

    auto& cfg = JSONConfigurationFile::i();

    auto& layers = cfg.json()["cursor_layers"];
    layers = json::array();
    for(const auto& l : layers_) {
        json layer;
        layer["name"] = l.name;
        layer["color1"] = { l.color1.x, l.color1.y, l.color1.z, l.color1.w };
        layer["color2"] = { l.color2.x, l.color2.y, l.color2.z, l.color2.w };
        layer["invert"] = l.invert;
        if(!l.fullscreen)
            layer["dims"] = { l.dims.x, l.dims.y };
        layer["fullscreen"] = l.fullscreen;
        layer["edge_thickness"] = l.edgeThickness;
        layer["secondary_thickness"] = l.secondaryThickness;
        layer["angle"] = l.angle;
        layer["type"] = int(l.type);

        layers.push_back(layer);
    }

    cfg.Save();

    needsSaving_ = false;
    lastSaveTime_ = TimeInMilliseconds();
}

} // namespace GW2Clarity
