#pragma once

#include <imgui.h>

#include "ActivationKeybind.h"
#include "ConfigurationFile.h"
#include "Graphics.h"
#include "Layouts.h"
#include "Main.h"
#include "SettingsMenu.h"

namespace GW2Clarity
{

struct Buff
{
    u32 id;
    i32 maxStacks;
    std::string name;
    std::string atlasEntry;
    vec2 uv {};
    std::set<u32> extraIds;
    std::string category;

    static std::string NameToAtlas(const std::string& name) { return ReplaceChars(ToLower(name), { { ' ', '_' }, { '\"', '_' } }); }

    implicit Buff(std::string&& name) : Buff(0xFFFFFFFF, std::move(name)) { }

    Buff(u32 id, std::string&& name, i32 maxStacks = std::numeric_limits<i32>::max())
        : id(id), maxStacks(maxStacks), name(std::move(name)) {
        atlasEntry = NameToAtlas(this->name);
    }

    Buff(u32 id, std::string&& name, std::string&& atlas, i32 maxStacks = std::numeric_limits<i32>::max())
        : id(id), maxStacks(maxStacks), name(std::move(name)), atlasEntry(std::move(atlas)) { }

    Buff(u32 id, std::string&& name, vec4&& uv, i32 maxStacks = std::numeric_limits<i32>::max())
        : id(id), maxStacks(maxStacks), name(std::move(name)), uv(std::move(uv)) { }

    Buff(std::initializer_list<u32> ids, std::string&& name, i32 maxStacks = std::numeric_limits<i32>::max())
        : id(*ids.begin()), maxStacks(maxStacks), name(std::move(name)) {
        atlasEntry = NameToAtlas(this->name);
        extraIds.insert(ids.begin() + 1, ids.end());
    }

    Buff(std::initializer_list<u32> ids, std::string&& name, std::string&& atlas, i32 maxStacks = std::numeric_limits<i32>::max())
        : id(*ids.begin()), maxStacks(maxStacks), name(std::move(name)), atlasEntry(std::move(atlas)) {
        extraIds.insert(ids.begin() + 1, ids.end());
    }

    Buff(std::initializer_list<u32> ids, std::string&& name, vec2&& uv, i32 maxStacks = std::numeric_limits<i32>::max())
        : id(*ids.begin()), maxStacks(maxStacks), name(std::move(name)), uv(std::move(uv)) {
        extraIds.insert(ids.begin() + 1, ids.end());
    }

    [[nodiscard]] i32 GetStacks(std::unordered_map<u32, i32>& activeBuffs) const {
        return std::accumulate(extraIds.begin(), extraIds.end(), activeBuffs[id], [&](i32 a, u32 b) { return a + activeBuffs[b]; });
    }

    [[nodiscard]] bool ShowNumber(i32 count) const { return maxStacks > 1 && count > 1; }
};

class Buffs
#ifdef _DEBUG
    : public SettingsMenu::Implementer
#endif
{
public:
    Buffs(ComPtr<ID3D11Device>& dev);

#ifdef _DEBUG
    void DrawMenu(Keybind** currentEditedKeybind) override;

    [[nodiscard]] const char* GetTabName() const override { return "Buffs Analyzer"; }
#endif

    void UpdateBuffsTable(StackedBuff* buffs);

    static inline const Buff UnknownBuff { 0, "Unknown", 1 };

    [[nodiscard]] auto buffs() const { return std::span { buffs_ }; }
    [[nodiscard]] vec2 GetNumber(i32 n) const {
        // 0 and 1 are not in the array, so 2 is at index 0
        n -= 2;
        return n < 0 ? vec2 {} : n < i32(numbers_.size()) ? numbers_[n] : numbers_.back();
    }
    [[nodiscard]] const auto& buffsMap() const { return buffsMap_; }
    [[nodiscard]] auto& activeBuffs() const { return activeBuffs_; }

    [[nodiscard]] const auto& buffsAtlasUVSize() const { return buffsAtlasUVSize_; }
    [[nodiscard]] const auto& numbersAtlasUVSize() const { return numbersAtlasUVSize_; }

    [[nodiscard]] const Texture2D& buffsAtlas() const { return buffsAtlas_; }
    [[nodiscard]] const Texture2D& numbersAtlas() const { return numbersAtlas_; }

    bool DrawBuffCombo(const char* name, const Buff*& selectedBuf, std::span<char> searchBuffer) const;

protected:
    Texture2D buffsAtlas_;
    Texture2D numbersAtlas_;

    vec2 buffsAtlasUVSize_;
    vec2 numbersAtlasUVSize_;

    const std::vector<Buff> buffs_;
    const std::unordered_map<i32, const Buff*> buffsMap_;
    const std::vector<vec2> numbers_;
    mutable std::unordered_map<u32, i32> activeBuffs_;
    i32 lastGetBuffsError_ = 0;

    static std::vector<Buff> GenerateBuffsList(vec2& uvSize);
    static std::unordered_map<i32, const Buff*> GenerateBuffsMap(const std::vector<Buff>& lst);

#ifdef _DEBUG
    i32 guildLogId_ = 3;
    std::unordered_map<u32, std::string> buffNames_;
    bool hideInactive_ = false;
    std::set<u32> hiddenBuffs_;

    void SaveNames() const;
    void LoadNames();
#endif
};

class BuffComboBox
{
public:
    BuffComboBox(const Buffs* buffs, std::string_view name) : buffs_(buffs), name_(name) { }

    const auto& name() const { return name_; }

    bool Draw(const char* display = nullptr) { return buffs_->DrawBuffCombo(display ? display : name_.c_str(), selectedBuff_, buffer_); }

    const auto* selectedBuff() const { return selectedBuff_; }

protected:
    const Buffs* buffs_;
    std::string name_;
    const Buff* selectedBuff_ = nullptr;
    std::array<char, 512> buffer_ { '\0' };
};

} // namespace GW2Clarity
