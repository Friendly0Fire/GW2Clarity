#pragma once

#include <Main.h>
#include <Graphics.h>
#include <glm/glm.hpp>
#include <map>
#include <ConfigurationFile.h>
#include <mutex>
#include <span>

namespace GW2Clarity
{
struct Buff
{
	uint id;
	std::string name;
	glm::vec2 uv;
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
	static inline const glm::ivec2 GridDefaultSpacing{ 128, 128 };
	static inline const glm::vec2 AtlasSize{ 1.f / 16.f - 1.f / 2048.f, 1.f / 16.f - 1.f / 2048.f };
	static inline const Buff UnknownBuff{ 0, "Unknown", { 0, 0 } };

	struct Item
	{
		glm::ivec2 pos { 0, 0 };
		const Buff* buff = &UnknownBuff;
		float activeAlpha = 1.f;
		float inactiveAlpha = 0.1f;
	};
	struct Grid
	{
		glm::ivec2 spacing = GridDefaultSpacing;
		std::map<int, Item> items;
		std::string name { "New Grid" };
		bool attached = false;
	};

	std::map<int, Grid> grids_;
	Texture2D buffsAtlas_;

	Grid creatingGrid_;
	Item creatingItem_;
	int currentGridId_ = 0;
	int currentItemId_ = 0;

	static inline const int UnselectedId = -1;
	static inline const int NewId = -2;

	int selectedGridId_ = UnselectedId;
	int selectedItemId_ = UnselectedId;

	const std::vector<Buff> buffs_;
	const std::map<int, const Buff*> buffsMap_;
	std::map<uint, int> activeBuffs_;

	static std::vector<Buff> GenerateBuffsList();
	static std::map<int, const Buff*> GenerateBuffsMap(const std::vector<Buff>& lst);
};
}