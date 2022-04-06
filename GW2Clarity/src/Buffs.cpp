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
#include <ranges>
#include <algorithm>
#include <range/v3/all.hpp>

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

std::vector<glm::vec4> GenerateNumbersMap()
{
	return {
		{},
		{},
		{ 0.0015337423, 0.0015337423, 0.19785276, 0.19785276 },
		{ 0.20092024, 0.0015337423, 0.39723927, 0.19785276 },
		{ 0.40030676, 0.0015337423, 0.59662575, 0.19785276 },
		{ 0.59969324, 0.0015337423, 0.7960123, 0.19785276 },
		{ 0.7990798, 0.0015337423, 0.99539876, 0.19785276 },
		{ 0.0015337423, 0.20092024, 0.19785276, 0.39723927 },
		{ 0.20092024, 0.20092024, 0.39723927, 0.39723927 },
		{ 0.40030676, 0.20092024, 0.59662575, 0.39723927 },
		{ 0.59969324, 0.20092024, 0.7960123, 0.39723927 },
		{ 0.7990798, 0.20092024, 0.99539876, 0.39723927 },
		{ 0.0015337423, 0.40030676, 0.19785276, 0.59662575 },
		{ 0.20092024, 0.40030676, 0.39723927, 0.59662575 },
		{ 0.40030676, 0.40030676, 0.59662575, 0.59662575 },
		{ 0.59969324, 0.40030676, 0.7960123, 0.59662575 },
		{ 0.7990798, 0.40030676, 0.99539876, 0.59662575 },
		{ 0.0015337423, 0.59969324, 0.19785276, 0.7960123 },
		{ 0.20092024, 0.59969324, 0.39723927, 0.7960123 },
		{ 0.40030676, 0.59969324, 0.59662575, 0.7960123 },
		{ 0.59969324, 0.59969324, 0.7960123, 0.7960123 },
		{ 0.7990798, 0.59969324, 0.99539876, 0.7960123 },
		{ 0.0015337423, 0.7990798, 0.19785276, 0.99539876 },
		{ 0.20092024, 0.7990798, 0.39723927, 0.99539876 },
		{ 0.40030676, 0.7990798, 0.59662575, 0.99539876 },
		{ 0.59969324, 0.7990798, 0.7960123, 0.99539876 },
	    { 0.7990798, 0.7990798, 0.99539876, 0.99539876 },
	};
}

Buffs::Buffs(ComPtr<ID3D11Device>& dev)
	: buffs_(GenerateBuffsList())
	, buffsMap_(GenerateBuffsMap(buffs_))
	, numbersMap_(GenerateNumbersMap())
{
	buffsAtlas_ = CreateTextureFromResource(dev.Get(), Core::i().dllModule(), IDR_BUFFS);
	numbersAtlas_ = CreateTextureFromResource(dev.Get(), Core::i().dllModule(), IDR_NUMBERS);

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
	mstime currentTime = TimeInMilliseconds();
	if (needsSaving_ && lastSaveTime_ + SaveDelay <= currentTime)
		Save();

	if (ImGui::Begin("Buffs Management"))
	{
		if (draggingGridScale_ || placingItem_)
		{
			const auto& sp = getG(selectedId_).spacing;
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
			if(!draggingGridScale_)
				needsSaving_ = true;
		}

		if (placingItem_)
		{
			const auto& sp = getG(selectedId_).spacing;
			auto& item = getI(selectedId_);
			auto mouse = ImGui::GetIO().MousePos - ImGui::GetIO().DisplaySize * 0.5f;

			item.pos = glm::ivec2(glm::floor(glm::vec2(mouse.x, mouse.y) / glm::vec2(sp)));

			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				placingItem_ = false;
				needsSaving_ = true;
			}
		}

		for (auto&& [gid, g] : grids_ | ranges::views::enumerate)
		{
			auto u = Unselected(gid);
			if (ImGui::Selectable(std::format("{} ({}x{})##{}", g.name, g.spacing.x, g.spacing.y, gid).c_str(), selectedId_ == u))
				selectedId_ = u;

			ImGui::Indent();
			for (auto&& [iid, i] : g.items | ranges::views::enumerate)
			{
				Id id{ gid, iid };
				if (ImGui::Selectable(std::format("{} ({}, {})##{}", i.buff->name.c_str(), i.pos.x, i.pos.y, iid).c_str(), selectedId_ == id))
					selectedId_ = id;
			}

			auto n = New(gid);
			if (ImGui::Selectable(std::format("+ New Item##{}", gid).c_str(), selectedId_.grid == gid && selectedId_ == n))
				selectedId_ = n;
			ImGui::Unindent();
		}
		{
			auto n = New();
			if (ImGui::Selectable("+ New Grid", selectedId_ == n))
				selectedId_ = n;
		}

		auto saveCheck = [this](bool changed) { needsSaving_ = needsSaving_ || changed; return changed; };

		bool editingGrid = selectedId_.grid >= 0 && selectedId_.item == UnselectedSubId;
		bool editingItem = selectedId_.item >= 0;
		if (editingGrid || selectedId_.grid == NewSubId)
		{
			ImGui::Separator();

			auto& editGrid = editingGrid ? getG(selectedId_) : creatingGrid_;
			if (editingGrid)
				ImGuiTitle(std::format("Editing Grid '{}'", editGrid.name).c_str());
			else
				ImGuiTitle("New Grid");

			saveCheck(ImGui::Checkbox("Square Grid", &editGrid.square));
			if (editGrid.square && saveCheck(ImGui::DragInt("Grid Scale", (int*)&editGrid.spacing, 0.1f, 1, 2048)) ||
				!editGrid.square && saveCheck(ImGui::DragInt2("Grid Scale", glm::value_ptr(editGrid.spacing), 0.1f, 1, 2048)))
			{
				draggingGridScale_ = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
			}
			if (editGrid.square)
				editGrid.spacing.y = editGrid.spacing.x;

			saveCheck(ImGui::InputText("Grid Name", &editGrid.name));
			saveCheck(ImGui::Checkbox("Attached to Mouse", &editGrid.attached));

			if (selectedId_.grid == NewSubId && ImGui::Button("Create Grid"))
			{
				grids_.push_back(creatingGrid_);
				creatingGrid_ = {};
				selectedId_ = Unselected();
				needsSaving_ = true;
			}
		}

		if (editingItem || selectedId_.item == NewSubId)
		{
			auto& editItem = editingItem ? getI(selectedId_) : creatingItem_;
			if (editingItem)
				ImGuiTitle(std::format("Editing Item '{}' of '{}'", editItem.buff->name, getG(selectedId_).name).c_str());
			else
				ImGuiTitle(std::format("New Item in '{}'", getG(selectedId_).name).c_str());

			ImGui::Separator();

			auto buffCombo = [&](auto& buff, int id)
			{
				if (ImGui::BeginCombo(std::format("Buff##{}", id).c_str(), buff->name.c_str()))
				{
					for (auto& b : buffs_)
					{
						ImGui::Image(buffsAtlas_.srv.Get(), ImVec2(32, 32), ToImGui(b.uv.xy), ToImGui(b.uv.zw));
						ImGui::SameLine();
						if (saveCheck(ImGui::Selectable(b.name.c_str(), false)))
							buff = &b;
					}
					ImGui::EndCombo();
				}
			};

			buffCombo(editItem.buff, -1);
			int removeId = -1;
			for (auto&& [n, extraBuff] : editItem.additionalBuffs | ranges::views::enumerate)
			{
				buffCombo(extraBuff, n);
				ImGui::SameLine();
				if (ImGuiClose(std::format("RemoveExtraBuff{}", n).c_str()))
					removeId = n;
			}
			if (removeId != -1)
				editItem.additionalBuffs.erase(editItem.additionalBuffs.begin() + removeId);

			if (ImGui::Button("Add Tracked Buff"))
				editItem.additionalBuffs.push_back(&UnknownBuff);

			saveCheck(ImGui::DragInt2("Location", glm::value_ptr(editItem.pos), 0.1f));
			ImGui::SameLine();
			if (ImGui::Button("Place"))
				placingItem_ = true;

			ImGui::Spacing();
			ImGui::PushFont(Core::i().fontBold());
			ImGui::TextUnformatted("Thresholds");
			ImGui::PopFont();
			ImGui::TextUnformatted("Simulate thresholds using ");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(100.f);
			ImGui::DragInt("stacks##Simulatedstacks", &editingItemFakeCount_, 0.1f, 0, 25);
			for (size_t i = 0; i < editItem.thresholds.size(); i++)
			{
				auto& th = editItem.thresholds[i];
				ImGui::TextUnformatted("When less than ");
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100.f);
				saveCheck(ImGui::DragInt(std::format("##Threshold{}", i).c_str(), &th.threshold, 0.1f, 0, 25));
				ImGui::SameLine();
				ImGui::TextUnformatted("stacks, apply tint ");
				ImGui::SameLine();
				saveCheck(ImGui::ColorEdit4(std::format("##ThresholdColor{}", i).c_str(), &th.tint.x, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs));
				ImGui::Spacing();
			}
			if (saveCheck(ImGui::Button("Add threshold")))
				editItem.thresholds.emplace_back();

			std::ranges::sort(editItem.thresholds, [](auto& a, auto& b) { return a.threshold < b.threshold; });

			ImGui::Separator();

			if (selectedId_.item == NewSubId && ImGui::Button("Create Item"))
			{
				getG(selectedId_).items.push_back(creatingItem_);
				creatingItem_ = {};
				selectedId_ = Unselected();
				needsSaving_ = true;
			}
		}

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
		auto drawItem = [&](const Grid& g, const Item& i, int count, bool editing) {

			auto thresh = std::ranges::find_if(i.thresholds, [=](const auto& t) { return count < t.threshold; });
			ImVec4 tint(1, 1, 1, 1);
			if (thresh != i.thresholds.end())
				tint = thresh->tint;

			glm::vec2 pos = (g.attached && !placingItem_ ? mouse : screen * 0.5f) + glm::vec2(i.pos * g.spacing);

			auto adj = AdjustToArea(128, 128, g.spacing.x);

			if (editing)
				ImGui::GetWindowDrawList()->AddRect(ToImGui(pos), ToImGui(pos) + adj, 0xFF0000FF);

			ImGui::SetCursorPos(ToImGui(pos));
			ImGui::Image(buffsAtlas_.srv.Get(), adj, ToImGui(i.buff->uv.xy), ToImGui(i.buff->uv.zw), tint);
			if (count > 1)
			{
				count = std::min(count, int(numbersMap_.size()) - 1);
				ImGui::SetCursorPos(ToImGui(pos));
				ImGui::Image(numbersAtlas_.srv.Get(), adj, ToImGui(numbersMap_[count].xy), ToImGui(numbersMap_[count].zw));
			}
		};

		for (const auto& [gid, g] : grids_ | ranges::views::enumerate)
		{
			for (const auto& [iid, i] : g.items | ranges::views::enumerate)
			{
				bool editing = selectedId_ == Id{ gid, iid };
				int count = 0;
				if (editing)
					count = editingItemFakeCount_;
				else
					count = std::accumulate(i.additionalBuffs.begin(), i.additionalBuffs.end(), activeBuffs_[i.buff->id].first, [&](int a, const Buff* b) { return a + activeBuffs_[b->id].first; });

				drawItem(g, i, count, editing);
			}
			if (selectedId_ == New(gid))
				drawItem(g, creatingItem_, editingItemFakeCount_, true);
		}
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
	using namespace nlohmann;
	grids_.clear();
	selectedId_ = Unselected();

	auto& cfg = JSONConfigurationFile::i();
	cfg.Reload();

	auto getivec2 = [](const auto& j) { return glm::ivec2(j[0].get<int>(), j[1].get<int>()); };
	auto getImVec4 = [](const auto& j) { return ImVec4(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>()); };

	const auto& grids = cfg.json()["buff_grids"];
	for (const auto& gIn : grids)
	{
		Grid g;
		g.spacing = getivec2(gIn["spacing"]);
		g.attached = gIn["attached"];
		g.square = gIn["square"];
		g.name = gIn["name"];

		for (const auto& iIn : gIn["items"])
		{
			Item i;
			i.pos = getivec2(iIn["pos"]);
			i.buff = buffsMap_.at(iIn["buff_id"]);

			if(iIn.contains("additional_buff_ids"))
				for (auto& bIn : iIn["additional_buff_ids"])
					if(const Buff* b = buffsMap_.at(bIn); b)
						i.additionalBuffs.push_back(b);

			if (iIn.contains("thresholds"))
				for (auto& tIn : iIn["thresholds"])
					i.thresholds.emplace_back(tIn["threshold"], getImVec4(tIn["tint"]));

			g.items.push_back(i);
		}
		grids_.push_back(g);
	}
}

void Buffs::Save()
{
	using namespace nlohmann;

	auto& cfg = JSONConfigurationFile::i();
	auto& grids = cfg.json()["buff_grids"];
	grids = json::array();
	for (const auto& g : grids_)
	{
		json grid;
		grid["spacing"] = { g.spacing.x, g.spacing.y };
		grid["attached"] = g.attached;
		grid["square"] = g.square;
		grid["name"] = g.name;

		json& gridItems = grid["items"];

		for (const auto& i : g.items)
		{
			json item;
			item["pos"] = { i.pos.x, i.pos.y };
			item["buff_id"] = i.buff->id;

			if (!i.thresholds.empty())
			{
				json thresholds = json::array();
				for (auto& t : i.thresholds)
					thresholds.push_back({ { "threshold", t.threshold }, { "tint", { t.tint.x, t.tint.y, t.tint.z, t.tint.w } } });
				item["thresholds"] = thresholds;
			}

			if (!i.additionalBuffs.empty())
			{
				json buffs = json::array();
				for (const auto* b : i.additionalBuffs)
					buffs.push_back(b->id);
				item["additional_buff_ids"] = buffs;
			}

			gridItems.push_back(item);
		}

		grids.push_back(grid);
	}

	cfg.Save();

	needsSaving_ = false;
	lastSaveTime_ = TimeInMilliseconds();
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