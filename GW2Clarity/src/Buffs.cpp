#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>

#include <Buffs.h>
#include <Core.h>
#include <imgui.h>
#include <ImGuiExtensions.h>
#include <glm/gtc/type_ptr.hpp>
#include <misc/cpp/imgui_stdlib.h>
#include <SimpleIni.h>
#include <cppcodec/base64_rfc4648.hpp>
#include <skyr/percent_encoding/percent_encode.hpp>
#include <shellapi.h>

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
#ifdef _DEBUG
	LoadNames();
#endif
}

#ifdef _DEBUG
void Buffs::LoadNames()
{
	buffNames_.clear();

	wchar_t fn[MAX_PATH];
	GetModuleFileName(GetBaseCore().dllModule(), fn, MAX_PATH);

	std::filesystem::path namesPath = fn;
	namesPath = namesPath.remove_filename() / "names.tsv";

	std::ifstream namesFile(namesPath);
	if (namesFile.good())
	{
		std::string line;
		std::vector<std::string> cols;
		bool header = true;
		while (std::getline(namesFile, line))
		{
			if (header)
			{
				header = false;
				continue;
			}

			cols.clear();
			SplitString(line.c_str(), "\t", std::back_inserter(cols));
			GW2_ASSERT(cols.size() == 1 || cols.size() == 2);

			buffNames_[std::atoi(cols[0].c_str())] = cols.size() == 2 ? cols[1] : "";
		}
	}
}

void Buffs::SaveNames()
{
	wchar_t fn[MAX_PATH];
	GetModuleFileName(GetBaseCore().dllModule(), fn, MAX_PATH);

	std::filesystem::path namesPath = fn;
	namesPath = namesPath.remove_filename() / "names.tsv";

	std::ofstream namesFile(namesPath, std::ofstream::trunc | std::ofstream::out);
	if (namesFile.good())
	{
		namesFile << "ID\tName" << std::endl;
		for (auto& n : buffNames_)
			namesFile << n.first << "\t" << n.second << std::endl;
	}
}
#endif

void Buffs::Draw(ComPtr<ID3D11DeviceContext>& ctx)
{
	if (ImGui::Begin("Buffs Management"))
	{
		if (draggingGridScale_ || placingItem_)
		{
			const auto& sp = selectedGridId_ >= 0 ? grids_[selectedGridId_].spacing : creatingGrid_.spacing;
			auto d = ImGui::GetIO().DisplaySize;
			auto c = d * 0.5f;

			auto* cmdList = ImGui::GetWindowDrawList();
			cmdList->PushClipRectFullScreen();

			int i = 0;
			auto color = [&i]() { return i != 0 ? (i % 4 == 0 ? 0xCCFFFFFF : 0x55FFFFFF) : 0xFFFFFFFF; };
			for (float x = 0.f; x < c.x; x += sp.x)
			{
				cmdList->AddLine(ImVec2(c.x + x, 0.f), ImVec2(c.x + x, d.y), color(), 1.2f);
				cmdList->AddLine(ImVec2(c.x - x, 0.f), ImVec2(c.x - x, d.y), color(), 1.2f);
				i++;
			}

			i = 0;
			for (float y = 0.f; y < c.y; y += sp.y)
			{
				cmdList->AddLine(ImVec2(0.f, c.y + y), ImVec2(d.x, c.y + y), color(), 1.2f);
				cmdList->AddLine(ImVec2(0.f, c.y - y), ImVec2(d.x, c.y - y), color(), 1.2f);
				i++;
			}
			cmdList->PopClipRect();

			draggingGridScale_ = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
		}

		if (placingItem_)
		{
			const auto& sp = grids_[selectedGridId_].spacing;
			auto& item = selectedItemId_ >= 0 ? grids_[selectedGridId_].items[selectedItemId_] : creatingItem_;
			auto mouse = ImGui::GetIO().MousePos - ImGui::GetIO().DisplaySize * 0.5f;

			item.pos = glm::ivec2(glm::floor(glm::vec2(mouse.x, mouse.y) / glm::vec2(sp)));

			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				placingItem_ = false;
		}

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

			ImGui::Checkbox("Square Grid", &editGrid.square);
			if (editGrid.square && ImGui::DragInt("Grid Scale", (int*)&editGrid.spacing, 0.1f, 1, 2048) ||
				!editGrid.square && ImGui::DragInt2("Grid Scale", glm::value_ptr(editGrid.spacing), 0.1f, 1, 2048))
			{
				draggingGridScale_ = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
			}
			if (editGrid.square)
				editGrid.spacing.y = editGrid.spacing.x;

			ImGui::InputText("Grid Name", &editGrid.name);
			ImGui::Checkbox("Attached to Mouse", &editGrid.attached);

			if (selectedGridId_ == NewId && ImGui::Button("Create Grid"))
			{
				grids_[currentGridId_] = creatingGrid_;
				creatingGrid_ = {};
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
			ImGui::DragInt2("Location", glm::value_ptr(editItem.pos), 0.1f);
			ImGui::SameLine();
			if (ImGui::Button("Place"))
				placingItem_ = true;

			if (selectedItemId_ == NewId && ImGui::Button("Create Item"))
			{
				grids_[selectedGridId_].items[currentItemId_] = creatingItem_;
				creatingItem_ = {};
				currentItemId_++;
			}
		}

		ImGui::Separator();
		if (ImGui::Button("Save", ImVec2(128, 32)))
			Save();

#ifdef _DEBUG
		ImGui::Separator();
		ImGui::InputInt("Say in Guild", &guildLogId_, 1);
		if (guildLogId_ < 0 || guildLogId_ > 5)
			guildLogId_ = 1;

		ImGui::Checkbox("Hide any inactive", &hideInactive_);
		if (ImGui::Button("Hide currently inactive"))
		{
			for (auto& [id, buff] : activeBuffs_)
				if (buff.first == 0)
					hiddenBuffs_.insert(id);
		}
		ImGui::SameLine();
		if (ImGui::Button("Hide currently active"))
		{
			for (auto& [id, buff] : activeBuffs_)
				if (buff.first > 0)
					hiddenBuffs_.insert(id);
		}
		ImGui::SameLine();
		if (ImGui::Button("Unhide all"))
			hiddenBuffs_.clear();

		if (ImGui::BeginTable("Active Buffs", 5, ImGuiTableFlags_RowBg))
		{
			ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 60.f);
			ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthFixed, 40.f);
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 5.f);
			ImGui::TableSetupColumn("Max Duration", ImGuiTableColumnFlags_WidthStretch, 2.f);
			ImGui::TableSetupColumn("Chat Link", ImGuiTableColumnFlags_WidthStretch, 5.f);
			ImGui::TableHeadersRow();
			for (auto& [id, buff] : activeBuffs_)
			{
				if (buff.first == 0 && hideInactive_ || hiddenBuffs_.count(id) > 0)
					continue;

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);

				ImGui::Text("%u", id);

				ImGui::TableNextColumn();

				ImGui::Text("%d", buff.first);

				ImGui::TableNextColumn();

				if (auto it = buffsMap_.find(id); it != buffsMap_.end())
					ImGui::TextUnformatted(it->second->name.c_str());
				else {
					auto& str = buffNames_[id];
					if (ImGui::InputText(std::format("##Name{}", id).c_str(), &str))
						SaveNames();
				}

				ImGui::TableNextColumn();

				const int hourMS = 1000 * 60 * 60;
				const int minuteMS = 1000 * 60;
				const int secondMS = 1000;

				int hours = buff.second / hourMS;
				int minutes = (buff.second - hours * hourMS) / minuteMS;
				int seconds = (buff.second - hours * hourMS - minutes * minuteMS) / secondMS;

				std::string timeLeft = "";
				if (hours > 0)
					timeLeft += std::format("{} hours", hours);
				if (minutes > 0)
					timeLeft += std::format(" {} minutes", minutes);
				if (seconds > 0)
					timeLeft += std::format(" {} seconds", seconds);

				ImGui::Text("%s", timeLeft.c_str());

				ImGui::TableNextColumn();

				byte chatCode[1 + 3 + 1];
				chatCode[0] = 0x06;
				chatCode[1 + 3] = 0x0;
				chatCode[1] = id & 0xFF;
				chatCode[2] = (id >> 8) & 0xFF;
				chatCode[3] = (id >> 16) & 0xFF;

				using base64 = cppcodec::base64_rfc4648;

				std::string chatCodeStr = std::format("[&{}]", base64::encode(chatCode));

				ImGui::TextUnformatted(chatCodeStr.c_str());
				ImGui::SameLine();
				if (ImGui::Button(("Copy##" + chatCodeStr).c_str()))
					ImGui::SetClipboardText(chatCodeStr.c_str());
				ImGui::SameLine();
				if (ImGui::Button(std::format("Say in G{}##{}", guildLogId_, chatCodeStr).c_str()))
				{
					mstime currentTime = TimeInMilliseconds();
					ImGui::SetClipboardText(std::format("/g{} {}: {}", guildLogId_, id, chatCodeStr).c_str());

					auto wait = [](int i) { std::this_thread::sleep_for(std::chrono::milliseconds(i)); };
					auto sendKeyEvent = [](uint vk, ScanCode sc, bool down) {
						INPUT i;
						ZeroMemory(&i, sizeof(INPUT));
						i.type = INPUT_KEYBOARD;
						i.ki.wVk = vk;
						i.ki.wScan = ScanCode_t(sc);
						i.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;

						SendInput(1, &i, sizeof(INPUT));
					};

					std::thread([=] {
						wait(100);

						sendKeyEvent(VK_RETURN, ScanCode::ENTER, true);
						wait(50);
						sendKeyEvent(VK_RETURN, ScanCode::ENTER, false);
						wait(100);

						sendKeyEvent(VK_CONTROL, ScanCode::CONTROLLEFT, true);
						wait(50);
						sendKeyEvent('V', ScanCode::V, true);
						wait(100);

						sendKeyEvent('V', ScanCode::V, false);
						wait(50);
						sendKeyEvent(VK_CONTROL, ScanCode::CONTROLLEFT, false);
						wait(100);

						sendKeyEvent(VK_RETURN, ScanCode::ENTER, true);
						wait(50);
						sendKeyEvent(VK_RETURN, ScanCode::ENTER, false);
						}).detach();
				}
				ImGui::SameLine();
				if (ImGui::Button(("Wiki##" + chatCodeStr).c_str()))
				{
					ShellExecute(0, 0, std::format(L"https://wiki.guildwars2.com/index.php?title=Special:Search&search={}", utf8_decode(skyr::percent_encode(chatCodeStr))).c_str(), 0, 0, SW_SHOW);
				}
			}
			ImGui::EndTable();
		}
#endif
	}
	ImGui::End();

	glm::vec2 screen{ ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y };
	glm::vec2 mouse{ ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y };
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
	if (ImGui::Begin("Display Area", nullptr, InvisibleWindowFlags))
	{
		auto* font = Core::i().fontBuffCounter();
		ImGui::PushFont(font);
		float origFontScale = font->Scale;

		auto drawItem = [&](const Grid& g, const Item& i, int count) {
			if (count == 0 && i.inactiveAlpha < 1.f / 255.f)
				return;

			glm::vec2 pos = (g.attached && !placingItem_ ? mouse : screen * 0.5f) + glm::vec2(i.pos * g.spacing);

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
		};

		for (const auto& [gid, g] : grids_)
		{
			for (const auto& [iid, i] : g.items)
			{
				int count = activeBuffs_[i.buff->id].first;
				if (placingItem_ && selectedGridId_ == gid && selectedItemId_ == iid)
					count = std::max(count, 1);

				drawItem(g, i, count);
			}
			if (placingItem_ && selectedGridId_ == gid && selectedItemId_ == NewId)
				drawItem(g, creatingItem_, 1);
		}
		font->Scale = origFontScale;
		ImGui::PopFont();
	}
	ImGui::End();
}

void Buffs::UpdateBuffsTable(StackedBuff* buffs)
{
#ifdef _DEBUG
	for (auto& b : activeBuffs_)
		b.second = { 0, 0 };
#else
	activeBuffs_.clear();
#endif
	for (size_t i = 0; buffs[i].id; i++)
		activeBuffs_[buffs[i].id] = { buffs[i].count, buffs[i].maxDuration };
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