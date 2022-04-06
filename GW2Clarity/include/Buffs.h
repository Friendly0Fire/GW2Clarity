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

namespace GW2Clarity
{
struct Buff
{
	uint id;
	std::string name;
	glm::vec4 uv;
};

class Buffs
{
public:
	Buffs(ComPtr<ID3D11Device>& dev);

	void Draw(ComPtr<ID3D11DeviceContext>& ctx);
	void UpdateBuffsTable(StackedBuff* buffs);

protected:
	void Load();
	void Save();
	static inline const glm::ivec2 GridDefaultSpacing{ 64, 64 };
	static inline const Buff UnknownBuff{ 0, "Unknown", { 0.f, 0.f, 0.f, 0.f } };

	struct Threshold
	{
		int threshold = 1;
		ImVec4 tint { 1, 1, 1, 1 };
	};

	struct Item
	{
		glm::ivec2 pos { 0, 0 };
		const Buff* buff = &UnknownBuff;
		std::vector<Threshold> thresholds{
			{ 1, ImVec4(1, 0.5f, 0.5f, 0.33f) }
		};
	};
	struct Grid
	{
		glm::ivec2 spacing = GridDefaultSpacing;
		std::map<int, Item> items;
		std::string name { "New Grid" };
		bool attached = false;
		bool square = true;
	};

	std::map<int, Grid> grids_;
	Texture2D buffsAtlas_;
	Texture2D numbersAtlas_;

	Grid creatingGrid_;
	Item creatingItem_;
	int editingItemFakeCount_ = 1;
	int currentGridId_ = 0;
	int currentItemId_ = 0;

	static inline const int UnselectedId = -1;
	static inline const int NewId = -2;

	int selectedGridId_ = UnselectedId;
	int selectedItemId_ = UnselectedId;

	const std::vector<Buff> buffs_;
	const std::map<int, const Buff*> buffsMap_;
	const std::vector<glm::vec4> numbersMap_;
	std::map<uint, std::pair<int, int>> activeBuffs_;


	static std::vector<Buff> GenerateBuffsList();
	static std::map<int, const Buff*> GenerateBuffsMap(const std::vector<Buff>& lst);

	bool draggingGridScale_ = false;
	bool placingItem_ = false;

	static const int InvisibleWindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollWithMouse;

#ifdef _DEBUG
	int guildLogId_ = 3;
	std::map<uint, std::string> buffNames_;
	bool hideInactive_ = false;
	std::set<uint> hiddenBuffs_;

	void SaveNames();
	void LoadNames();
#endif
};
}