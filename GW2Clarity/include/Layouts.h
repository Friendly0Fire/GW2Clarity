#pragma once

#include <ActivationKeybind.h>
#include <ConfigurationFile.h>
#include <Graphics.h>
#include <Main.h>
#include <SettingsMenu.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <map>
#include <mutex>
#include <set>
#include <span>

namespace GW2Clarity
{

class Grids;

class Layouts : public SettingsMenu::Implementer
{
public:
    Layouts(ComPtr<ID3D11Device>& dev, Grids* grids);
    virtual ~Layouts();

    void        Draw(ComPtr<ID3D11DeviceContext>& ctx);
    void        DrawMenu(Keybind** currentEditedKeybind) override;

    const char* GetTabName() const override
    {
        return "Layouts";
    }

    void Delete(short id);
    void GridDeleted(Id id);

protected:
    void Load(size_t gridCount);
    void Save();

public:
    struct Layout
    {
        std::string   name;
        std::set<int> grids;
        bool          combatOnly = false;
    };

    const std::vector<Layout>& sets() const
    {
        return layouts_;
    }

    short currentLayoutId() const
    {
        return currentLayoutId_;
    }
    const Layout* currentLayout() const
    {
        return currentLayoutId_ >= 0 && currentLayoutId_ < layouts_.size() ? &layouts_[currentLayoutId_] : nullptr;
    }
    bool enableDefaultLayout() const
    {
        return layouts_.empty();
    }

protected:
    short                          currentLayoutId_      = UnselectedSubId;
    short                          currentHoveredLayout_ = UnselectedSubId;

    Grids*                         gridsInstance_        = nullptr;

    static inline const char*      ChangeLayoutPopupName = "QuickLayout";

    std::vector<Layout>            layouts_;

    short                          selectedLayoutId_   = UnselectedSubId;
    mstime                         lastSaveTime_       = 0;
    bool                           needsSaving_        = false;
    static inline constexpr mstime SaveDelay           = 1000;
    bool                           showLayoutSelector_ = false;
    bool                           firstDraw_          = true;

    ActivationKeybind              changeGridLayoutKey_;
    ConfigurationOption<bool>      rememberLayout_;
    ConfigurationOption<short>     rememberedLayoutId_;
};
} // namespace GW2Clarity