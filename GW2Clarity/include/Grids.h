#pragma once

#include <Main.h>
#include <Graphics.h>
#include <glm/glm.hpp>
#include <map>
#include <ConfigurationFile.h>
#include <mutex>
#include <span>
#include <imgui.h>
#include <set>
#include <ActivationKeybind.h>
#include <SettingsMenu.h>
#include <Sets.h>

namespace GW2Clarity
{
struct Buff
{
	uint id;
	int maxStacks;
	std::string name;
	std::string atlasEntry;
	glm::vec4 uv {};
	std::set<uint> extraIds;
	
	Buff(std::string&& name)
		: Buff(0xFFFFFFFF, std::move(name)) {}

	Buff(uint id, std::string&& name, int maxStacks = std::numeric_limits<int>::max())
		: id(id), name(std::move(name)), maxStacks(maxStacks) {
		atlasEntry = ReplaceChar(ToLower(this->name), ' ', '_');
	}

	Buff(uint id, std::string&& name, std::string&& atlas, int maxStacks = std::numeric_limits<int>::max())
		: id(id), name(std::move(name)), atlasEntry(std::move(atlas)), maxStacks(maxStacks) {}

	Buff(uint id, std::string&& name, glm::vec4&& uv, int maxStacks = std::numeric_limits<int>::max())
		: id(id), name(std::move(name)), uv(uv), maxStacks(maxStacks) {}

	Buff(std::initializer_list<uint> ids, std::string&& name, int maxStacks = std::numeric_limits<int>::max())
		: id(*ids.begin()), name(std::move(name)), maxStacks(maxStacks) {
		atlasEntry = ReplaceChar(ToLower(this->name), ' ', '_');
		extraIds.insert(ids.begin() + 1, ids.end());
	}

	Buff(std::initializer_list<uint> ids, std::string&& name, std::string&& atlas, int maxStacks = std::numeric_limits<int>::max())
		: id(*ids.begin()), name(std::move(name)), atlasEntry(std::move(atlas)), maxStacks(maxStacks) {
		extraIds.insert(ids.begin() + 1, ids.end());
	}

	Buff(std::initializer_list<uint> ids, std::string&& name, glm::vec4&& uv, int maxStacks = std::numeric_limits<int>::max())
		: id(*ids.begin()), name(std::move(name)), uv(uv), maxStacks(maxStacks) {
		extraIds.insert(ids.begin() + 1, ids.end());
	}
};

class Grids : public SettingsMenu::Implementer
{
public:
	Grids(ComPtr<ID3D11Device>& dev);
	virtual ~Grids();

	void Draw(ComPtr<ID3D11DeviceContext>& ctx, const Sets::Set* set, bool shouldIgnoreSet);
	void UpdateBuffsTable(StackedBuff* buffs);
	void DrawMenu(Keybind** currentEditedKeybind) override;

	const char* GetTabName() const override { return "Grids"; }

	void Delete(Id id);

protected:
	void Load();
	void Save();

	void DrawEditingGrid();
	void PlaceItem();
	void DrawGridList();
	void DrawItems(const Sets::Set* set, bool shouldIgnoreSet);

	static inline constexpr glm::ivec2 GridDefaultSpacing{ 64, 64 };
	static inline const Buff UnknownBuff{ 0, "Unknown", glm::vec4{ 0.f, 0.f, 0.f, 0.f } };

	struct Threshold
	{
		int threshold = 1;
		ImVec4 tint { 1, 1, 1, 1 };
	};

	struct Item
	{
		glm::ivec2 pos { 0, 0 };
		const Buff* buff = &UnknownBuff;
		std::vector<const Buff*> additionalBuffs;
		std::vector<Threshold> thresholds{
			{ 1, ImVec4(1, 0.5f, 0.5f, 0.33f) }
		};
	};

public:
	struct Grid
	{
		glm::ivec2 spacing = GridDefaultSpacing;
		glm::ivec2 offset = {};
		float centralWeight = 0.f;
		glm::ivec2 mouseClipMin { std::numeric_limits<int>::max() };
		glm::ivec2 mouseClipMax { std::numeric_limits<int>::min() };
		bool trackMouseWhileHeld = true;
		std::vector<Item> items;
		std::string name { "New Grid" };
		bool attached = false;
		bool square = true;
	};

	const std::vector<Grid>& grids() const { return grids_; }
	
	inline Grid& getG(const Id& id)
	{
		if (id.grid == UnselectedSubId)
			throw std::invalid_argument("unselected id");
		else if (id.grid == NewSubId)
			return creatingGrid_;
		else
			return grids_[id.grid];
	}

	inline Item& getI(const Id& id)
	{
		if (id.grid == UnselectedSubId || id.item == UnselectedSubId)
			throw std::invalid_argument("unselected id");
		else if (id.item == NewSubId)
			return creatingItem_;
		else
			return grids_[id.grid].items[id.item];
	}

protected:
	Grid creatingGrid_;
	Item creatingItem_;
	Id currentHovered_ = Unselected();

	std::vector<Grid> grids_;
	Texture2D buffsAtlas_;
	Texture2D numbersAtlas_;

	Id selectedId_ = Unselected();

	int editingItemFakeCount_ = 1;

	const std::vector<Buff> buffs_;
	const std::map<int, const Buff*> buffsMap_;
	const std::vector<glm::vec4> numbersMap_;
	std::unordered_map<uint, int> activeBuffs_;
	
	static std::vector<Buff> GenerateBuffsList();
	static std::map<int, const Buff*> GenerateBuffsMap(const std::vector<Buff>& lst);

	bool draggingGridScale_ = false, draggingMouseBoundaries_ = false;
	bool testMouseMode_ = false;
	bool placingItem_ = false;
	mstime lastSaveTime_ = 0;
	bool needsSaving_ = false;
	static inline constexpr mstime SaveDelay = 1000;
	char buffSearch_[512];
	bool firstDraw_ = true;
	ScanCode holdingMouseButton_ = ScanCode::NONE;
	ImVec2 heldMousePos_ {};
	int lastGetBuffsError_ = 0;

	static constexpr int InvisibleWindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollWithMouse;
	
#ifdef _DEBUG
	int guildLogId_ = 3;
	std::map<uint, std::string> buffNames_;
	bool hideInactive_ = false;
	std::set<uint> hiddenBuffs_;
	bool showAnalyzer_ = true;

	void SaveNames();
	void LoadNames();
	void DrawBuffAnalyzer();
#endif
};
}