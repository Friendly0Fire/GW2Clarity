#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>

#include <Core.h>
#include <Grids.h>
#include <ImGuiExtensions.h>
#include <Styles.h>
#include <cppcodec/base64_rfc4648.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <range/v3/all.hpp>
#include <variant>

namespace GW2Clarity
{
Styles::Styles(ComPtr<ID3D11Device>& dev, const Buffs* buffs)
    : buffs_(buffs)
    , previewRenderer_(dev, buffs)
{
    Load();

    preview_ = MakeRenderTarget(dev, PreviewSize, PreviewSize, DXGI_FORMAT_R8G8B8A8_UNORM);

    SettingsMenu::i().AddImplementer(this);
}

Styles::~Styles()
{
    SettingsMenu::f([&](auto& i) { i.RemoveImplementer(this); });
}

void Styles::Delete(uint id)
{
    styles_.erase(styles_.begin() + id);
    selectedId_  = UnselectedId;
    needsSaving_ = true;
}

void Styles::DrawMenu(Keybind** currentEditedKeybind)
{
    drewMenu_      = true;
    auto saveCheck = [this](bool changed)
    {
        needsSaving_ = needsSaving_ || changed;
        return changed;
    };

    ImGuiTitle("Styles");

    if (ImGui::BeginListBox("##StylesList", ImVec2(-FLT_MIN, 0.f)))
    {
        for (auto it : styles_ | ranges::views::enumerate)
        {
            // Need explicit types to shut up IntelliSense, still present as of 17.2.6
            uint   sid = static_cast<uint>(it.first);
            Style& s   = it.second;

            if (ImGui::Selectable(s.name.c_str(), selectedId_ == sid, ImGuiSelectableFlags_AllowItemOverlap))
                selectedId_ = sid;

            if (sid != 0 && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) || ImGui::IsItemActive())
            {
                auto& style = ImGui::GetStyle();
                auto  orig  = style.Colors[ImGuiCol_Button];
                style.Colors[ImGuiCol_Button] *= ImVec4(0.5f, 0.5f, 0.5f, 1.f);
                if (ImGuiClose(std::format("CloseItem{}", sid).c_str(), 0.75f, false))
                {
                    selectedId_ = sid;
                    Core::i().DisplayDeletionMenu({ s.name, "style", "", selectedId_ });
                }

                style.Colors[ImGuiCol_Button] = orig;
            }
        }
        ImGui::EndListBox();

        if (ImGui::Button("Add"))
        {
            std::string name    = "New Style";
            int         nameIdx = 1;
            while (ranges::any_of(styles_, [&](const auto& s) { return s.name == name; }))
                name = std::format("New Style ({})", ++nameIdx);

            styles_.emplace_back(name);
            selectedId_ = uint(styles_.size()) - 1;
        }

        {
            ImGuiDisabler d(selectedId_ == UnselectedId);
            ImGui::SameLine();
            if (ImGui::Button(std::format("Duplicate{}", d.disabled() ? "" : std::format(" '{}'", styles_[selectedId_].name)).c_str()))
            {
                const auto& baseName = styles_[selectedId_].name;
                std::string name;
                int         nameIdx = 1;
                do
                    name = std::format("{} ({})", baseName, ++nameIdx);
                while (ranges::any_of(styles_, [&](const auto& s) { return s.name == name; }));

                styles_.push_back(styles_[selectedId_]);
                styles_.back().name = name;
                selectedId_         = uint(styles_.size()) - 1;
            }
        }
    }

    if (selectedId_ < styles_.size())
    {
        auto& s = styles_.at(selectedId_);

        {
            ImGuiDisabler disable(selectedId_ == 0);
            ImGui::InputText("Name", &s.name);
        }
        if (selectedId_ == 0)
        {
            ImGui::TextUnformatted("(cannot rename default style)");
        }

        ImGui::NewLine();

        ImGui::TextUnformatted("Preview style with ");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100.f);
        ImGui::DragInt("stacks##SimulatedStacks", &editingItemFakeCount_, 0.1f, 0, 100, "%d", ImGuiSliderFlags_AlwaysClamp);
        ImGuiHelpTooltip("Helper feature to tune styles. Will simulate what the buff icon would look like with the given number of stacks.");
        ImGui::SameLine();
        ImGui::TextUnformatted(" of buff ");
        buffs_->DrawBuffCombo("##PreviewBuff", previewBuff_, buffSearch_);
        ImGui::Separator();

        ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x) * 0.5f - PreviewSize * 0.25f);
        ImGui::Image(preview_.srv.Get(), ImVec2(PreviewSize, PreviewSize) * 0.5f, ImVec2(0.f, 0.f), ImVec2(1.f, 1.f), ImVec4(1.f, 1.f, 1.f, 1.f), ImVec4(1.f, 1.f, 1.f, 1.f));

        ImGui::Separator();

        if (ImGuiBeginTimeline("Range", 26, ImGui::CalcTextSize("99-99").x))
        {
            for (size_t i = 0; i < s.thresholds.size(); i++)
            {
                auto&           th = s.thresholds[i];
                ImTimelineRange r{ int(th.thresholdMin), int(th.thresholdMax) };
                const auto      name     = th.thresholdMin == th.thresholdMax ? std::format("{}", th.thresholdMin) : std::format("{}-{}", th.thresholdMin, th.thresholdMax);
                auto [changed, selected] = ImGuiTimelineEvent(std::format("{}", i).c_str(), name.c_str(), r, selectedThresholdId_ == i);
                if (changed)
                {
                    th.thresholdMin = r[0];
                    th.thresholdMax = r[1];
                    needsSaving_    = true;
                }

                if (selected)
                    selectedThresholdId_ = int(i);
            }
        }
        int lines[] = { 0, 1, 5, 10, 15, 20, 25 };
        ImGuiEndTimeline(std::size(lines), lines);

        if (saveCheck(ImGui::Button("Add range")))
        {
            s.thresholds.emplace_back();
        }

        if (selectedThresholdId_ != UnselectedId)
        {
            auto& th = s.thresholds[selectedThresholdId_];
            if (th.thresholdMin == th.thresholdMax)
                ImGui::TextUnformatted(std::format("At {} stacks:", th.thresholdMin).c_str());
            else
                ImGui::TextUnformatted(std::format("Between {} and {} stacks:", th.thresholdMin, th.thresholdMax).c_str());
            auto& app = th.appearance;

            ImGui::TextUnformatted("Priority control:");
            ImGui::SameLine();
            if (selectedThresholdId_ > 0)
            {
                if (ImGui::Button("Move up"))
                {
                    std::swap(s.thresholds[selectedThresholdId_], s.thresholds[selectedThresholdId_ - 1]);
                    selectedThresholdId_--;
                }

                ImGui::SameLine();
            }

            if (selectedThresholdId_ < s.thresholds.size() - 1)
            {
                if (ImGui::Button("Move down"))
                {
                    std::swap(s.thresholds[selectedThresholdId_], s.thresholds[selectedThresholdId_ + 1]);
                    selectedThresholdId_++;
                }
            }

            ImGuiHelpTooltip("If more than one range is defined for the same stack count, the highest priority range (first in the list) will be used.");

            saveCheck(ImGui::ColorEdit4("Tint Color", &app.tint.x, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs));
            saveCheck(ImGui::DragFloat("Glow Size", &app.glowSize, 0.01f, 0.f, 10.f));
            if (app.glowSize > 0.f)
            {
                saveCheck(ImGui::ColorEdit4("Glow Color", &app.glow.x, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs));
                saveCheck(ImGui::DragFloat("Glow Pulse Intensity", &app.glowPulse.x, 0.01f, 0.f, 1.f));
                saveCheck(ImGui::DragFloat("Glow Pulse Speed", &app.glowPulse.y, 0.01f, 0.f, 5.f));
            }
            saveCheck(ImGui::DragFloat("Border Thickness", &app.borderThickness, 0.1f, 0.f, 512.f));
            if (app.borderThickness > 0.f)
                saveCheck(ImGui::ColorEdit4("Border Color", &app.border.x, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs));

            if (ImGui::Button("Delete selected"))
            {
                s.thresholds.erase(s.thresholds.begin() + selectedThresholdId_);
                selectedThresholdId_ = UnselectedId;
            }
        }
    }

    mstime currentTime = TimeInMilliseconds();
    if (needsSaving_ && lastSaveTime_ + SaveDelay <= currentTime)
        Save();
}

void Styles::Draw(ComPtr<ID3D11DeviceContext>& ctx)
{
    if (selectedId_ == UnselectedId || !drewMenu_ || !previewBuff_)
        return;

    GridInstanceData data{
        .posDims    = {0.5f, 0.5f, 1.f, 1.f},
        .uv         = previewBuff_->uv,
        .numberUV   = buffs_->GetNumber(editingItemFakeCount_),
        .showNumber = previewBuff_->ShowNumber(editingItemFakeCount_)
    };
    ApplyStyle(selectedId_, editingItemFakeCount_, data);

    float clear[4] = { 0.f, 0.f, 0.f, 1.f };
    ctx->ClearRenderTargetView(preview_.rtv.Get(), clear);

    previewRenderer_.Add(std::move(data));
    previewRenderer_.Draw(ctx, true, &preview_, false);

    drewMenu_ = false;
}

void Styles::Load()
{
    using namespace nlohmann;
    styles_.clear();

    auto& cfg = JSONConfigurationFile::i();
    cfg.Reload();

    auto getvec4  = [](const json& j) { return glm::vec4(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>()); };
    auto getvec2  = [](const json& j) { return glm::vec2(j[0].get<float>(), j[1].get<float>()); };
    auto maybe_at = []<typename D>(const json& j, const char* n, const D& def, const std::variant<std::monostate, std::function<D(const json&)>>& cvt = {})
    {
        auto it = j.find(n);
        if (it == j.end())
            return def;
        if (cvt.index() == 0)
            return static_cast<D>(*it);
        return std::get<1>(cvt)(*it);
    };

    const auto& sets = cfg.json()["styles"];
    for (const auto& sIn : sets)
    {
        Style s{};
        s.name = sIn["name"];

        for (const auto& tIn : sIn["thresholds"])
        {
            Threshold t;
            t.thresholdMin      = maybe_at(tIn, "threshold_min", 0);
            t.thresholdMax      = maybe_at(tIn, "threshold_max", 1);
            auto& app           = t.appearance;
            app.tint            = maybe_at(tIn, "tint", glm::vec4(1), { getvec4 });
            app.border          = maybe_at(tIn, "border", glm::vec4(0), { getvec4 });
            app.glow            = maybe_at(tIn, "glow", glm::vec4(0), { getvec4 });
            app.borderThickness = maybe_at(tIn, "border_thickness", 0.f);
            app.glowSize        = maybe_at(tIn, "glow_size", 0.f);
            app.glowPulse       = maybe_at(tIn, "glow_pulse", glm::vec2(0), { getvec2 });
            s.thresholds.push_back(t);
        }

        styles_.push_back(s);
    }

    if (styles_.empty() || styles_[0].name != "Default")
    {
        Style s;
        if (auto it = ranges::find_if(styles_, [](const auto& s) { return s.name == "Default"; }); it != styles_.end())
        {
            s = *it;
            styles_.erase(it);
        }
        else
        {
            s.name = "Default";
            s.thresholds.emplace_back();
        }

        styles_.insert(styles_.begin(), s);
    }

    BuildCache();
}

void Styles::Save()
{
    using namespace nlohmann;

    auto& cfg    = JSONConfigurationFile::i();

    auto& styles = cfg.json()["styles"];
    styles       = json::array();
    for (const auto& s : styles_)
    {
        json style;
        style["name"]         = s.name;

        json& styleThresholds = style["thresholds"];
        for (const auto& t : s.thresholds)
        {
            json threshold;
            threshold["threshold_min"]    = t.thresholdMin;
            threshold["threshold_max"]    = t.thresholdMax;
            auto& app                     = t.appearance;
            threshold["tint"]             = { app.tint.x, app.tint.y, app.tint.z, app.tint.w };
            threshold["border"]           = { app.border.x, app.border.y, app.border.z, app.border.w };
            threshold["glow"]             = { app.glow.x, app.glow.y, app.glow.z, app.glow.w };
            threshold["border_thickness"] = app.borderThickness;
            threshold["glow_size"]        = app.glowSize;
            threshold["glow_pulse"]       = { app.glowPulse.x, app.glowPulse.y };

            styleThresholds.push_back(threshold);
        }

        styles.push_back(style);
    }

    cfg.Save();

    needsSaving_  = false;
    lastSaveTime_ = TimeInMilliseconds();

    BuildCache();
}

void Styles::BuildCache()
{
    for (auto& s : styles_)
    {
        ranges::fill(s.appearanceCache, Appearance{});

        // Reverse iteration order so low priority appearance is set first and overwritten by high priority ones
        for (const auto& th : s.thresholds | ranges::views::reverse)
            ranges::fill(s.appearanceCache.begin() + th.thresholdMin, s.appearanceCache.begin() + std::min(th.thresholdMax + 1, uint(s.appearanceCache.size())), th.appearance);

        // Normal iteration order to find first threshold at 100, if any
        for (const auto& th : s.thresholds)
            if (th.thresholdMax >= 100)
            {
                s.appearanceAbove = th.appearance;
                break;
            }
    }
}

void Styles::ApplyStyle(uint id, int count, GridInstanceData& out) const
{
    if (id >= styles_.size() || count < 0)
        return;

    const auto& style = styles_.at(id);
    if (auto appPair = style[count]; appPair.first)
    {
        const auto& app = appPair.second;
        if (app.glowPulse.x > 0.f)
        {
            auto  currentTime = TimeInMilliseconds();
            float x           = sinf(static_cast<float>(currentTime) / 1000.f * 2.f * static_cast<float>(M_PI) * app.glowPulse.y) * 0.5f + 0.5f;
            out.glowSize.x    = glm::mix(1.f - app.glowPulse.x, 1.f, x) * app.glowSize;
            out.glowSize.y    = app.glowSize;
        }
        else
            out.glowSize = glm::vec2(app.glowSize);
        out.borderColor     = app.border;
        out.borderThickness = app.borderThickness;
        out.glowColor       = app.glowSize > 0.f ? app.glow : glm::vec4(0.f);
        out.tint            = app.tint;
    }
}
} // namespace GW2Clarity
