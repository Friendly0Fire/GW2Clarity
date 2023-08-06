#pragma once

#include <imgui.h>

#include "ActivationKeybind.h"
#include "Buffs.h"
#include "GridRenderer.h"
#include "Layouts.h"
#include "Main.h"
#include "SettingsMenu.h"
#include "Styles.h"

namespace GW2Clarity
{
class Grids : public SettingsMenu::Implementer
{
    using Style = Styles::Style;

public:
    Grids(ComPtr<ID3D11Device>& dev, const Buffs* buffs, const Styles* styles);
    Grids(const Grids&) = delete;
    Grids(Grids&&) = delete;
    Grids& operator=(const Grids&) = delete;
    Grids& operator=(Grids&&) = delete;
    virtual ~Grids();

    void Draw(ComPtr<ID3D11DeviceContext>& ctx, const Layouts::Layout* layout, bool shouldIgnoreLayout);
    void DrawMenu(Keybind** currentEditedKeybind) override;

    [[nodiscard]] const char* GetTabName() const override { return "Grids"; }

    void Delete(Id id);
    void StyleDeleted(u32 id);

protected:
    void Load();
    void Save();

    void DrawEditingGrid();
    void DrawGridList();
    void DrawItems(ComPtr<ID3D11DeviceContext>& ctx, const Layouts::Layout* layout, bool shouldIgnoreLayout);

    static inline constexpr ivec2 GridDefaultSpacing { 64, 64 };

    struct Item
    {
        ivec2 pos { 0, 0 };
        const Buff* buff = &Buffs::UnknownBuff;
        u32 style = 0;
        std::vector<const Buff*> additionalBuffs;
    };

public:
    struct Grid
    {
        std::string name { "New Grid" };
        ivec2 spacing = GridDefaultSpacing;
        ivec2 offset = {};
        f32 centralWeight = 0.f;
        ivec2 mouseClipMin { std::numeric_limits<i32>::max() };
        ivec2 mouseClipMax { std::numeric_limits<i32>::min() };
        bool trackMouseWhileHeld = true;
        std::vector<Item> items;
        bool attached = false;
        bool square = true;

        auto ComputeOrigin(const Grids& grids, bool editMode, const vec2& screen, const vec2& mouse) const {
            vec2 gridOrigin;
            if(!attached || (editMode && !grids.testMouseMode_))
                gridOrigin = screen * 0.5f + vec2(offset);
            else {
                if(!trackMouseWhileHeld && grids.holdingMouseButton_ != ScanCode::None)
                    gridOrigin = vec2 { grids.heldMousePos_.x, grids.heldMousePos_.y };
                else
                    gridOrigin = mouse;

                if(mouseClipMin.x != std::numeric_limits<i32>::max()) {
                    gridOrigin = glm::max(gridOrigin, vec2(mouseClipMin));
                    gridOrigin = glm::min(gridOrigin, vec2(mouseClipMax));
                }

                if(centralWeight > 0.f)
                    gridOrigin = glm::mix(gridOrigin, screen * 0.5f, centralWeight);
            }

            return gridOrigin;
        }
    };

    [[nodiscard]] const std::vector<Grid>& grids() const { return grids_; }

    [[nodiscard]] inline Grid& grid(Id id) {
        if(id.grid == UnselectedSubId)
            throw std::invalid_argument("unselected id");
        else
            return grids_[id.grid];
    }

    [[nodiscard]] inline Grid& grid() {
        if(selectedId_.grid == UnselectedSubId)
            throw std::invalid_argument("unselected id");
        else
            return grids_[selectedId_.grid];
    }

    [[nodiscard]] inline Item& item(Id id) {
        if(id.grid == UnselectedSubId || id.item == UnselectedSubId)
            throw std::invalid_argument("unselected id");
        else
            return grids_[id.grid].items[id.item];
    }

    [[nodiscard]] inline Item& item() {
        if(selectedId_.grid == UnselectedSubId || selectedId_.item == UnselectedSubId)
            throw std::invalid_argument("unselected id");
        else
            return grids_[selectedId_.grid].items[selectedId_.item];
    }

protected:
    const Buffs* buffs_;
    const Styles* styles_;
    GridRenderer<1024> gridRenderer_;
    Id currentHovered_ = Unselected();

    std::vector<Grid> grids_;

    Id selectedId_ = Unselected();

    i32 editingItemFakeCount_ = 1;

    bool draggingMouseBoundaries_ = false;
    bool testMouseMode_ = false;
    mstime lastSaveTime_ = 0;
    bool needsSaving_ = false;
    BuffComboBox selector_;
    static inline constexpr mstime SaveDelay = 1000;
    bool firstDraw_ = true;
    ScanCode holdingMouseButton_ = ScanCode::None;
    ImVec2 heldMousePos_ {};

    ConfigurationOption<bool> enableBetterFiltering_;

    static constexpr i32 InvisibleWindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs |
                                                ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollWithMouse;

#ifdef _DEBUG
    std::string debugGridFilter_;
#endif
};
} // namespace GW2Clarity
