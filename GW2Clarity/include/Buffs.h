#pragma once

#include <imgui.h>

#include "ActivationKeybind.h"
#include "ConfigurationFile.h"
#include "Graphics.h"
#include "Main.h"
#include "SettingsMenu.h"

namespace GW2Clarity
{

using BuffCategories = std::vector<std::string>;
using BuffCategoriesPtr = std::shared_ptr<BuffCategories>;

struct BuffDescription
{
    u32 id;
    i32 maxStacks;
    std::string name;
    std::unordered_set<u32> extraIds;
    Texture2D icon;
    BuffCategoriesPtr categories;

    inline static constexpr u32 InvalidId = std::numeric_limits<u32>::max();

    static std::string NameToTextureName(const std::string& name) {
        return ReplaceChars(ToLower(name), { { ' ', '_' }, { '\"', '_' } });
    }
    static Texture2D LoadTexture(const std::string& tex);

    template<std::convertible_to<const char*>... Args>
    implicit BuffDescription(Args&&... cats) : id(InvalidId), maxStacks(std::numeric_limits<i32>::max()) {
        if constexpr(sizeof...(cats) > 0)
            currentCategories_s = std::make_shared<BuffCategories>(std::initializer_list<std::string> { std::forward<Args>(cats)... });
        else
            currentCategories_s = nullptr;

        name = currentCategories_s ? currentCategories_s->at(0) : "<undefined>";
    }

    BuffDescription(u32 i, std::string&& n, i32 m = std::numeric_limits<i32>::max())
        : id(i), maxStacks(m), name(std::move(n)), icon(LoadTexture(NameToTextureName(name))), categories(currentCategories_s) {
    }

    BuffDescription(u32 i, std::string&& n, std::string&& t, i32 m = std::numeric_limits<i32>::max())
        : id(i), maxStacks(m), name(std::move(n)), icon(LoadTexture(NameToTextureName(t))), categories(currentCategories_s) {
    }

    BuffDescription(std::initializer_list<u32> is, std::string&& n, i32 m = std::numeric_limits<i32>::max())
        : id(*is.begin()), maxStacks(m), name(std::move(n)), icon(LoadTexture(NameToTextureName(name))), categories(currentCategories_s) {
        extraIds.insert(is.begin() + 1, is.end());
    }

    BuffDescription(std::initializer_list<u32> is, std::string&& n, std::string&& t, i32 m = std::numeric_limits<i32>::max())
        : id(*is.begin()), maxStacks(m), name(std::move(n)), icon(LoadTexture(NameToTextureName(t))), categories(currentCategories_s) {
        extraIds.insert(is.begin() + 1, is.end());
    }

    [[nodiscard]] i32 GetStacks(std::unordered_map<u32, i32>& activeBuffs) const {
        return std::accumulate(extraIds.begin(), extraIds.end(), activeBuffs[id], [&](i32 a, u32 b) { return a + activeBuffs[b]; });
    }

    [[nodiscard]] bool ShowNumber(i32 count) const {
        return maxStacks > 1 && count > 1;
    }

private:
    inline static BuffCategoriesPtr currentCategories_s = nullptr;
};

class BuffsLibrary
#ifdef _DEBUG
    : public SettingsMenu::Implementer
#endif
{
public:
    BuffsLibrary(ComPtr<ID3D11Device>& dev);

#ifdef _DEBUG
    void DrawMenu(Keybind** currentEditedKeybind) override;

    [[nodiscard]] const char* GetTabName() const override {
        return "Buffs Analyzer";
    }
#endif

    void UpdateBuffsTable(StackedBuff* buffs);

    static inline const BuffDescription UnknownBuff { "Unknown" };

    [[nodiscard]] auto buffs() const {
        return std::span { buffs_ };
    }
    [[nodiscard]] vec2 numberUV(i32 n) const {
        // 0 and 1 are not in the array, so 2 is at index 0
        n -= 2;
        return n < 0 ? vec2 {} : n < static_cast<i32>(numbers_.size()) ? numbers_[n] : numbers_.back();
    }
    [[nodiscard]] const auto& buffsMap() const {
        return buffsMap_;
    }
    [[nodiscard]] auto& activeBuffs() const {
        return activeBuffs_;
    }

    [[nodiscard]] const auto& numbersAtlasUVSize() const {
        return numbersAtlasUVSize_;
    }

    [[nodiscard]] const Texture2D& numbersAtlas() const {
        return numbersAtlas_;
    }

    bool DrawBuffCombo(const char* name, const BuffDescription*& selectedBuf, std::span<char> searchBuffer) const;

protected:
    Texture2D numbersAtlas_;
    vec2 numbersAtlasUVSize_;

    const std::vector<BuffDescription> buffs_;
    const std::unordered_map<i32, const BuffDescription*> buffsMap_;
    const std::vector<vec2> numbers_;
    mutable std::unordered_map<u32, i32> activeBuffs_;
    i32 lastGetBuffsError_ = 0;

    static std::vector<BuffDescription> GenerateBuffsList();
    static std::unordered_map<i32, const BuffDescription*> GenerateBuffsMap(const std::vector<BuffDescription>& lst);

#ifdef _DEBUG
    i32 guildLogId_ = 3;
    std::unordered_map<u32, std::string> buffNames_;
    bool hideInactive_ = false;
    std::set<u32> hiddenBuffs_;

    void SaveNames() const;
    void LoadNames();
#endif
};

} // namespace GW2Clarity
