#pragma once

#include <ActivationKeybind.h>
#include <Buffs.h>
#include <ConfigurationFile.h>
#include <Graphics.h>
#include <Main.h>
#include <Sets.h>
#include <SettingsMenu.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <include/GridRenderer.h>
#include <include/Styles.h>
#include <map>
#include <mutex>
#include <set>
#include <span>

namespace GW2Clarity
{
class Grids : public SettingsMenu::Implementer
{
    using Style = Styles::Style;

public:
    Grids(ComPtr<ID3D11Device>& dev, const Buffs* buffs, const Styles* styles);
    Grids(const Grids&)            = delete;
    Grids(Grids&&)                 = delete;
    Grids& operator=(const Grids&) = delete;
    Grids& operator=(Grids&&)      = delete;
    virtual ~Grids();

    void                      Draw(ComPtr<ID3D11DeviceContext>& ctx, const Sets::Set* set, bool shouldIgnoreSet);
    void                      DrawMenu(Keybind** currentEditedKeybind) override;

    [[nodiscard]] const char* GetTabName() const override
    {
        return "Grids";
    }

    void Delete(Id id);

protected:
    void                               Load();
    void                               Save();

    void                               DrawEditingGrid();
    void                               PlaceItem();
    void                               DrawGridList();
    void                               DrawItems(ComPtr<ID3D11DeviceContext>& ctx, const Sets::Set* set, bool shouldIgnoreSet);

    static inline constexpr glm::ivec2 GridDefaultSpacing{ 64, 64 };

    struct Item
    {
        glm::ivec2               pos{ 0, 0 };
        const Buff*              buff  = &Buffs::UnknownBuff;
        uint                     style = 0;
        std::vector<const Buff*> additionalBuffs;
    };

public:
    struct Grid
    {
        glm::ivec2        spacing       = GridDefaultSpacing;
        glm::ivec2        offset        = {};
        float             centralWeight = 0.f;
        glm::ivec2        mouseClipMin{ std::numeric_limits<int>::max() };
        glm::ivec2        mouseClipMax{ std::numeric_limits<int>::min() };
        bool              trackMouseWhileHeld = true;
        std::vector<Item> items;
        std::string       name{ "New Grid" };
        bool              attached = false;
        bool              square   = true;
    };

    [[nodiscard]] const std::vector<Grid>& grids() const
    {
        return grids_;
    }

    [[nodiscard]] inline Grid& getG(const Id& id)
    {
        if (id.grid == UnselectedSubId)
            throw std::invalid_argument("unselected id");
        else if (id.grid == NewSubId)
            return creatingGrid_;
        else
            return grids_[id.grid];
    }

    [[nodiscard]] inline Item& getI(const Id& id)
    {
        if (id.grid == UnselectedSubId || id.item == UnselectedSubId)
            throw std::invalid_argument("unselected id");
        else if (id.item == NewSubId)
            return creatingItem_;
        else
            return grids_[id.grid].items[id.item];
    }

protected:
    const Buffs*                   buffs_;
    const Styles*                  styles_;
    GridRenderer<1024>             gridRenderer_;
    Grid                           creatingGrid_;
    Item                           creatingItem_;
    Id                             currentHovered_ = Unselected();

    std::vector<Grid>              grids_;

    Id                             selectedId_           = Unselected();

    int                            editingItemFakeCount_ = 1;

    bool                           draggingGridScale_ = false, draggingMouseBoundaries_ = false;
    bool                           testMouseMode_ = false;
    bool                           placingItem_   = false;
    mstime                         lastSaveTime_  = 0;
    bool                           needsSaving_   = false;
    static inline constexpr mstime SaveDelay      = 1000;
    char                           buffSearch_[512];
    bool                           firstDraw_          = true;
    ScanCode                       holdingMouseButton_ = ScanCode::NONE;
    ImVec2                         heldMousePos_{};

    ConfigurationOption<bool>      enableBetterFiltering_;

    static constexpr int           InvisibleWindowFlags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollWithMouse;

#ifdef _DEBUG
    std::string debugGridFilter_;
#endif
};
} // namespace GW2Clarity