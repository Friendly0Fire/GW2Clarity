#pragma once

#include <imgui.h>

#include "ActivationKeybind.h"
#include "Common.h"
#include "Graphics.h"
#include "ImGuiExtensions.h"
#include "Main.h"
#include "SettingsMenu.h"
#include "ShaderManager.h"

namespace GW2Clarity
{

class Cursor : public SettingsMenu::Implementer
{
public:
    explicit Cursor(ComPtr<ID3D11Device>& dev);
    ~Cursor() override;

    void Draw(ComPtr<ID3D11DeviceContext>& ctx);
    void DrawMenu(Keybind** currentEditedKeybind) override;

    const char* GetTabName() const override {
        return "Custom Cursor";
    }

    struct Layer
    {
        std::string name;

        vec4 colorFill { 1.f }, colorBorder { 1.f };
        float invertFill = 0.f, invertBorder = 0.f;
        vec2 dims { 32.f };
        f32 edgeThickness = 1.f;

        struct Circle
        { };
        struct Square
        {
            f32 angle = 0.f;
        };
        struct Cross
        {
            f32 angle = 0.f;
            f32 crossThickness = 4.f;
            bool fullscreen = false;
        };

        std::variant<Circle, Square, Cross> type;

        friend void to_json(nlohmann::json& j, const Layer& l) {
            j = nlohmann::json { { "name", l.name },
                                 { "color_fill", l.colorFill },
                                 { "color_border", l.colorBorder },
                                 { "invert_fill", l.invertFill },
                                 { "invert_border", l.invertBorder },
                                 { "dims", l.dims },
                                 { "edge_thickness", l.edgeThickness },
                                 { "type", l.type.index() } };
            std::visit(Overloaded { [&](const Circle&) {},
                                    [&](const Square& s) { j["angle"] = s.angle; },
                                    [&](const Cross& c) {
                                        j["angle"] = c.angle;
                                        j["cross_thickness"] = c.crossThickness;
                                        j["fullscreen"] = c.fullscreen;
                                    } },
                       l.type);
        }

        friend void from_json(const nlohmann::json& j, Layer& l) {
            glm::from_json(j, l.colorBorder);
            j.at("name").get_to(l.name);
            j.at("color_fill").get_to(l.colorFill);
            j.at("color_border").get_to(l.colorBorder);
            j.at("invert_fill").get_to(l.invertFill);
            j.at("invert_border").get_to(l.invertBorder);
            j.at("dims").get_to(l.dims);
            j.at("edge_thickness").get_to(l.edgeThickness);
            switch(j.at("type").get<u32>()) {
            default:
            case get_index<Circle, decltype(l.type)>():
                break;
            case get_index<Square, decltype(l.type)>():
                {
                    Square s;
                    j.at("angle").get_to(s.angle);
                    l.type = s;
                }
                break;
            case get_index<Cross, decltype(l.type)>():
                {
                    Cross c;
                    j.at("angle").get_to(c.angle);
                    j.at("cross_thickness").get_to(c.crossThickness);
                    j.at("fullscreen").get_to(c.fullscreen);
                    l.type = c;
                }
                break;
            }
        }
    };
    using LayerType = decltype(Layer::type);

protected:
    void Load();
    void Save();

    struct CursorData
    {
        vec4 dimensions;
        vec4 parameters;
        vec4 color1;
        vec4 color2;
    };
    ConstantBufferSPtr<CursorData> cursorCB_;
    ShaderId screenSpaceVS_;
    std::array<ShaderId, std::variant_size_v<LayerType>> cursorPS_;

    ComPtr<ID3D11BlendState> defaultBlend_, invertBlend_;

    std::vector<Layer> layers_;

    UI::SelectableListBox layerSelector_ { "Layer", "Cursor Layers", "Drag and drop layers to reorder. Layers are drawn top to bottom." };

    UI::SaveTracker save_;

    ActivationKeybind activateCursor_;
    bool visible_ = false;
};
} // namespace GW2Clarity
