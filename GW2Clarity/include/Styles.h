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

    void Delete(uint id);

protected:
    void Load();
    void Save();
    void BuildCache();

public:
    struct Appearance
    {
        glm::vec4 tint { 1, 1, 1, 1 };
        glm::vec4 border { 0 };
        glm::vec4 glow { 0 };
        float borderThickness = 0.f;
        float glowSize = 0.f;
        glm::vec2 glowPulse { 0 };
    };

    struct Threshold
    {
        uint thresholdMin = 0;
        uint thresholdMax = 0;

        Appearance appearance {};
    };

    struct ThresholdBuilder
    {
        using S = ThresholdBuilder&;

        Threshold t {};

        operator Threshold() const { return t; }

        S min(uint min) {
            t.thresholdMin = min;
            return *this;
        }

        S max(uint max) {
            t.thresholdMax = max;
            return *this;
        }

        S tint(float rgb, float a) {
            t.appearance.tint = glm::vec4(rgb, rgb, rgb, a);
            return *this;
        }

        S tint(float r, float g, float b, float a) {
            t.appearance.tint = glm::vec4(r, g, b, a);
            return *this;
        }

        S border(float rgb, float a) {
            t.appearance.border = glm::vec4(rgb, rgb, rgb, a);
            return *this;
        }

        S border(float r, float g, float b, float a) {
            t.appearance.border = glm::vec4(r, g, b, a);
            return *this;
        }

        S glow(float rgb, float a) {
            t.appearance.glow = glm::vec4(rgb, rgb, rgb, a);
            return *this;
        }

        S glow(float r, float g, float b, float a) {
            t.appearance.glow = glm::vec4(r, g, b, a);
            return *this;
        }

        S borderThickness(float borderThickness) {
            t.appearance.borderThickness = borderThickness;
            return *this;
        }

        S glowSize(float glowSize) {
            t.appearance.glowSize = glowSize;
            return *this;
        }

        S glowPulse(const glm::vec2& glowPulse) {
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

        std::pair<bool, const Appearance&> operator[](int count) const {
            if(count < 0)
                return { false, DefaultAppearance };

            if(count < appearanceCache.size())
                return { true, appearanceCache[count] };

            return { true, appearanceAbove };
        }
    };

    [[nodiscard]] const auto& styles() const { return styles_; }

    [[nodiscard]] const Style& style(uint id) const { return id < styles_.size() ? styles_[id] : styles_[0]; }

    [[nodiscard]] uint FindStyle(const std::string& name) const {
        auto it = ranges::find_if(styles_, [&](const auto& s) { return s.name == name; });
        if(it != styles_.end())
            return uint(std::distance(styles_.begin(), it));
        else
            return 0;
    }

    void ApplyStyle(uint id, int count, GridInstanceData& out) const;

protected:
    static constexpr uint UnselectedId = std::numeric_limits<uint>::max();
    const Buffs* buffs_;
    std::vector<Style> styles_;
    uint selectedId_ = UnselectedId;
    int selectedThresholdId_ = UnselectedId;
    mstime lastPreviewChoiceTime_ = 0;
    const Buff* previewBuff_ = nullptr;
    int previewCount_ = 0;
    RenderTarget preview_;
    std::mt19937 previewRng_ { std::random_device {}() };
    GridRenderer<1> previewRenderer_;
    bool drewMenu_ = false;

    mstime lastSaveTime_ = 0;
    bool needsSaving_ = false;
    static inline constexpr mstime SaveDelay = 1000;
    static inline constexpr int PreviewSize = 512;
};
} // namespace GW2Clarity
