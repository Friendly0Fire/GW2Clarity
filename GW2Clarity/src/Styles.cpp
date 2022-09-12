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

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) || ImGui::IsItemActive())
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
                name = std::format("New Style {}", ++nameIdx);

            styles_.emplace_back(name);
            selectedId_ = styles_.size() - 1;
        }
    }

    if (selectedId_ < styles_.size())
    {
        auto& s = styles_.at(selectedId_);

        ImGui::InputText("Name", &s.name);

        ImGui::NewLine();

        ImGui::TextUnformatted("Visualize style with ");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100.f);
        ImGui::DragInt("stacks##SimulatedStacks", &editingItemFakeCount_, 0.1f, 0, 100, "%d", ImGuiSliderFlags_AlwaysClamp);
        ImGuiHelpTooltip("Helper feature to tune styles. Will simulate what the buff icon would look like with the given number of stacks.");
        ImGui::SameLine();
        ImGui::TextUnformatted(" of buff ");
        buffs_->DrawBuffCombo("##PreviewBuff", previewBuff_, buffSearch_);
        ImGui::Separator();

        ImGui::Image(preview_.srv.Get(), ImVec2(PreviewSize, PreviewSize));

        ImGui::Separator();

        for (size_t i = 0; i < s.thresholds.size(); i++)
        {
            auto& th = s.thresholds[i];
            ImGui::TextUnformatted("When less than ");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100.f);
            saveCheck(ImGui::DragInt(std::format("##Threshold{}", i).c_str(), (int*)&th.threshold, 0.1f, 0, 100, "%d", ImGuiSliderFlags_AlwaysClamp));
            ImGui::SameLine();
            ImGui::TextUnformatted("stacks:");

            ImGui::Indent();

            saveCheck(ImGui::ColorEdit4(std::format("Tint Color##{}", i).c_str(), &th.tint.x, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs));
            saveCheck(ImGui::DragFloat(std::format("Glow Size##{}", i).c_str(), &th.glowSize, 0.01f, 0.f, 10.f));
            if (th.glowSize > 0.f)
            {
                saveCheck(ImGui::ColorEdit4(std::format("Glow Color##{}", i).c_str(), &th.glow.x, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs));
                saveCheck(ImGui::DragFloat(std::format("Glow Pulse Intensity##{}", i).c_str(), &th.glowPulse.x, 0.01f, 0.f, 1.f));
                saveCheck(ImGui::DragFloat(std::format("Glow Pulse Speed##{}", i).c_str(), &th.glowPulse.y, 0.01f, 0.f, 5.f));
            }
            saveCheck(ImGui::DragFloat(std::format("Border Thickness##{}", i).c_str(), &th.borderThickness, 0.1f, 0.f, 512.f));
            if (th.borderThickness > 0.f)
                saveCheck(ImGui::ColorEdit4(std::format("Border Color##{}", i).c_str(), &th.border.x, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs));

            ImGui::Unindent();

            ImGui::Separator();
        }
        if (saveCheck(ImGui::Button("Add threshold")))
            s.thresholds.emplace_back();

        if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            ranges::sort(s.thresholds, [](auto& a, auto& b) { return a.threshold < b.threshold; });
    }

    mstime currentTime = TimeInMilliseconds();
    if (needsSaving_ && lastSaveTime_ + SaveDelay <= currentTime)
        Save();
}

void Styles::Draw(ComPtr<ID3D11DeviceContext>& ctx)
{
    if (selectedId_ == UnselectedSubId || !drewMenu_ || !previewBuff_)
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
            t.threshold       = maybe_at(tIn, "threshold", 1);
            t.tint            = maybe_at(tIn, "tint", glm::vec4(1), { getvec4 });
            t.border          = maybe_at(tIn, "border", glm::vec4(0), { getvec4 });
            t.glow            = maybe_at(tIn, "glow", glm::vec4(0), { getvec4 });
            t.borderThickness = maybe_at(tIn, "border_thickness", 0.f);
            t.glowSize        = maybe_at(tIn, "glow_size", 0.f);
            t.glowPulse       = maybe_at(tIn, "glow_pulse", glm::vec2(0), { getvec2 });
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
            threshold["threshold"]        = t.threshold;
            threshold["tint"]             = { t.tint.x, t.tint.y, t.tint.z, t.tint.w };
            threshold["border"]           = { t.border.x, t.border.y, t.border.z, t.border.w };
            threshold["glow"]             = { t.glow.x, t.glow.y, t.glow.z, t.glow.w };
            threshold["border_thickness"] = t.borderThickness;
            threshold["glow_size"]        = t.glowSize;
            threshold["glow_pulse"]       = { t.glowPulse.x, t.glowPulse.y };

            styleThresholds.push_back(threshold);
        }

        styles.push_back(style);
    }

    cfg.Save();

    needsSaving_  = false;
    lastSaveTime_ = TimeInMilliseconds();
}

void Styles::ApplyStyle(uint id, int count, GridInstanceData& out) const
{
    if (id >= styles_.size())
        return ApplyStyle(0, count, out);

    const auto& style = styles_.at(id);
    if (auto threshIt = ranges::find_if(style.thresholds, [=](const auto& t) { return count < t.threshold; }); threshIt != style.thresholds.end())
    {
        const auto& thresh = *threshIt;

        if (thresh.glowPulse.x > 0.f)
        {
            auto  currentTime = TimeInMilliseconds();
            float x           = sinf(static_cast<float>(currentTime) / 1000.f * 2.f * static_cast<float>(M_PI) * thresh.glowPulse.y) * 0.5f + 0.5f;
            out.glowSize.x    = glm::mix(1.f - thresh.glowPulse.x, 1.f, x) * thresh.glowSize;
            out.glowSize.y    = thresh.glowSize;
        }
        else
            out.glowSize = glm::vec2(thresh.glowSize);
        out.borderColor     = thresh.border;
        out.borderThickness = thresh.borderThickness;
        out.glowColor       = thresh.glowSize > 0.f ? thresh.glow : glm::vec4(0.f);
        out.tint            = thresh.tint;
    }
}
} // namespace GW2Clarity
