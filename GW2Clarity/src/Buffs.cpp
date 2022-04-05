#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>

#include <Buffs.h>
#include <Core.h>
#include <imgui.h>
#include <ImGuiExtensions.h>
#include <glm/gtc/type_ptr.hpp>
#include <misc/cpp/imgui_stdlib.h>
#include <SimpleIni.h>

namespace GW2Clarity
{
ImVec2 AdjustToArea(int w, int h, int availW)
{
	ImVec2 dims(w, h);
	if (dims.x < dims.y)
		dims.x = dims.y * w / h;
	else if (dims.x > dims.y)
		dims.y = dims.x * h / w;

	if (availW < dims.x)
	{
		dims.y *= availW / dims.x;
		dims.x = availW;
	}

	return dims;
}

Buffs::Buffs(ComPtr<ID3D11Device>& dev)
	: buffs_(GenerateBuffsList())
	, buffsMap_(GenerateBuffsMap(buffs_))
{
	buffsAtlas_ = CreateTextureFromResource(dev.Get(), Core::i().dllModule(), IDR_BOONS);

	Load();
}

void Buffs::Draw(ComPtr<ID3D11DeviceContext>& ctx)
{
	if (ImGui::Begin("Buffs Management"))
	{
		for (auto& [gid, g] : grids_)
		{
			if (ImGui::Selectable(std::format("{} ({}x{})##{}", g.name, g.spacing.x, g.spacing.y, gid).c_str(), selectedGridId_ == gid && selectedItemId_ == UnselectedId))
			{
				selectedGridId_ = gid;
				selectedItemId_ = UnselectedId;
			}
			ImGui::Indent();
			for (auto& [iid, i] : g.items)
			{
				if (ImGui::Selectable(std::format("{} ({}, {})##{}", i.buff->name.c_str(), i.pos.x, i.pos.y, iid).c_str(), selectedItemId_ == iid))
				{
					selectedGridId_ = gid;
					selectedItemId_ = iid;
				}
			}
			if (ImGui::Selectable(std::format("+ New Item##{}", gid).c_str(), selectedGridId_ == gid && selectedItemId_ == NewId))
			{
				selectedGridId_ = gid;
				selectedItemId_ = NewId;
			}
			ImGui::Unindent();
		}
		if (ImGui::Selectable("+ New Grid", selectedGridId_ == NewId))
		{
			selectedGridId_ = NewId;
			selectedItemId_ = UnselectedId;
		}

		bool editingGrid = selectedGridId_ >= 0 && selectedItemId_ == UnselectedId;
		bool editingItem = selectedItemId_ >= 0;
		if (editingGrid || selectedGridId_ == NewId)
		{
			ImGui::Separator();

			auto& editGrid = editingGrid ? grids_[selectedGridId_] : creatingGrid_;
			if (editingGrid)
				ImGuiTitle(std::format("Editing Grid '{}'", editGrid.name).c_str());
			else
				ImGuiTitle("New Grid");

			ImGui::DragInt2("Grid Scale", glm::value_ptr(editGrid.spacing));
			ImGui::InputText("Grid Name", &editGrid.name);
			ImGui::Checkbox("Attached to Mouse", &editGrid.attached);

			if (selectedGridId_ == NewId && ImGui::Button("Create Grid"))
			{
				grids_[currentGridId_] = creatingGrid_;
				creatingGrid_.name = "New Grid";
				creatingGrid_.spacing = GridDefaultSpacing;
				creatingGrid_.attached = false;
				currentGridId_++;
			}
		}

		if (editingItem || selectedItemId_ == NewId)
		{
			auto& editItem = editingItem ? grids_[selectedGridId_].items[selectedItemId_] : creatingItem_;
			if (editingItem)
				ImGuiTitle(std::format("Editing Item '{}' of '{}'", editItem.buff->name, grids_[selectedGridId_].name).c_str());
			else
				ImGuiTitle(std::format("New Item in '{}'", grids_[selectedGridId_].name).c_str());

			ImGui::Separator();

			if (ImGui::BeginCombo("Buff", editItem.buff->name.c_str()))
			{
				for (auto& b : buffs_)
				{
					ImGui::Image(buffsAtlas_.srv.Get(), ImVec2(32, 32), ToImGui(b.uv.xy), ToImGui(b.uv.zw));
					ImGui::SameLine();
					if (ImGui::Selectable(b.name.c_str(), false))
						editItem.buff = &b;
				}
				ImGui::EndCombo();
			}
			ImGui::DragInt2("Location", glm::value_ptr(editItem.pos));

			if (selectedItemId_ == NewId && ImGui::Button("Create Item"))
			{
				grids_[selectedGridId_].items[currentItemId_] = creatingItem_;
				creatingItem_.pos = { 1, 1 };
				creatingItem_.buff = &UnknownBuff;
				currentItemId_++;
			}
		}

		ImGui::Separator();
		if (ImGui::Button("Save", ImVec2(128, 32)))
			Save();
	}
	ImGui::End();

	glm::vec2 screen{ ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y };
	glm::vec2 mouse{ ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y };
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
	if (ImGui::Begin("Display Area", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollWithMouse))
	{
		auto* font = Core::i().fontBuffCounter();
		ImGui::PushFont(font);
		float origFontScale = font->Scale;
		for (const auto& [gid, g] : grids_)
		{
			for (const auto& [iid, i] : g.items)
			{
				int count = activeBuffs_[i.buff->id];
				if (count == 0 && i.inactiveAlpha < 1.f / 255.f)
					continue;
				
				glm::vec2 pos = (g.attached ? mouse : screen * 0.5f) + glm::vec2(i.pos * g.spacing);

				ImGui::SetCursorPos(ToImGui(pos));
				ImGui::Image(buffsAtlas_.srv.Get(), AdjustToArea(128, 128, g.spacing.x), ToImGui(i.buff->uv.xy), ToImGui(i.buff->uv.zw), ImVec4(1, 1, 1, count > 0 ? i.activeAlpha : i.inactiveAlpha));
				if (count > 1)
				{
					pos += glm::vec2(g.spacing);
					font->Scale = 0.01f * g.spacing.x;
					auto countStr = std::format("{}", count);
					auto dim = ImGui::CalcTextSize(countStr.c_str());
					dim.y *= 0.85f;
					dim.x += float(g.spacing.x) / 16;

					ImGui::SetCursorPos(ToImGui(pos) - dim);
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 1));
					ImGui::TextUnformatted(countStr.c_str());
					ImGui::PopStyleColor();
					ImGui::SetCursorPos(ToImGui(pos) - dim - ImVec2(font->Scale * 2.f, font->Scale * 2.f));
					ImGui::TextUnformatted(countStr.c_str());
				}
			}
		}
		font->Scale = origFontScale;
		ImGui::PopFont();
	}
	ImGui::End();
}

void Buffs::UpdateBuffsTable(StackedBuff* buffs)
{
	activeBuffs_.clear();
	for (size_t i = 0; buffs[i].id; i++)
		activeBuffs_[buffs[i].id] = buffs[i].count;
}

void Buffs::Load()
{
#if 0
	grids_.clear();

	ConfigurationFile::i().Reload();
	auto& ini = ConfigurationFile::i().ini();

	std::vector<std::string> itemParts;
	std::list<CSimpleIniA::Entry> sections;
	ini.GetAllSections(sections);
	for (auto& sec : sections)
	{
		std::string secName = sec.pItem;
		if (secName.starts_with("Grid "))
		{
			Grid loadedGrid;
			loadedGrid.name = secName.substr(5);
			loadedGrid.spacing.x = ini.GetLongValue(secName.c_str(), "spacing_x", GridDefaultSpacing.x);
			loadedGrid.spacing.y = ini.GetLongValue(secName.c_str(), "spacing_y", GridDefaultSpacing.y);
			loadedGrid.attached = ini.GetBoolValue(secName.c_str(), "attached");

			std::list<CSimpleIniA::Entry> keys;
			ini.GetAllKeys(secName.c_str(), keys);
			for (auto& k : keys)
			{
				std::string keyName = k.pItem;
				if (keyName.starts_with("item_"))
				{
					auto* i = ini.GetValue(secName.c_str(), keyName.c_str());
					itemParts.clear();
					SplitString(i, ",", std::back_inserter(itemParts));
					Item it;
					it.buff = buffsMap_.at(std::atoi(itemParts[0].c_str()));
					if (itemParts.size() > 1)
						it.pos.x = std::atoi(itemParts[1].c_str());
					if (itemParts.size() > 2)
						it.pos.y = std::atoi(itemParts[2].c_str());
					if (itemParts.size() > 3)
						it.activeAlpha = std::atof(itemParts[3].c_str());
					if (itemParts.size() > 4)
						it.inactiveAlpha = std::atof(itemParts[4].c_str());

					loadedGrid.items[currentItemId_++] = it;
				}
			}

			grids_[currentGridId_++] = loadedGrid;
		}
	}
#endif
}

void Buffs::Save()
{
#if 0
	auto& ini = ConfigurationFile::i().ini();

	for (const auto& [gid, g] : grids_)
	{
		std::string secName = "Grid " + g.name;
		ini.SetLongValue(secName.c_str(), "spacing_x", g.spacing.x);
		ini.SetLongValue(secName.c_str(), "spacing_y", g.spacing.y);
		ini.SetBoolValue(secName.c_str(), "attached", g.attached);

		for (const auto& [iid, i] : g.items)
			ini.SetValue(secName.c_str(), std::format("item_{}", iid).c_str(), std::format("{}, {}, {}, {}, {}", i.buff->id, i.pos.x, i.pos.y, i.activeAlpha, i.inactiveAlpha).c_str());
	}

	ConfigurationFile::i().Save();
#endif
}

std::vector<Buff> Buffs::GenerateBuffsList()
{
	const std::map<std::string, glm::vec4> atlasElements
	{
		#include <assets/atlas.inc>
	};

	std::vector<Buff> buffs {
		{ 743, "Aegis" },
		{ 30328, "Alacrity" },
		{ 725, "Fury" },
		{ 719, "Swiftness" },
		{ 740, "Might" },
		{ 717, "Protection" },
		{ 1187, "Quickness" },
		{ 718, "Regeneration" },
		{ 26980, "Resistance" },
		{ 873, "Resolution" },
		{ 1122, "Stability" },
		{ 726, "Vigor" },
	};
	for (auto& b : buffs)
	{
		auto it = atlasElements.find(ToLower(b.name));
		b.uv = it != atlasElements.end() ? it->second : glm::vec4 { 0.f, 0.f, 0.f, 0.f };
	}

	std::ranges::sort(buffs, [](auto& a, auto& b) { return a.name < b.name; });

	return buffs;
}

std::map<int, const Buff*> Buffs::GenerateBuffsMap(const std::vector<Buff>& lst)
{
	std::map<int, const Buff*> m;
	for (auto& b : lst)
	{
		m[b.id] = &b;
	}
	return m;
}

}