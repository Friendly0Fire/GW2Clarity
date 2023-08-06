#pragma once

#include <range/v3/all.hpp>

#include "ActivationKeybind.h"
#include "Buffs.h"
#include "ConfigurationFile.h"
#include "Graphics.h"
#include "GridRenderer.h"
#include "SettingsMenu.h"

namespace GW2Clarity
{

struct GridInstanceData;

class Styles : public SettingsMenu::Implementer
{
public:
    Styles(ComPtr<ID3D11Device>& dev, const Buffs* buffs);
    virtual ~Styles();

    void Draw(ComPtr<ID3D11DeviceContext>& ctx);
    void DrawMenu(Keybind** currentEditedKeybind) override;

    const char* GetTabName() const override { return "Styles"; }

    void Delete(u32 id);

protected:
    void Load();
    void Save();
    void BuildCache();

public:
    struct Appearance
    {
        vec4 tint { 1, 1, 1, 1 };
        vec4 border { 0 };
        vec4 glow { 0 };
        f32 borderThickness = 0.f;
        f32 glowSize = 0.f;
        vec2 glowPulse { 0 };
    };

    struct Threshold
    {
        u32 thresholdMin = 0;
        u32 thresholdMax = 0;

        Appearance appearance {};
    };

    struct ThresholdBuilder
    {
        using S = ThresholdBuilder&;

        Threshold t {};

        operator Threshold() const { return t; }

        S min(u32 min) {
            t.thresholdMin = min;
            return *this;
        }

        S max(u32 max) {
            t.thresholdMax = max;
            return *this;
        }

        S tint(f32 rgb, f32 a) {
            t.appearance.tint = vec4(rgb, rgb, rgb, a);
            return *this;
        }

        S tint(f32 r, f32 g, f32 b, f32 a) {
            t.appearance.tint = vec4(r, g, b, a);
            return *this;
        }

        S border(f32 rgb, f32 a) {
            t.appearance.border = vec4(rgb, rgb, rgb, a);
            return *this;
        }

        S border(f32 r, f32 g, f32 b, f32 a) {
            t.appearance.border = vec4(r, g, b, a);
            return *this;
        }

        S glow(f32 rgb, f32 a) {
            t.appearance.glow = vec4(rgb, rgb, rgb, a);
            return *this;
        }

        S glow(f32 r, f32 g, f32 b, f32 a) {
            t.appearance.glow = vec4(r, g, b, a);
            return *this;
        }

        S borderThickness(f32 borderThickness) {
            t.appearance.borderThickness = borderThickness;
            return *this;
        }

        S glowSize(f32 glowSize) {
            t.appearance.glowSize = glowSize;
            return *this;
        }

        S glowPulse(const vec2& glowPulse) {
            t.appearance.glowPulse = glowPulse;
            return *this;
        }

        auto build() const { return t; }
    };

    struct Style
    {
        inline static const std::string BuiltInPrefix = "[Default] ";

        Style() = default;
        Style(Style&&) = default;

        Style(const Style& s) : builtIn(false), thresholds(s.thresholds) {
            if(s.name.starts_with(BuiltInPrefix))
                name = s.name.substr(BuiltInPrefix.size());
            else
                name = s.name;
        }

        Style(std::string_view name) : name(name) { }

        Style(std::string_view name, auto&&... thresholds)
            : name(BuiltInPrefix + std::string(name)), thresholds(std::initializer_list<Threshold> { thresholds... }), builtIn(true) { }

        Style& operator=(const Style& s) {
            GW2_ASSERT(!builtIn);

            if(s.name.starts_with(BuiltInPrefix))
                name = s.name.substr(BuiltInPrefix.size());
            else
                name = s.name;

            thresholds = s.thresholds;

            return *this;
        }

        Style& operator=(Style&& s) {
            GW2_ASSERT(builtIn == s.builtIn);

            std::swap(name, s.name);
            std::swap(thresholds, s.thresholds);

            return *this;
        }

        std::string name;
        const bool builtIn = false;

        std::vector<Threshold> thresholds;

        std::array<Appearance, 100> appearanceCache;
        Appearance appearanceAbove;

        inline static constexpr Appearance DefaultAppearance {};

        std::pair<bool, const Appearance&> operator[](i32 count) const {
            if(count < 0)
                return { false, DefaultAppearance };

            if(count < appearanceCache.size())
                return { true, appearanceCache[count] };

            return { true, appearanceAbove };
        }
    };

    [[nodiscard]] const auto& styles() const { return styles_; }

    [[nodiscard]] const Style& style(u32 id) const { return id < styles_.size() ? styles_[id] : styles_[0]; }

    [[nodiscard]] u32 FindStyle(const std::string& name) const {
        auto it = ranges::find_if(styles_, [&](const auto& s) { return s.name == name; });
        if(it != styles_.end())
            return u32(std::distance(styles_.begin(), it));
        else
            return 0;
    }

    void ApplyStyle(u32 id, i32 count, GridInstanceData& out) const;

protected:
    static constexpr u32 UnselectedId = std::numeric_limits<u32>::max();
    const Buffs* buffs_;
    std::vector<Style> styles_;
    u32 selectedId_ = UnselectedId;
    i32 selectedThresholdId_ = UnselectedId;
    mstime lastPreviewChoiceTime_ = 0;
    const Buff* previewBuff_ = nullptr;
    i32 previewCount_ = 0;
    RenderTarget preview_;
    std::mt19937 previewRng_ { std::random_device {}() };
    GridRenderer<1> previewRenderer_;
    bool drewMenu_ = false;

    mstime lastSaveTime_ = 0;
    bool needsSaving_ = false;
    static inline constexpr mstime SaveDelay = 1000;
    static inline constexpr i32 PreviewSize = 512;
};
} // namespace GW2Clarity
