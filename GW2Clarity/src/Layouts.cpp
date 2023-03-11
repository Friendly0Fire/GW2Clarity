#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>

#include <Core.h>
#include <ImGuiExtensions.h>
#include <Layouts.h>
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

Layouts::Layouts(ComPtr<ID3D11Device>& dev, Grids* grids)
    : changeGridLayoutKey_("change_layout", "Change Layout", "General")
    , rememberLayout_("Remember Layout on launch", "remember_layout", "General", false)
    , rememberedLayoutId_("Remembered Layout", "remembered_layout", "General", UnselectedSubId)
    , gridsInstance_(grids)
{
    Load(gridsInstance_->grids().size());

    if (rememberLayout_.value())
        currentLayoutId_ = rememberedLayoutId_.value();

    changeGridLayoutKey_.callback(
        [&](Activated a)
        {
            if (a && !showLayoutSelector_)
            {
                ImGui::SetWindowPos(ChangeLayoutPopupName, ImGui::GetIO().MousePos);
                showLayoutSelector_ = true;
                return true;
            }
            else
                return false;
        });

    SettingsMenu::i().AddImplementer(this);
}

Layouts::~Layouts()
{
    SettingsMenu::f([&](auto& i) { i.RemoveImplementer(this); });
}

void Layouts::Delete(short id)
{
    layouts_.erase(layouts_.begin() + selectedLayoutId_);
    selectedLayoutId_ = UnselectedSubId;
    needsSaving_      = true;
}

void Layouts::GridDeleted(Id id)
{
    for (auto& s : layouts_)
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

void Layouts::DrawMenu(Keybind** currentEditedKeybind)
{
    if (enableDefaultLayout())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, 0xFF0000FF);
        ImGui::TextWrapped("Notice: No Layout defined! All Grids will be displayed by default at all times unless a Layout is created.");
        ImGui::PopStyleColor();
    }
    auto saveCheck = [this](bool changed)
    {
        needsSaving_ = needsSaving_ || changed;
        return changed;
    };

    if (ImGui::BeginListBox("##LayoutsList", ImVec2(-FLT_MIN, 0.f)))
    {
        short newCurrentHovered = currentHoveredLayout_;
        for (auto&& [sid, s] : layouts_ | ranges::views::enumerate)
        {
            if (ImGui::Selectable(std::format("{}##Layout", s.name).c_str(), selectedLayoutId_ == sid || currentHoveredLayout_ == sid, ImGuiSelectableFlags_AllowItemOverlap))
            {
                selectedLayoutId_ = short(sid);
            }

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) || ImGui::IsItemActive())
            {
                auto& style = ImGui::GetStyle();
                auto  orig  = style.Colors[ImGuiCol_Button];
                style.Colors[ImGuiCol_Button] *= ImVec4(0.5f, 0.5f, 0.5f, 1.f);
                if (ImGuiClose(std::format("CloseLayout{}", sid).c_str(), 0.75f, false))
                {
                    selectedLayoutId_ = short(sid);
                    Core::i().DisplayDeletionMenu({ s.name, "Layout", "", selectedLayoutId_ });
                }
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
                    newCurrentHovered = short(sid);

                style.Colors[ImGuiCol_Button] = orig;
            }
        }
        currentHoveredLayout_ = newCurrentHovered;
        ImGui::EndListBox();
    }
    if (ImGui::Button("New Layout"))
    {
        selectedLayoutId_ = NewSubId;
    }

    bool editingLayout = selectedLayoutId_ != NewSubId;
    if (selectedLayoutId_ != UnselectedSubId)
    {
        auto& editLayout = selectedLayoutId_ == NewSubId ? creatingLayout_ : layouts_[selectedLayoutId_];
        if (editingLayout)
            ImGuiTitle(std::format("Editing Layout '{}'", editLayout.name).c_str(), 0.75f);
        else
            ImGuiTitle("New Layout", 0.75f);

        saveCheck(ImGui::InputText("Name##NewLayout", &editLayout.name));
        saveCheck(ImGui::Checkbox("Show in Combat Only##NewLayout", &editLayout.combatOnly));

        ImGui::Separator();

        for (auto&& [gid, g] : gridsInstance_->grids() | ranges::views::enumerate)
        {
            bool sel = editLayout.grids.count(int(gid)) > 0;
            if (saveCheck(ImGui::Checkbox(std::format("{}##GridInLayout", g.name).c_str(), &sel)))
            {
                if (sel)
                    editLayout.grids.insert(int(gid));
                else
                    editLayout.grids.erase(int(gid));
            }
        }

        ImGui::PushFont(Core::i().fontBold());
        if (selectedLayoutId_ == NewSubId && ImGui::Button("Create Layout"))
        {
            layouts_.push_back(creatingLayout_);
            creatingLayout_   = {};
            selectedLayoutId_ = UnselectedSubId;
            needsSaving_      = true;
        }
        else if (selectedLayoutId_ >= 0 && ImGui::Button("Delete Layout"))
            ImGui::OpenPopup("Confirm Deletion");
        ImGui::PopFont();
    }

    ImGuiKeybindInput(changeGridLayoutKey_, currentEditedKeybind, "Displays the Quick Layout menu to change what buffs are displayed.");

    ImGuiConfigurationWrapper(&ImGui::Checkbox, rememberLayout_);
    ImGuiHelpTooltip("If checked, the currently selected Layout will be saved on exit and restored on launch.");

    mstime currentTime = TimeInMilliseconds();
    if (needsSaving_ && lastSaveTime_ + SaveDelay <= currentTime)
        Save();
}

void Layouts::Draw(ComPtr<ID3D11DeviceContext>& ctx)
{
    if (!SettingsMenu::i().isVisible())
    {
        selectedLayoutId_ = UnselectedSubId;
    }

    if (showLayoutSelector_ || firstDraw_)
    {
        if (ImGui::Begin(ChangeLayoutPopupName, nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
        {
            if (layouts_.empty())
                showLayoutSelector_ = false;

            if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && ImGui::IsMouseDown(ImGuiMouseButton_Left))
                showLayoutSelector_ = false;

            for (auto&& [i, s] : layouts_ | ranges::views::enumerate)
            {
                if (ImGui::Selectable(s.name.c_str(), currentLayoutId_ == i))
                {
                    currentLayoutId_ = short(i);
                    rememberedLayoutId_.value(currentLayoutId_);
                    showLayoutSelector_ = false;
                }
            }

            if (ImGui::Selectable("None", currentLayoutId_ == UnselectedSubId))
            {
                currentLayoutId_ = UnselectedSubId;
                rememberedLayoutId_.value(currentLayoutId_);
                showLayoutSelector_ = false;
            }

            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                showLayoutSelector_ = false;
        }
        ImGui::End();
    }

    firstDraw_ = false;
}

void Layouts::Load(size_t gridCount)
{
    using namespace nlohmann;
    layouts_.clear();
    selectedLayoutId_ = UnselectedSubId;

    auto& cfg         = JSONConfigurationFile::i();
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

    const auto& layouts   = cfg.json()["buff_layouts"];
    for (const auto& sIn : layouts)
    {
        Layout s{};
        s.name       = sIn["name"];
        s.combatOnly = maybe_at(sIn, "combat_only", false);

        for (const auto& gIn : sIn["grids"])
        {
            int id = gIn;
            if (id < gridCount)
                s.grids.insert(id);
        }

        layouts_.push_back(s);
    }
}

void Layouts::Save()
{
    using namespace nlohmann;

    auto& cfg     = JSONConfigurationFile::i();

    auto& layouts = cfg.json()["buff_layouts"];
    layouts       = json::array();
    for (const auto& s : layouts_)
    {
        json layout;
        layout["name"]        = s.name;
        layout["combat_only"] = s.combatOnly;

        json& layoutGrids     = layout["grids"];

        for (int i : s.grids)
            layoutGrids.push_back(i);

        layouts.push_back(layout);
    }

    cfg.Save();

    needsSaving_  = false;
    lastSaveTime_ = TimeInMilliseconds();
}

} // namespace GW2Clarity