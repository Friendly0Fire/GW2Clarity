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

class Sets : public SettingsMenu::Implementer
{
public:
    Sets(ComPtr<ID3D11Device>& dev, Grids* grids);
    virtual ~Sets();

    void        Draw(ComPtr<ID3D11DeviceContext>& ctx);
    void        DrawMenu(Keybind** currentEditedKeybind) override;

    const char* GetTabName() const override
    {
        return "Sets";
    }

    void Delete(short id);
    void GridDeleted(Id id);

protected:
    void Load(size_t gridCount);
    void Save();

public:
    struct Set
    {
        std::string   name;
        std::set<int> grids;
        bool          combatOnly = false;
    };

    const std::vector<Set>& sets() const
    {
        return sets_;
    }

    short currentSetId() const
    {
        return currentSetId_;
    }
    const Set* currentSet() const
    {
        return currentSetId_ >= 0 && currentSetId_ < sets_.size() ? &sets_[currentSetId_] : nullptr;
    }
    bool enableDefaultSet() const
    {
        return sets_.empty();
    }

protected:
    Set                            creatingSet_;
    short                          currentSetId_      = UnselectedSubId;
    short                          currentHoveredSet_ = UnselectedSubId;

    Grids*                         gridsInstance_     = nullptr;

    static inline const char*      ChangeSetPopupName = "QuickSet";

    std::vector<Set>               sets_;

    short                          selectedSetId_   = UnselectedSubId;
    mstime                         lastSaveTime_    = 0;
    bool                           needsSaving_     = false;
    static inline constexpr mstime SaveDelay        = 1000;
    bool                           showSetSelector_ = false;
    bool                           firstDraw_       = true;

    ActivationKeybind              changeGridSetKey_;
    ConfigurationOption<bool>      rememberSet_;
    ConfigurationOption<short>     rememberedSetId_;
};
} // namespace GW2Clarity