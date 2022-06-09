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

namespace GW2Clarity
{
struct Buff
{
	uint id;
	std::string name;
	std::string atlasEntry;
	glm::vec4 uv {};
	
	Buff(std::string&& name)
		: Buff(0xFFFFFFFF, std::move(name)) {}

	Buff(uint id, std::string&& name)
		: id(id), name(std::move(name)) {
		atlasEntry = ReplaceChar(ToLower(this->name), ' ', '_');
	}

	Buff(uint id, std::string&& name, std::string&& atlas)
		: id(id), name(std::move(name)), atlasEntry(std::move(atlas)) {}

	Buff(uint id, std::string&& name, glm::vec4&& uv)
		: id(id), name(std::move(name)), uv(uv) {}
};

class Buffs : public SettingsMenu::Implementer
{
public:
	Buffs(ComPtr<ID3D11Device>& dev);
	~Buffs();

	void Draw(ComPtr<ID3D11DeviceContext>& ctx);
	void UpdateBuffsTable(StackedBuff* buffs);
	void DrawMenu(Keybind** currentEditedKeybind) override;

	const char* GetTabName() const override { return "Buffs"; }

protected:
	void Load();
	void Save();

	void DrawEditingGrid();
	void PlaceItem();
	void DrawGridList();
	void DrawItems();

	static inline constexpr glm::ivec2 GridDefaultSpacing{ 64, 64 };
	static inline const Buff UnknownBuff{ 0, "Unknown", glm::vec4{ 0.f, 0.f, 0.f, 0.f } };

	struct Threshold
	{
		int threshold = 1;
		ImVec4 tint { 1, 1, 1, 1 };
	};

	static inline const short UnselectedSubId = -1;
	static inline const short NewSubId = -2;

	union Id
	{
		int id;
		struct {
			short grid;
			short item;
		};

		template<std::integral T1, std::integral T2>
		constexpr Id(T1 g, T2 i) : grid(short(g)), item(short(i)) { }

		constexpr bool operator==(const Id& other) const {
			return id == other.id;
		}
		constexpr bool operator!=(const Id& other) const {
			return id != other.id;
		}
	};
	static_assert(sizeof(Id) == sizeof(int));

	template<std::integral T = short>
	static Id Unselected(T gid = T(UnselectedSubId)) { return { short(gid), UnselectedSubId }; }
	template<std::integral T = short>
	static Id New(T gid = T(NewSubId)) { return { short(gid), NewSubId }; }

	void Delete(Id& id);

	struct Item
	{
		glm::ivec2 pos { 0, 0 };
		const Buff* buff = &UnknownBuff;
		std::vector<const Buff*> additionalBuffs;
		std::vector<Threshold> thresholds{
			{ 1, ImVec4(1, 0.5f, 0.5f, 0.33f) }
		};
	};
	struct Grid
	{
		glm::ivec2 spacing = GridDefaultSpacing;
		std::vector<Item> items;
		std::string name { "New Grid" };
		bool attached = false;
		bool square = true;
	};
	struct Set
	{
		std::string name;
		std::set<int> grids;
		bool combatOnly = true;
	};

	Grid creatingGrid_;
	Item creatingItem_;
	Set creatingSet_;
	short currentSetId_ = UnselectedSubId;

	static inline const char* ChangeSetPopupName = "QuickSet";

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

	std::vector<Grid> grids_;
	std::vector<Set> sets_;
	Texture2D buffsAtlas_;
	Texture2D numbersAtlas_;

	Id selectedId_ = Unselected();
	short selectedSetId_ = UnselectedSubId;

	int editingItemFakeCount_ = 1;

	const std::vector<Buff> buffs_;
	const std::map<int, const Buff*> buffsMap_;
	const std::vector<glm::vec4> numbersMap_;
	std::map<uint, std::pair<int, int>> activeBuffs_;


	static std::vector<Buff> GenerateBuffsList();
	static std::map<int, const Buff*> GenerateBuffsMap(const std::vector<Buff>& lst);

	bool draggingGridScale_ = false;
	bool placingItem_ = false;
	mstime lastSaveTime_ = 0;
	bool needsSaving_ = false;
	static inline constexpr mstime SaveDelay = 1000;
	bool showSetSelector_ = false;
	char buffSearch_[512];
	bool firstDraw_ = true;

	static constexpr int InvisibleWindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollWithMouse;

	ActivationKeybind changeGridSetKey_;

#ifdef _DEBUG
	ActivationKeybind showAnalyzerKey_;
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