#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>

#include <Core.h>
#include <ImGuiExtensions.h>
#include <Sets.h>
#include <SimpleIni.h>
#include <algorithm>
#include <cppcodec/base64_rfc4648.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <range/v3/all.hpp>
#include <shellapi.h>
#include <skyr/percent_encoding/percent_encode.hpp>
#include <variant>

namespace GW2Clarity
{

Sets::Sets(ComPtr<ID3D11Device>& dev, Grids* grids)
    : changeGridSetKey_("change_grid_set", "Change Grid Set", "General")
    , rememberSet_("Remember Set on launch", "remember_set", "General", false)
    , rememberedSetId_("Remembered set", "remembered_set", "General", UnselectedSubId)
    , gridsInstance_(grids)
{
    Load(gridsInstance_->grids().size());

    if (rememberSet_.value())
        currentSetId_ = rememberedSetId_.value();

    changeGridSetKey_.callback(
        [&](Activated a)
        {
            if (a && !showSetSelector_)
            {
                ImGui::SetWindowPos(ChangeSetPopupName, ImGui::GetIO().MousePos);
                showSetSelector_ = true;
                return true;
            }
            else
                return false;
        });

    SettingsMenu::i().AddImplementer(this);
}

Sets::~Sets()
{
    SettingsMenu::f([&](auto& i) { i.RemoveImplementer(this); });
}

void Sets::Delete(short id)
{
    sets_.erase(sets_.begin() + selectedSetId_);
    selectedSetId_ = UnselectedSubId;
    needsSaving_   = true;
}

void Sets::GridDeleted(Id id)
{
    for (auto& s : sets_)
    {
        std::vector<int> toadd;

        s.grids.erase(id.grid);
        for (auto it = s.grids.begin(); it != s.grids.end();)
        {
            int gid = *it;
            if (gid > id.grid)
            {
                it = s.grids.erase(it);
                toadd.push_back(gid - 1);
            }
            else
                ++it;
        }

        for (int i : toadd)
            s.grids.insert(i);
    }

    needsSaving_ = true;
}

void Sets::DrawMenu(Keybind** currentEditedKeybind)
{
    if (enableDefaultSet())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, 0xFF0000FF);
        ImGui::TextWrapped("Notice: No Set defined! All Grids will be displayed by default at all times unless a Set is created.");
        ImGui::PopStyleColor();
    }
    auto saveCheck = [this](bool changed)
    {
        needsSaving_ = needsSaving_ || changed;
        return changed;
    };

    if (ImGui::BeginListBox("##SetsList", ImVec2(-FLT_MIN, 0.f)))
    {
        short newCurrentHovered = currentHoveredSet_;
        for (auto&& [sid, s] : sets_ | ranges::views::enumerate)
        {
            if (ImGui::Selectable(std::format("{}##Set", s.name).c_str(), selectedSetId_ == sid || currentHoveredSet_ == sid, ImGuiSelectableFlags_AllowItemOverlap))
            {
                selectedSetId_ = short(sid);
            }

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) || ImGui::IsItemActive())
            {
                auto& style = ImGui::GetStyle();
                auto  orig  = style.Colors[ImGuiCol_Button];
                style.Colors[ImGuiCol_Button] *= ImVec4(0.5f, 0.5f, 0.5f, 1.f);
                if (ImGuiClose(std::format("CloseSet{}", sid).c_str(), 0.75f, false))
                {
                    selectedSetId_ = short(sid);
                    Core::i().DisplayDeletionMenu({ s.name, "set", "", selectedSetId_ });
                }
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
                    newCurrentHovered = short(sid);

                style.Colors[ImGuiCol_Button] = orig;
            }
        }
        currentHoveredSet_ = newCurrentHovered;
        ImGui::EndListBox();
    }
    if (ImGui::Button("New set"))
    {
        selectedSetId_ = NewSubId;
    }

    bool editingSet = selectedSetId_ != NewSubId;
    if (selectedSetId_ != UnselectedSubId)
    {
        auto& editSet = selectedSetId_ == NewSubId ? creatingSet_ : sets_[selectedSetId_];
        if (editingSet)
            ImGuiTitle(std::format("Editing Set '{}'", editSet.name).c_str(), 0.75f);
        else
            ImGuiTitle("New Set", 0.75f);

        saveCheck(ImGui::InputText("Name##NewSet", &editSet.name));
        saveCheck(ImGui::Checkbox("Show in Combat Only##NewSet", &editSet.combatOnly));

        ImGui::Separator();

        for (auto&& [gid, g] : gridsInstance_->grids() | ranges::views::enumerate)
        {
            bool sel = editSet.grids.count(int(gid)) > 0;
            if (saveCheck(ImGui::Checkbox(std::format("{}##GridInSet", g.name).c_str(), &sel)))
            {
                if (sel)
                    editSet.grids.insert(int(gid));
                else
                    editSet.grids.erase(int(gid));
            }
        }

        ImGui::PushFont(Core::i().fontBold());
        if (selectedSetId_ == NewSubId && ImGui::Button("Create Set"))
        {
            sets_.push_back(creatingSet_);
            creatingSet_   = {};
            selectedSetId_ = UnselectedSubId;
            needsSaving_   = true;
        }
        else if (selectedSetId_ >= 0 && ImGui::Button("Delete Set"))
            ImGui::OpenPopup("Confirm Deletion");
        ImGui::PopFont();
    }

    ImGuiKeybindInput(changeGridSetKey_, currentEditedKeybind, "Displays the Quick Set menu to change what buffs are displayed.");

    ImGuiConfigurationWrapper(&ImGui::Checkbox, rememberSet_);
    ImGuiHelpTooltip("If checked, the currently selected set will be saved on exit and restored on launch.");

    mstime currentTime = TimeInMilliseconds();
    if (needsSaving_ && lastSaveTime_ + SaveDelay <= currentTime)
        Save();
}

void Sets::Draw(ComPtr<ID3D11DeviceContext>& ctx)
{
    if (!SettingsMenu::i().isVisible())
    {
        selectedSetId_ = UnselectedSubId;
    }

    if (showSetSelector_ || firstDraw_)
    {
        if (ImGui::Begin(ChangeSetPopupName, nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
        {
            if (sets_.empty())
                showSetSelector_ = false;

            if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && ImGui::IsMouseDown(ImGuiMouseButton_Left))
                showSetSelector_ = false;

            for (auto&& [i, s] : sets_ | ranges::views::enumerate)
            {
                if (ImGui::Selectable(s.name.c_str(), currentSetId_ == i))
                {
                    currentSetId_ = short(i);
                    rememberedSetId_.value(currentSetId_);
                    showSetSelector_ = false;
                    needsSaving_ = true;
                }
            }

            if (ImGui::Selectable("None", currentSetId_ == UnselectedSubId))
            {
                currentSetId_ = UnselectedSubId;
                rememberedSetId_.value(currentSetId_);
                showSetSelector_ = false;
                needsSaving_ = true;
            }

            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                showSetSelector_ = false;

            mstime currentTime = TimeInMilliseconds();
            if (needsSaving_ && lastSaveTime_ + SaveDelay <= currentTime)
                Save();
        }
        ImGui::End();
    }

    firstDraw_ = false;
}

void Sets::Load(size_t gridCount)
{
    using namespace nlohmann;
    sets_.clear();
    selectedSetId_ = UnselectedSubId;

    auto& cfg      = JSONConfigurationFile::i();
    cfg.Reload();

    auto maybe_at = []<typename D>(const json& j, const char* n, const D& def, const std::variant<std::monostate, std::function<D(const json&)>>& cvt = {})
    {
        auto it = j.find(n);
        if (it == j.end())
            return def;
        if (cvt.index() == 0)
            return static_cast<D>(*it);
        else
            return std::get<1>(cvt)(*it);
    };

    auto        getivec2  = [](const json& j) { return glm::ivec2(j[0].get<int>(), j[1].get<int>()); };
    auto        getImVec4 = [](const json& j) { return ImVec4(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>()); };

    const auto& sets      = cfg.json()["buff_sets"];
    for (const auto& sIn : sets)
    {
        Set s{};
        s.name       = sIn["name"];
        s.combatOnly = maybe_at(sIn, "combat_only", false);

        for (const auto& gIn : sIn["grids"])
        {
            int id = gIn;
            if (id < gridCount)
                s.grids.insert(id);
        }

        sets_.push_back(s);
    }

    short currentSet = maybe_at(cfg.json(), "current_set", UnselectedSubId);
    currentSetId_ = currentSet;
    rememberedSetId_.value(currentSet);
}

void Sets::Save()
{
    using namespace nlohmann;

    auto& cfg  = JSONConfigurationFile::i();

    auto& sets = cfg.json()["buff_sets"];
    sets       = json::array();
    for (const auto& s : sets_)
    {
        json set;
        set["name"]        = s.name;
        set["combat_only"] = s.combatOnly;

        json& setGrids     = set["grids"];

        for (int i : s.grids)
            setGrids.push_back(i);

        sets.push_back(set);
    }

    cfg.json()["current_set"] = currentSetId_;

    cfg.Save();

    needsSaving_  = false;
    lastSaveTime_ = TimeInMilliseconds();
}

} // namespace GW2Clarity