#pragma once

#include <ActivationKeybind.h>
#include <Buffs.h>
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

struct GridInstanceData;

class Styles : public SettingsMenu::Implementer
{
public:
    Styles(ComPtr<ID3D11Device>& dev, const Buffs* buffs);
    virtual ~Styles();

    void        Draw(ComPtr<ID3D11DeviceContext>& ctx);
    void        DrawMenu(Keybind** currentEditedKeybind) override;

    const char* GetTabName() const override
    {
        return "Styles";
    }

    void Delete(uint id);

protected:
    void Load();
    void Save();

public:
    struct Threshold
    {
        uint      threshold = 1;

        glm::vec4 tint{ 1, 1, 1, 1 };
        glm::vec4 border{ 0 };
        glm::vec4 glow{ 0 };
        float     borderThickness = 0.f;
        float     glowSize        = 0.f;
        glm::vec2 glowPulse{ 0 };
    };

    struct Style
    {
        std::string            name;
        std::vector<Threshold> thresholds;

        std::pair<int, int>    thresholdExtents(int i) const
        {
            int a = i == 0 ? -1 : int(thresholds[i - 1].threshold);
            int b = i == thresholds.size() - 1 ? std::numeric_limits<int>::max() : int(thresholds[i].threshold);

            return { a, b };
        }
    };

    [[nodiscard]] const auto& styles() const
    {
        return styles_;
    }

    [[nodiscard]] const Style& style(uint id) const
    {
        return id < styles_.size() ? styles_[id] : styles_[0];
    }

    [[nodiscard]] uint FindStyle(const std::string& name) const
    {
        auto it = ranges::find_if(styles_, [&](const auto& s) { return s.name == name; });
        if (it != styles_.end())
            return uint(std::distance(styles_.begin(), it));
        else
            return 0;
    }

    void ApplyStyle(uint id, int count, GridInstanceData& out) const;

protected:
    static constexpr uint          UnselectedId = std::numeric_limits<uint>::max();
    const Buffs*                   buffs_;
    std::vector<Style>             styles_;
    uint                           selectedId_           = UnselectedId;
    int                            editingItemFakeCount_ = 1;
    const Buff*                    previewBuff_          = nullptr;
    char                           buffSearch_[512];
    RenderTarget                   preview_;
    GridRenderer<1>                previewRenderer_;
    bool                           drewMenu_     = false;

    mstime                         lastSaveTime_ = 0;
    bool                           needsSaving_  = false;
    static inline constexpr mstime SaveDelay     = 1000;
    static inline constexpr int    PreviewSize   = 512;
};
} // namespace GW2Clarity