#pragma once

#include <imgui.h>

#include "ActivationKeybind.h"
#include "Graphics.h"
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

    void Delete(char id);

    const char* GetTabName() const override { return "Custom Cursor"; }

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

    i32 selectedId_ = UnselectedSubId;

    mstime lastSaveTime_ = 0;
    bool needsSaving_ = false;
    static inline constexpr mstime SaveDelay = 1000;

    ActivationKeybind activateCursor_;
    bool visible_ = false;
};
} // namespace GW2Clarity
