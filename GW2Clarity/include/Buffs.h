#pragma once

#include <ActivationKeybind.h>
#include <ConfigurationFile.h>
#include <Graphics.h>
#include <Main.h>
#include <Sets.h>
#include <SettingsMenu.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <map>
#include <mutex>
#include <set>
#include <span>

namespace GW2Clarity
{
struct Buff
{
    uint               id;
    int                maxStacks;
    std::string        name;
    std::string        atlasEntry;
    glm::vec2          uv{};
    std::set<uint>     extraIds;
    std::string        category;

    static std::string NameToAtlas(const std::string& name)
    {
        return ReplaceChars(ToLower(name), {
                                               {' ',   '_'},
                                               { '\"', '_'}
        });
    }

    implicit Buff(std::string&& name)
        : Buff(0xFFFFFFFF, std::move(name))
    {}

    Buff(uint id, std::string&& name, int maxStacks = std::numeric_limits<int>::max())
        : id(id)
        , maxStacks(maxStacks)
        , name(std::move(name))
    {
        atlasEntry = NameToAtlas(this->name);
    }

    Buff(uint id, std::string&& name, std::string&& atlas, int maxStacks = std::numeric_limits<int>::max())
        : id(id)
        , maxStacks(maxStacks)
        , name(std::move(name))
        , atlasEntry(std::move(atlas))
    {}

    Buff(uint id, std::string&& name, glm::vec4&& uv, int maxStacks = std::numeric_limits<int>::max())
        : id(id)
        , maxStacks(maxStacks)
        , name(std::move(name))
        , uv(std::move(uv))
    {}

    Buff(std::initializer_list<uint> ids, std::string&& name, int maxStacks = std::numeric_limits<int>::max())
        : id(*ids.begin())
        , maxStacks(maxStacks)
        , name(std::move(name))
    {
        atlasEntry = NameToAtlas(this->name);
        extraIds.insert(ids.begin() + 1, ids.end());
    }

    Buff(std::initializer_list<uint> ids, std::string&& name, std::string&& atlas, int maxStacks = std::numeric_limits<int>::max())
        : id(*ids.begin())
        , maxStacks(maxStacks)
        , name(std::move(name))
        , atlasEntry(std::move(atlas))
    {
        extraIds.insert(ids.begin() + 1, ids.end());
    }

    Buff(std::initializer_list<uint> ids, std::string&& name, glm::vec2&& uv, int maxStacks = std::numeric_limits<int>::max())
        : id(*ids.begin())
        , maxStacks(maxStacks)
        , name(std::move(name))
        , uv(std::move(uv))
    {
        extraIds.insert(ids.begin() + 1, ids.end());
    }

    [[nodiscard]] int GetStacks(std::unordered_map<uint, int>& activeBuffs) const
    {
        return std::accumulate(extraIds.begin(), extraIds.end(), activeBuffs[id], [&](int a, uint b) { return a + activeBuffs[b]; });
    }

    [[nodiscard]] bool ShowNumber(int count) const
    {
        return maxStacks > 1 && count > 1;
    }
};

class Buffs
#ifdef _DEBUG
    : public SettingsMenu::Implementer
#endif
{
public:
    Buffs(ComPtr<ID3D11Device>& dev);

#ifdef _DEBUG
    void                      DrawMenu(Keybind** currentEditedKeybind) override;

    [[nodiscard]] const char* GetTabName() const override
    {
        return "Buffs Analyzer";
    }
#endif

    void                     UpdateBuffsTable(StackedBuff* buffs);

    static inline const Buff UnknownBuff{ 0, "Unknown", 1 };

    [[nodiscard]] auto       buffs() const
    {
        return std::span{ buffs_ };
    }
    [[nodiscard]] glm::vec2 GetNumber(int n) const
    {
        // 0 and 1 are not in the array, so 2 is at index 0
        n -= 2;
        return n < 0 ? glm::vec2{} : n < int(numbers_.size()) ? numbers_[n] : numbers_.back();
    }
    [[nodiscard]] const auto& buffsMap() const
    {
        return buffsMap_;
    }
    [[nodiscard]] auto& activeBuffs() const
    {
        return activeBuffs_;
    }

    [[nodiscard]] const auto& buffsAtlasUVSize() const
    {
        return buffsAtlasUVSize_;
    }
    [[nodiscard]] const auto& numbersAtlasUVSize() const
    {
        return numbersAtlasUVSize_;
    }

    [[nodiscard]] const Texture2D& buffsAtlas() const
    {
        return buffsAtlas_;
    }
    [[nodiscard]] const Texture2D& numbersAtlas() const
    {
        return numbersAtlas_;
    }

    bool DrawBuffCombo(const char* name, const Buff*& selectedBuf, std::span<char> searchBuffer) const;

protected:
    Texture2D                             buffsAtlas_;
    Texture2D                             numbersAtlas_;

    glm::vec2                             buffsAtlasUVSize_;
    glm::vec2                             numbersAtlasUVSize_;

    const std::vector<Buff>               buffs_;
    const std::map<int, const Buff*>      buffsMap_;
    const std::vector<glm::vec2>          numbers_;
    mutable std::unordered_map<uint, int> activeBuffs_;
    int                                   lastGetBuffsError_ = 0;

    static std::vector<Buff>              GenerateBuffsList(glm::vec2& uvSize);
    static std::map<int, const Buff*>     GenerateBuffsMap(const std::vector<Buff>& lst);

#ifdef _DEBUG
    int                         guildLogId_ = 3;
    std::map<uint, std::string> buffNames_;
    bool                        hideInactive_ = false;
    std::set<uint>              hiddenBuffs_;

    void                        SaveNames() const;
    void                        LoadNames();
#endif
};
} // namespace GW2Clarity