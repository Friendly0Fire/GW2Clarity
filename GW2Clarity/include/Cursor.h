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
    virtual ~Cursor();

    void Draw(ComPtr<ID3D11DeviceContext>& ctx);
    void DrawMenu(Keybind** currentEditedKeybind) override;

    void Delete(char id);

    const char* GetTabName() const override { return "Cursor"; }

    enum class CursorType : i32
    {
        CIRCLE = 0,
        SQUARE = 1,
        CROSS = 2,
        SMOOTH = 3,

        COUNT
    };

    struct Layer
    {
        std::string name;

        vec4 color1 { 1.f }, color2 { 1.f };
        bool invert = false;
        vec2 dims { 32.f };
        bool fullscreen = false;
        f32 edgeThickness = 1.f, secondaryThickness = 4.f, angle = 0.f;
        CursorType type = CursorType::CIRCLE;
    };

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
    std::array<ShaderId, size_t(CursorType::COUNT)> cursorPS_;

    ComPtr<ID3D11BlendState> defaultBlend_, invertBlend_;

    std::vector<Layer> layers_;

    char currentHoveredLayer_ = UnselectedSubId;
    char selectedLayerId_ = UnselectedSubId;

    mstime lastSaveTime_ = 0;
    bool needsSaving_ = false;
    static inline constexpr mstime SaveDelay = 1000;

    ActivationKeybind activateCursor_;
    bool visible_ = false;
};
} // namespace GW2Clarity
