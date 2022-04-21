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
	, changeGridSetKey_("change_grid_set", "Change Grid Set", "General")
#ifdef _DEBUG
	, showAnalyzerKey_("show_analyzer", "Show Analyzer", "General", ScanCode::X, Modifier::CTRL | Modifier::SHIFT, false)
#endif
{
	buffsAtlas_ = CreateTextureFromResource(dev.Get(), Core::i().dllModule(), IDR_BUFFS);
	numbersAtlas_ = CreateTextureFromResource(dev.Get(), Core::i().dllModule(), IDR_NUMBERS);

	changeGridSetKey_.callback([&](Activated a) {
		if (a && !showSetSelector_)
		{
			ImGui::SetWindowPos(ChangeSetPopupName, ImGui::GetIO().MousePos);
			showSetSelector_ = true;
			return true;
		}
		else
			return false;
	});

#ifdef _DEBUG
	showAnalyzerKey_.callback([&](Activated a) {
		if (a && !showAnalyzer_)
		{
			showAnalyzer_ = true;
			return true;
		}
		else
			return false;
});
#endif

	Load();
#ifdef _DEBUG
	LoadNames();
#endif

	SettingsMenu::i().AddImplementer(this);
}

Buffs::~Buffs()
{
	SettingsMenu::f([&](auto& i) { i.RemoveImplementer(this); });
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

void Buffs::DrawBuffAnalyzer()
{
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
}
#endif

void Buffs::DrawEditingGrid()
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
		if (!draggingGridScale_)
			needsSaving_ = true;
	}
}

void Buffs::PlaceItem()
{
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
}

void Buffs::DrawGridList()
{
	for (auto&& [gid, g] : grids_ | ranges::views::enumerate)
	{
		auto u = Unselected(gid);
		if (ImGui::Selectable(std::format("{} ({}x{})##{}", g.name, g.spacing.x, g.spacing.y, gid).c_str(), selectedId_ == u))
		{
			selectedId_ = u;
			selectedSetId_ = UnselectedSubId;
		}

		ImGui::Indent();
		for (auto&& [iid, i] : g.items | ranges::views::enumerate)
		{
			Id id{ gid, iid };
			if (ImGui::Selectable(std::format("{} ({}, {})##{}", i.buff->name.c_str(), i.pos.x, i.pos.y, iid).c_str(), selectedId_ == id))
			{
				selectedId_ = id;
				selectedSetId_ = UnselectedSubId;
			}
		}

		auto n = New(gid);
		if (ImGui::Selectable(std::format("+ New Item##{}", gid).c_str(), selectedId_.grid == gid && selectedId_ == n))
		{
			selectedId_ = n;
			selectedSetId_ = UnselectedSubId;
		}
		ImGui::Unindent();
	}
	{
		auto n = New();
		if (ImGui::Selectable("+ New Grid", selectedId_ == n))
		{
			selectedId_ = n;
			selectedSetId_ = UnselectedSubId;
		}
	}
}

void Buffs::DrawItems()
{
	bool editMode = selectedId_.grid != UnselectedSubId;

	if (currentSetId_ != UnselectedSubId || editMode)
	{
		if(!sets_[currentSetId_].combatOnly || MumbleLink::i().isInCombat())
		{
			glm::vec2 screen{ ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y };
			glm::vec2 mouse{ ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y };
			ImGui::SetNextWindowPos(ImVec2(0, 0));
			ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
			if (ImGui::Begin("Display Area", nullptr, InvisibleWindowFlags))
			{
				struct NumberDraw
				{
					ImVec2 adj;
					ImVec2 pos;
					ImVec2 uv1;
					ImVec2 uv2;
				};
				std::vector<NumberDraw> delayedDraws;

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
						delayedDraws.emplace_back(adj, ToImGui(pos), ToImGui(numbersMap_[count].xy), ToImGui(numbersMap_[count].zw));
					}
				};

				auto drawGrid = [&](Grid& g, short gid)
				{
					for (const auto& [iid, i] : g.items | ranges::views::enumerate)
					{
						bool editing = editMode && selectedId_ == Id{ gid, short(iid) };
						int count = 0;
						if (editing)
							count = editingItemFakeCount_;
						else
							count = std::accumulate(i.additionalBuffs.begin(), i.additionalBuffs.end(), activeBuffs_[i.buff->id].first, [&](int a, const Buff* b) { return a + activeBuffs_[b->id].first; });

						drawItem(g, i, count, editing);
					}
					if (editMode && selectedId_ == New(gid))
						drawItem(g, creatingItem_, editingItemFakeCount_, true);
				};

				if (editMode)
					drawGrid(getG(selectedId_), selectedId_.grid);
				else
					for (int gid : sets_[currentSetId_].grids)
						drawGrid(grids_[gid], UnselectedSubId);

				for (const auto& dd : delayedDraws)
				{
					ImGui::SetCursorPos(dd.pos);
					ImGui::Image(numbersAtlas_.srv.Get(), dd.adj, dd.uv1, dd.uv2);
				}
			}
			ImGui::End();
		}
	}
}

void Buffs::Delete(Id& id)
{
	if (id.item >= 0)
	{
		auto& g = getG(id);
		g.items.erase(g.items.begin() + id.item);
		id = Unselected();

		needsSaving_ = true;
	}
	else
	{
		grids_.erase(grids_.begin() + id.grid);

		for (auto& s : sets_)
		{
			std::vector<int> toadd;

			s.grids.erase(id.grid);
			for (auto it = s.grids.begin(); it != s.grids.end();)
			{
				int gid = *it;
				if (gid > id.grid)
				{
					it = s.grids.erase(it);
					toadd.push_back(gid - 1);
				}
				else
					it++;
			}

			for (int i : toadd)
				s.grids.insert(i);
		}

		id = Unselected();
		needsSaving_ = true;
	}
}

void Buffs::DrawMenu(Keybind** currentEditedKeybind)
{
	DrawEditingGrid();
	PlaceItem();

	auto saveCheck = [this](bool changed) { needsSaving_ = needsSaving_ || changed; return changed; };

	{
		ImGuiTitle("Grid Editor");

		DrawGridList();

		if (ImGui::BeginPopupModal("Confirm Deletion"))
		{
			bool isSet = selectedSetId_ >= 0 && selectedId_ == Unselected();
			bool isItem = !isSet && selectedId_.item >= 0;
			const auto& name = isSet ? sets_[selectedSetId_].name : isItem ? getI(selectedId_).buff->name : getG(selectedId_).name;
			const char* type = isSet ? "set" : isItem ? "item" : "grid";
			ImGui::TextUnformatted(std::format("Are you sure you want to delete {} '{}'{}?", type, name, isItem ? std::format(" from grid '{}'", getG(selectedId_).name) : "").c_str());
			if (ImGui::Button("Yes"))
			{
				if (isSet)
				{
					sets_.erase(sets_.begin() + selectedSetId_);
					selectedSetId_ = UnselectedSubId;
					needsSaving_ = true;
				}
				else
					Delete(selectedId_);
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}

		bool editingGrid = selectedId_.grid >= 0 && selectedId_.item == UnselectedSubId;
		bool editingItem = selectedId_.item >= 0;
		if (editingGrid || selectedId_.grid == NewSubId)
		{
			auto& editGrid = editingGrid ? getG(selectedId_) : creatingGrid_;
			if (editingGrid)
				ImGuiTitle(std::format("Editing Grid '{}'", editGrid.name).c_str(), 0.75f);
			else
				ImGuiTitle("New Grid", 0.75f);

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

			if (selectedId_.grid >= 0 && ImGui::Button("Delete Grid"))
				ImGui::OpenPopup("Confirm Deletion");
		}
		else if (editingItem || selectedId_.item == NewSubId)
		{
			auto& editItem = editingItem ? getI(selectedId_) : creatingItem_;
			if (editingItem)
				ImGuiTitle(std::format("Editing Item '{}' of '{}'", editItem.buff->name, getG(selectedId_).name).c_str(), 0.75f);
			else
				ImGuiTitle(std::format("New Item in '{}'", getG(selectedId_).name).c_str(), 0.75f);

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

			if (selectedId_.item >= 0 && ImGui::Button("Delete Item"))
				ImGui::OpenPopup("Confirm Deletion");
		}
	}
	{
		ImGuiTitle("Set Editor");
		for (auto&& [sid, s] : sets_ | ranges::views::enumerate)
		{
			if (ImGui::Selectable(std::format("{}##Set", s.name).c_str(), selectedSetId_ == sid))
			{
				selectedSetId_ = sid;
				selectedId_ = Unselected();
			}
		}
		if (ImGui::Selectable("+ Add Set", selectedSetId_ == NewSubId))
		{
			selectedSetId_ = NewSubId;
			selectedId_ = Unselected();
		}

		bool editingSet = selectedSetId_ != NewSubId;
		if (selectedSetId_ != UnselectedSubId)
		{
			auto& editSet = selectedSetId_ == NewSubId ? creatingSet_ : sets_[selectedSetId_];
			if (editingSet)
				ImGuiTitle(std::format("Editing Set '{}'", editSet.name).c_str(), 0.75f);
			else
				ImGuiTitle("New Set", 0.75f);
			
			saveCheck(ImGui::InputText("Name##NewSet", &editSet.name));
			saveCheck(ImGui::Checkbox("Show in Combat Only##NewSet", &editSet.combatOnly));

			ImGui::Separator();

			for (auto&& [gid, g] : grids_ | ranges::views::enumerate)
			{
				bool sel = editSet.grids.count(gid) > 0;
				if (saveCheck(ImGui::Checkbox(std::format("{}##GridInSet", g.name).c_str(), &sel)))
				{
					if (sel)
						editSet.grids.insert(gid);
					else
						editSet.grids.erase(gid);
				}
			}


			if (selectedSetId_ == NewSubId && ImGui::Button("Create Set"))
			{
				sets_.push_back(creatingSet_);
				creatingSet_ = {};
				selectedSetId_ = UnselectedSubId;
				needsSaving_ = true;
			}
			else if(selectedSetId_ >= 0 && ImGui::Button("Delete Set"))
				ImGui::OpenPopup("Confirm Deletion");
		}
	}

	ImGuiKeybindInput(changeGridSetKey_, currentEditedKeybind, "Displays the Quick Set menu to change what buffs are displayed.");

	mstime currentTime = TimeInMilliseconds();
	if (needsSaving_ && lastSaveTime_ + SaveDelay <= currentTime)
		Save();
}

void Buffs::Draw(ComPtr<ID3D11DeviceContext>& ctx)
{
	if (!SettingsMenu::i().isVisible())
		selectedId_ = Unselected();

	if(showSetSelector_)
	{
		if (ImGui::Begin(ChangeSetPopupName, nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
		{
			if (sets_.empty())
				showSetSelector_ = false;

			if(!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && ImGui::IsMouseDown(ImGuiMouseButton_Left))
				showSetSelector_ = false;

			for (auto&& [i, s] : sets_ | ranges::views::enumerate)
			{
				if (ImGui::Selectable(s.name.c_str(), currentSetId_ == i))
				{
					currentSetId_ = i;
					showSetSelector_ = false;
				}
			}

			if (ImGui::Selectable("None", currentSetId_ == UnselectedSubId))
			{
				currentSetId_ = UnselectedSubId;
				showSetSelector_ = false;
			}

			if (ImGui::IsKeyPressed(ImGuiKey_Escape))
				showSetSelector_ = false;
		}
		ImGui::End();
	}

	DrawItems();

#ifdef _DEBUG
	if (ImGui::Begin("Buffs Analyzer", &showAnalyzer_))
		DrawBuffAnalyzer();
	ImGui::End();
#endif
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

	const auto& sets = cfg.json()["buff_sets"];
	for (const auto& sIn : sets)
	{
		Set s;
		s.name = sIn["name"];
		s.combatOnly = sIn["combat_only"];

		for (const auto& gIn : sIn["grids"])
		{
			int id = gIn;
			if (id < grids_.size())
				s.grids.insert(id);
		}

		sets_.push_back(s);
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

	auto& sets = cfg.json()["buff_sets"];
	sets = json::array();
	for (const auto& s : sets_)
	{
		json set;
		set["name"] = s.name;
		set["combat_only"] = s.combatOnly;

		json& setGrids = set["grids"];

		for (int i : s.grids)
			setGrids.push_back(i);

		sets.push_back(set);
	}

	cfg.Save();

	needsSaving_ = false;
	lastSaveTime_ = TimeInMilliseconds();
}

static const std::vector<Buff> g_Buffs {
	// Common Buffs
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
	{ 873, "Retaliation" },

	// Conditions
	{ 736, "Bleeding" },
	{ 737, "Burning" },
	{ 861, "Confusion" },
	{ 723, "Poison" },
	{ 19426, "Torment" },
	{ 720, "Blind" },
	{ 722, "Chilled" },
	{ 721, "Crippled" },
	{ 791, "Fear" },
	{ 727, "Immobile" },
	{ 26766, "Slow" },
	{ 742, "Weakness" },
	{ 27705, "Taunt" },
	{ 738, "Vulnerability" },

	// Special
	{ 46668, "Diminished" }, // https://wiki.guildwars2.com/images/7/71/Diminished.png
	{ 46587, "Malnourished" }, // https://wiki.guildwars2.com/images/6/67/Malnourished.png
	{ 770, "Downed" }, // https://wiki.guildwars2.com/images/d/dd/Downed.png
	{ 46842, "Exhaustion" }, // https://wiki.guildwars2.com/images/8/88/Exhaustion.png
	{ 13017, "Stealth" }, // https://wiki.guildwars2.com/images/1/19/Stealth.png
	{ 10269, "Hide in Shadows" }, // https://wiki.guildwars2.com/images/1/19/Stealth.png
	{ 890, "Revealed" }, // https://wiki.guildwars2.com/images/d/db/Revealed.png
	{ 5974, "Superspeed" }, // https://wiki.guildwars2.com/images/c/c8/Superspeed.png
	{ 5974, "Superspeed" }, // https://wiki.guildwars2.com/images/c/c8/Superspeed.png
	{ 762, "Determined (762)" }, // https://wiki.guildwars2.com/images/e/eb/Determined.png
	{ 788, "Determined (788)" }, // https://wiki.guildwars2.com/images/e/eb/Determined.png
	{ 848, "Resurrection" }, // https://wiki.guildwars2.com/images/e/eb/Determined.png
	{ 895, "Determined (895)" }, // https://wiki.guildwars2.com/images/e/eb/Determined.png
	{ 3892, "Determined (3892)" }, // https://wiki.guildwars2.com/images/e/eb/Determined.png
	{ 31450, "Determined (31450)" }, // https://wiki.guildwars2.com/images/e/eb/Determined.png
	{ 52271, "Determined (52271)" }, // https://wiki.guildwars2.com/images/e/eb/Determined.png
	{ 757, "Invulnerability (757)" }, // https://wiki.guildwars2.com/images/e/eb/Determined.png
	{ 56227, "Invulnerability (56227)" }, // https://wiki.guildwars2.com/images/e/eb/Determined.png
	{ 801, "Invulnerability (801)" }, // https://wiki.guildwars2.com/images/e/eb/Determined.png
	{ 34113, "Spawn Protection?" }, // https://wiki.guildwars2.com/images/e/eb/Determined.png
	{ 872, "Stun" }, // https://wiki.guildwars2.com/images/9/97/Stun.png
	{ 833, "Daze" }, // https://wiki.guildwars2.com/images/7/79/Daze.png
	{ 48209, "Exposed (48209)" }, // https://wiki.guildwars2.com/images/6/6b/Exposed.png
	{ 31589, "Exposed (31589)" }, // https://wiki.guildwars2.com/images/6/6b/Exposed.png
	{ 64936, "Exposed (64936)" }, // https://wiki.guildwars2.com/images/6/6b/Exposed.png
	{ 36781, "Unblockable" }, // https://wiki.guildwars2.com/images/f/f0/Unblockable_%28effect%29.png
	{ 1159, "Encumbered" }, // https://wiki.guildwars2.com/images/d/d7/Encumbered.png
	{ 27048, "Celeritas Spores" }, // https://wiki.guildwars2.com/images/7/7b/Blazing_Speed_Mushrooms.png
	{ 52182, "Branded Accumulation" }, // https://wiki.guildwars2.com/images/5/55/Achilles_Bane.png
	// Auras
	{ 10332, "Chaos Aura" }, // https://wiki.guildwars2.com/images/e/ec/Chaos_Aura.png
	{ 5677, "Fire Aura" }, // https://wiki.guildwars2.com/images/c/ce/Fire_Aura.png
	{ 5579, "Frost Aura" }, // https://wiki.guildwars2.com/images/8/87/Frost_Aura_%28effect%29.png
	{ 25518, "Light Aura" }, // https://wiki.guildwars2.com/images/5/5a/Light_Aura.png
	{ 5684, "Magnetic Aura" }, // https://wiki.guildwars2.com/images/0/0b/Magnetic_Aura_%28effect%29.png
	{ 5577, "Shocking Aura" }, // https://wiki.guildwars2.com/images/5/5d/Shocking_Aura_%28effect%29.png
	{ 39978, "Dark Aura" }, // https://wiki.guildwars2.com/images/e/ef/Dark_Aura.png
	// Racial
	{ 12459, "Take Root" }, // https://wiki.guildwars2.com/images/b/b2/Take_Root.png
	{ 12426, "Become the Bear" }, // https://wiki.guildwars2.com/images/7/7e/Become_the_Bear.png
	{ 12405, "Become the Raven" }, // https://wiki.guildwars2.com/images/2/2c/Become_the_Raven.png
	{ 12400, "Become the Snow Leopard" }, // https://wiki.guildwars2.com/images/7/78/Become_the_Snow_Leopard.png
	{ 12393, "Become the Wolf" }, // https://wiki.guildwars2.com/images/f/f1/Become_the_Wolf.png
	{ 12368, "Avatar of Melandru" }, // https://wiki.guildwars2.com/images/3/30/Avatar_of_Melandru.png
	{ 12326, "Power Suit" }, // https://wiki.guildwars2.com/images/8/89/Summon_Power_Suit.png
	{ 12366, "Reaper of Grenth" }, // https://wiki.guildwars2.com/images/0/07/Reaper_of_Grenth.png
	{ 43503, "Charrzooka" }, // https://wiki.guildwars2.com/images/1/17/Charrzooka.png
	//
	{ 33833, "Guild Item Research" }, // https://wiki.guildwars2.com/images/c/c6/Guild_Magic_Find_Banner_Boost.png
	//
	{ 37657, "Crystalline Heart" }, // https://wiki.guildwars2.com/images/5/56/Crystalline_Heart.png
	// WvW
	{ 14772, "Minor Borderlands Bloodlust" }, // https://wiki.guildwars2.com/images/f/f7/Major_Borderlands_Bloodlust.png
	{ 14773, "Major Borderlands Bloodlust" }, // https://wiki.guildwars2.com/images/f/f7/Major_Borderlands_Bloodlust.png
	{ 14774, "Superior Borderlands Bloodlust" }, // https://wiki.guildwars2.com/images/f/f7/Major_Borderlands_Bloodlust.png
	{ 33120, "Blessing of Elements" }, // https://wiki.guildwars2.com/images/3/3c/Blessing_of_Air.png
	{ 34031, "Flame's Embrace" }, // https://wiki.guildwars2.com/images/5/53/Flame%27s_Embrace.png

	{ 33719, "Sigil of Concentration" }, // https://wiki.guildwars2.com/images/b/b3/Superior_Sigil_of_Concentration.png
	{ 53285, "Superior Rune of the Monk" }, // https://wiki.guildwars2.com/images/1/18/Superior_Rune_of_the_Monk.png
	{ 9374, "Sigil of Corruption" }, // https://wiki.guildwars2.com/images/1/18/Superior_Sigil_of_Corruption.png
	{ 9386, "Sigil of Life" }, // https://wiki.guildwars2.com/images/a/a7/Superior_Sigil_of_Life.png
	{ 9385, "Sigil of Perception" }, // https://wiki.guildwars2.com/images/c/cc/Superior_Sigil_of_Perception.png
	{ 9286, "Sigil of Bloodlust" }, // https://wiki.guildwars2.com/images/f/fb/Superior_Sigil_of_Bloodlust.png
	{ 38588, "Sigil of Bounty" }, // https://wiki.guildwars2.com/images/f/f8/Superior_Sigil_of_Bounty.png
	{ 9398, "Sigil of Benevolence" }, // https://wiki.guildwars2.com/images/5/59/Superior_Sigil_of_Benevolence.png
	{ 22144, "Sigil of Momentum" }, // https://wiki.guildwars2.com/images/3/30/Superior_Sigil_of_Momentum.png
	{ 46953, "Sigil of the Stars" }, // https://wiki.guildwars2.com/images/d/dc/Superior_Sigil_of_the_Stars.png
	{ 43930, "Sigil of Severance" }, // https://wiki.guildwars2.com/images/c/c2/Superior_Sigil_of_Severance.png
	{ 9441, "Sigil of Doom" }, // https://wiki.guildwars2.com/images/6/67/Superior_Sigil_of_Doom.png

	{ 36341, "Mistlock Instability: Adrenaline Rush" }, // https://wiki.guildwars2.com/images/7/72/Mistlock_Instability_Adrenaline_Rush.png
	{ 22228, "Mistlock Instability: Afflicted" }, // https://wiki.guildwars2.com/images/3/3f/Mistlock_Instability_Afflicted.png
	{ 53673, "Mistlock Instability: Boon Overload" }, // https://wiki.guildwars2.com/images/d/d7/Mistlock_Instability_Boon_Overload.png
	{ 36386, "Mistlock Instability: Flux Bomb" }, // https://wiki.guildwars2.com/images/3/3f/Mistlock_Instability_Flux_Bomb.png
	{ 48296, "Mistlock Instability: Fractal Vindicators" }, // https://wiki.guildwars2.com/images/4/48/Mistlock_Instability_Fractal_Vindicators.png
	{ 54477, "Mistlock Instability: Frailty" }, // https://wiki.guildwars2.com/images/d/d6/Mistlock_Instability_Frailty.png
	{ 47323, "Mistlock Instability: Hamstrung" }, // https://wiki.guildwars2.com/images/9/99/Mistlock_Instability_Hamstrung.png
	{ 22293, "Mistlock Instability: Last Laugh" }, // https://wiki.guildwars2.com/images/5/58/Mistlock_Instability_Last_Laugh.png
	{ 36224, "Mistlock Instability: Mists Convergence" }, // https://wiki.guildwars2.com/images/9/95/Mistlock_Instability_Mists_Convergence.png
	{ 22277, "Mistlock Instability: No Pain, No Gain" }, // https://wiki.guildwars2.com/images/c/c3/Mistlock_Instability_No_Pain%2C_No_Gain.png
	{ 54084, "Mistlock Instability: Outflanked" }, // https://wiki.guildwars2.com/images/0/0c/Mistlock_Instability_Outflanked.png
	{ 32942, "Mistlock Instability: Social Awkwardness" }, // https://wiki.guildwars2.com/images/d/d2/Mistlock_Instability_Social_Awkwardness.png
	{ 53932, "Mistlock Instability: Stick Together" }, // https://wiki.guildwars2.com/images/5/59/Mistlock_Instability_Stick_Together.png
	{ 54237, "Mistlock Instability: Sugar Rush" }, // https://wiki.guildwars2.com/images/4/4c/Mistlock_Instability_Sugar_Rush.png
	{ 36204, "Mistlock Instability: Toxic Trail" }, // https://wiki.guildwars2.com/images/d/df/Mistlock_Instability_Toxic_Trail.png
	{ 46865, "Mistlock Instability: Vengeance" }, // https://wiki.guildwars2.com/images/c/c6/Mistlock_Instability_Vengeance.png
	{ 54719, "Mistlock Instability: We Bleed Fire" }, // https://wiki.guildwars2.com/images/2/24/Mistlock_Instability_We_Bleed_Fire.png
	{ 47288, "Mistlock Instability: Toxic Sickness" }, // https://wiki.guildwars2.com/images/6/6f/Mistlock_Instability_Toxic_Sickness.png

	// Generic
	{ 38077, "Spectral Agony" }, // https://wiki.guildwars2.com/images/7/70/Spectral_Agony.png
	{ 15773, "Agony" }, // https://wiki.guildwars2.com/images/b/be/Agony.png
	{ 47856, "Hamstrung" }, // https://wiki.guildwars2.com/images/b/b9/Unseen_Burden.png
	{ 18711, "Enraged (?)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	{ 23235, "Enraged (??)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	{ 50070, "Enraged 1 (100%)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	{ 61138, "Enraged 2 (100%)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	{ 31648, "Enraged 1 (200%)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	{ 36078, "Enraged 2 (200%)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	{ 53498, "Enraged 3 (200%)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	{ 60674, "Enraged 4 (200%)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	{ 61006, "Enraged (300%)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	{ 31534, "Enraged (500%)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	{ 34164, "Call of the Mists" }, // https://wiki.guildwars2.com/images/7/79/Call_of_the_Mists_%28raid_effect%29.png
	{ 38793, "Untargetable" }, // https://wiki.guildwars2.com/images/e/eb/Determined.png
	// Strike Essences
	{ 58786, "Essence of Vigilance Tier 1" }, // https://wiki.guildwars2.com/images/a/a4/Essence_of_Vigilance.png
	{ 58721, "Essence of Vigilance Tier 2" }, // https://wiki.guildwars2.com/images/a/a4/Essence_of_Vigilance.png
	{ 58182, "Essence of Vigilance Tier 3" }, // https://wiki.guildwars2.com/images/a/a4/Essence_of_Vigilance.png
	{ 60173, "Essence of Vigilance Tier 4" }, // https://wiki.guildwars2.com/images/a/a4/Essence_of_Vigilance.png
	{ 58254, "Power of Vigilance Tier 2" }, // https://wiki.guildwars2.com/images/8/86/Power_of_Vigilance.png
	{ 58712, "Power of Vigilance Tier 3" }, // https://wiki.guildwars2.com/images/8/86/Power_of_Vigilance.png
	{ 58623, "Essence of Resilience Tier 1" }, // https://wiki.guildwars2.com/images/b/b6/Essence_of_Resilience.png
	{ 58841, "Essence of Resilience Tier 2" }, // https://wiki.guildwars2.com/images/b/b6/Essence_of_Resilience.png
	{ 58839, "Essence of Resilience Tier 3" }, // https://wiki.guildwars2.com/images/b/b6/Essence_of_Resilience.png
	{ 60057, "Essence of Resilience Tier 4" }, // https://wiki.guildwars2.com/images/b/b6/Essence_of_Resilience.png
	{ 58278, "Power of Resilience Tier 2" }, // https://wiki.guildwars2.com/images/d/d3/Power_of_Resilience.png
	{ 60312, "Power of Resilience Tier 4" }, // https://wiki.guildwars2.com/images/d/d3/Power_of_Resilience.png
	{ 58480, "Essence of Valor Tier 1" }, // https://wiki.guildwars2.com/images/6/6f/Essence_of_Valor.png
	{ 58585, "Essence of Valor Tier 2" }, // https://wiki.guildwars2.com/images/6/6f/Essence_of_Valor.png
	{ 58792, "Essence of Valor Tier 3" }, // https://wiki.guildwars2.com/images/6/6f/Essence_of_Valor.png
	{ 60062, "Essence of Valor Tier 4" }, // https://wiki.guildwars2.com/images/6/6f/Essence_of_Valor.png
	{ 58385, "Power of Valor Tier 1" }, // https://wiki.guildwars2.com/images/6/64/Power_of_Valor.png
	{ 58414, "Power of Valor Tier 2" }, // https://wiki.guildwars2.com/images/6/64/Power_of_Valor.png
	// Unknown Fixation            
	{ 48533, "Fixated 1(???)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	{ 58136, "Fixated 2(???)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	//////////////////////////////////////////////
	// Mordremoth
	{ 32208, "Parietal Mastery" }, // https://wiki.guildwars2.com/images/7/76/Parietal_Mastery.png
	{ 33305, "Parietal Origin" }, // https://wiki.guildwars2.com/images/7/76/Parietal_Mastery.png
	{ 33705, "Temporal Mastery" }, // https://wiki.guildwars2.com/images/8/80/Temporal_Mastery.png
	{ 33009, "Temporal Origin" }, // https://wiki.guildwars2.com/images/8/80/Temporal_Mastery.png
	{ 33834, "Occipital Mastery" }, // https://wiki.guildwars2.com/images/9/9a/Occipital_Mastery.png
	{ 32218, "Occipital Origin" }, // https://wiki.guildwars2.com/images/9/9a/Occipital_Mastery.png
	{ 33194, "Frontal Mastery" }, // https://wiki.guildwars2.com/images/4/44/Frontal_Mastery.png
	{ 33095, "Frontal Origin" }, // https://wiki.guildwars2.com/images/4/44/Frontal_Mastery.png
	{ 33470, "Exposed (Mordremoth)" }, // https://wiki.guildwars2.com/images/6/6b/Exposed.png
	{ 33378, "Weakened (Effect 1)" }, // https://wiki.guildwars2.com/images/8/8a/Weakened.png
	{ 33904, "Weakened (Effect 2)" }, // https://wiki.guildwars2.com/images/8/8a/Weakened.png
	{ 33763, "Empowered (Hearts and Minds)" }, // https://wiki.guildwars2.com/images/5/5e/Empowered_%28Hearts_and_Minds%29.png
	{ 32762, "Power (Hearts and Minds)" }, // https://wiki.guildwars2.com/images/e/ec/Power_%28Hearts_and_Minds%29.png
	{ 33131, "Shifty Aura" }, // https://wiki.guildwars2.com/images/7/78/Branded_Aura.png
	{ 18981, "Fiery Block" }, // https://wiki.guildwars2.com/images/d/de/Shield_Stance.png
	//////////////////////////////////////////////
	// VG
	{ 31413, "Blue Pylon Power" }, // https://wiki.guildwars2.com/images/6/6e/Blue_Pylon_Power.png
	{ 31695, "Pylon Attunement: Red" }, // https://wiki.guildwars2.com/images/9/94/Pylon_Attunement-_Red.png
	{ 31317, "Pylon Attunement: Blue" }, // https://wiki.guildwars2.com/images/6/6e/Blue_Pylon_Power.png
	{ 31852, "Pylon Attunement: Green" }, // https://wiki.guildwars2.com/images/a/aa/Pylon_Attunement-_Green.png
	{ 31539, "Unstable Pylon: Red" }, // https://wiki.guildwars2.com/images/3/36/Unstable_Pylon_%28Red%29.png
	{ 31884, "Unstable Pylon: Blue" }, // https://wiki.guildwars2.com/images/c/c3/Unstable_Pylon_%28Blue%29.png
	{ 31828, "Unstable Pylon: Green" }, // https://wiki.guildwars2.com/images/9/9d/Unstable_Pylon_%28Green%29.png
	{ 34979, "Unbreakable" }, // https://wiki.guildwars2.com/images/5/56/Xera%27s_Embrace.png
	{ 51371, "Unstable Magic Spike" }, // 
	// Gorseval
	{ 31722, "Spirited Fusion" }, // https://wiki.guildwars2.com/images/e/eb/Spirited_Fusion.png
	{ 31877, "Protective Shadow" }, // https://wiki.guildwars2.com/images/8/87/Protective_Shadow.png
	{ 31623, "Ghastly Prison" }, // https://wiki.guildwars2.com/images/6/62/Ghastly_Prison.png
	{ 31548, "Vivid Echo" }, // https://wiki.guildwars2.com/images/4/4f/Vivid_Echo.png
	{ 31498, "Spectral Darkness" }, // https://wiki.guildwars2.com/images/a/a8/Spectral_Darkness.png
	// Sabetha    
	{ 34108, "Shell-Shocked" }, // https://wiki.guildwars2.com/images/3/39/Shell-Shocked.png
	{ 31473, "Sapper Bomb" }, // https://wiki.guildwars2.com/images/8/88/Sapper_Bomb_%28effect%29.png
	{ 31485, "Time Bomb" }, // https://wiki.guildwars2.com/images/9/91/Time_Bomb.png
	//////////////////////////////////////////////
	// Slothasor
	{ 34467, "Narcolepsy" }, // https://wiki.guildwars2.com/images/e/eb/Determined.png
	{ 34496, "Nauseated" }, // https://wiki.guildwars2.com/images/3/30/Nauseated.png
	{ 34362, "Magic Transformation" }, // https://wiki.guildwars2.com/images/4/45/Magic_Transformation.png
	{ 34508, "Fixated (Slothasor)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	{ 34387, "Volatile Poison" }, // https://wiki.guildwars2.com/images/1/12/Volatile_Poison.png
	// Trio
	{ 34434, "Not the Bees!" }, // https://wiki.guildwars2.com/images/0/08/Throw_Jar.png
	{ 34464, "Slow Burn" }, // https://wiki.guildwars2.com/images/6/6e/Heat_Wave_%28Matthias_Gabrel_effect%29.png
	{ 34392, "Targeted" }, // https://wiki.guildwars2.com/images/2/24/Targeted.png
	{ 34393, "Target!" }, // https://wiki.guildwars2.com/images/0/09/Target.png
	{ 34439, "Locust Trail" }, // https://wiki.guildwars2.com/images/0/09/Target.png
	// Matthias
	{ 34376, "Blood Shield Abo" }, // https://wiki.guildwars2.com/images/a/a6/Blood_Shield.png
	{ 34518, "Blood Shield" }, // https://wiki.guildwars2.com/images/a/a6/Blood_Shield.png
	{ 34422, "Blood Fueled" }, // https://wiki.guildwars2.com/images/d/d3/Blood_Fueled.png
	{ 34428, "Blood Fueled Abo" }, // https://wiki.guildwars2.com/images/d/d3/Blood_Fueled.png
	{ 34450, "Unstable Blood Magic" }, // https://wiki.guildwars2.com/images/0/09/Unstable_Blood_Magic.png
	{ 34416, "Corruption" }, // https://wiki.guildwars2.com/images/3/34/Locust_Trail.png
	{ 34367, "Unbalanced" }, // https://wiki.guildwars2.com/images/8/80/Unbalanced.png
	{ 34511, "Zealous Benediction" }, // https://wiki.guildwars2.com/images/4/45/Unstable.png
	{ 34369, "Snowstorm" }, // https://wiki.guildwars2.com/images/2/26/Snowstorm_%28Matthias_Gabrel_effect%29.png
	{ 34526, "Heat Wave" }, // https://wiki.guildwars2.com/images/6/6e/Heat_Wave_%28Matthias_Gabrel_effect%29.png
	{ 34472, "Downpour" }, // https://wiki.guildwars2.com/images/4/4a/Downpour.png
	{ 34433, "Snowstorm (Matthias)" }, // https://wiki.guildwars2.com/images/2/26/Snowstorm_%28Matthias_Gabrel_effect%29.png
	{ 34458, "Heat Wave (Matthias)" }, // https://wiki.guildwars2.com/images/6/6e/Heat_Wave_%28Matthias_Gabrel_effect%29.png
	{ 34568, "Downpour (Matthias)" }, // https://wiki.guildwars2.com/images/4/4a/Downpour.png
	{ 34548, "Unstable" }, // https://wiki.guildwars2.com/images/4/45/Unstable.png
	//////////////////////////////////////////////
	// KC
	{ 35096, "Compromised" }, // https://wiki.guildwars2.com/images/4/48/Compromised.png
	{ 35025, "Xera's Boon" }, // https://wiki.guildwars2.com/images/0/04/Xera%27s_Boon.png
	{ 35103, "Xera's Fury" }, // https://wiki.guildwars2.com/images/d/dd/Xera%27s_Fury.png
	{ 34912, "Statue Fixated" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	{ 34925, "Statue Fixated 2" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	{ 35140, "Incoming!" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	{ 35091, "Crimson Attunement (Orb)" }, // https://wiki.guildwars2.com/images/3/3e/Crimson_Attunement.png
	{ 35109, "Radiant Attunement (Orb)" }, // https://wiki.guildwars2.com/images/6/68/Radiant_Attunement.png
	{ 35014, "Crimson Attunement (Phantasm)" }, // https://wiki.guildwars2.com/images/3/3e/Crimson_Attunement.png
	{ 34992, "Radiant Attunement (Phantasm)" }, // https://wiki.guildwars2.com/images/6/68/Radiant_Attunement.png
	{ 35119, "Magic Blast" }, // https://wiki.guildwars2.com/images/a/a9/Magic_Blast_Intensity.png
	{ 35075, "Gaining Power" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	// Twisted Castle
	{ 34918, "Spatial Distortion" }, // https://wiki.guildwars2.com/images/7/7a/Bloodstone_Blessed.png
	{ 35006, "Madness" }, // https://wiki.guildwars2.com/images/e/ee/Madness.png
	{ 35106, "Still Waters" }, // https://wiki.guildwars2.com/images/5/5c/Still_Waters_%28effect%29.png
	{ 34955, "Soothing Waters" }, // https://wiki.guildwars2.com/images/8/8f/Soothing_Waters.png
	{ 34963, "Chaotic Haze" }, // https://wiki.guildwars2.com/images/4/48/Lava_Font.png
	{ 34927, "Creeping Pursuit" }, // https://wiki.guildwars2.com/images/f/f8/Creeping_Pursuit.png
	// Xera      
	{ 34965, "Derangement" }, // https://wiki.guildwars2.com/images/c/ca/Derangement.png
	{ 35084, "Bending Chaos" }, // https://wiki.guildwars2.com/images/3/39/Target%21.png
	{ 35162, "Shifting Chaos" }, // https://wiki.guildwars2.com/images/0/04/Shifting_Chaos.png
	{ 35032, "Twisting Chaos" }, // https://wiki.guildwars2.com/images/6/60/Twisting_Chaos.png
	{ 35000, "Intervention" }, // https://wiki.guildwars2.com/images/a/a2/Intervention_%28effect%29.png
	{ 35168, "Bloodstone Protection" }, // https://wiki.guildwars2.com/images/4/4e/Bloodstone_Protection.png
	{ 34917, "Bloodstone Blessed" }, // https://wiki.guildwars2.com/images/7/7a/Bloodstone_Blessed.png
	{ 34883, "Void Zone" }, // https://wiki.guildwars2.com/images/5/56/Void_Zone.png
	//////////////////////////////////////////////
	// Cairn        
	{ 38049, "Shared Agony" }, // https://wiki.guildwars2.com/images/5/53/Shared_Agony.png
	{ 37675, "Enraged (Cairn)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	{ 38153, "Unseen Burden" }, // https://wiki.guildwars2.com/images/b/b9/Unseen_Burden.png
	{ 38098, "Countdown" }, // https://wiki.guildwars2.com/images/0/05/Countdown.png
	{ 37714, "Gaze Avoidance" }, // https://wiki.guildwars2.com/images/1/10/Gaze_Avoidance.png
	// MO             
	{ 37626, "Empowered" }, // https://wiki.guildwars2.com/images/9/9c/Empowered_%28Mursaat_Overseer%29.png
	{ 38155, "Mursaat Overseer's Shield" }, // https://wiki.guildwars2.com/images/8/84/Mursaat_Overseer%27s_Shield.png
	{ 37813, "Protect (SAK)" }, // https://wiki.guildwars2.com/images/f/f6/Protect.png
	{ 37697, "Dispel (SAK)" }, // https://wiki.guildwars2.com/images/8/84/Mursaat_Overseer%27s_Shield.png
	{ 37779, "Claim (SAK)" }, // https://wiki.guildwars2.com/images/e/ef/Claim.png
	// Samarog            
	{ 37868, "Fixated (Samarog)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	{ 38223, "Fixated (Guldhem)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	{ 37693, "Fixated (Rigom)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	{ 37966, "Inevitable Betrayal (Big)" }, // https://wiki.guildwars2.com/images/b/b8/Feeding_Frenzy_%28GW1%29.png
	{ 38247, "Inevitable Betrayal (Small)" }, // https://wiki.guildwars2.com/images/b/b8/Feeding_Frenzy_%28GW1%29.png
	{ 37892, "Soul Swarm" }, // https://wiki.guildwars2.com/images/0/0e/Soul_Swarm_%28effect%29.png
	// Deimos
	{ 38224, "Unnatural Signet" }, // https://wiki.guildwars2.com/images/2/20/Unnatural_Signet.png
	{ 38187, "Weak Minded" }, // https://wiki.guildwars2.com/images/3/38/Unseen_Burden_%28Deimos%29.png
	{ 37733, "Tear Instability" }, // https://wiki.guildwars2.com/images/1/11/Tear_Instability.png
	{ 37871, "Form Up and Advance!" }, // https://wiki.guildwars2.com/images/5/56/Form_Up_and_Advance%21.png
	{ 37718, "Devour" }, // https://wiki.guildwars2.com/images/3/3d/Devour.png
	{ 38266, "Unseen Burden (Deimos)" }, // https://wiki.guildwars2.com/images/3/38/Unseen_Burden_%28Deimos%29.png
	//////////////////////////////////////////////
	// Soulless Horror
	{ 48349, "Exile's Embrace" }, // https://wiki.guildwars2.com/images/b/b4/Exile%27s_Embrace.png
	{ 47434, "Fixated (SH)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	{ 47414, "Necrosis" }, // https://wiki.guildwars2.com/images/4/47/Ichor.png
	// River
	{ 47470, "Soul Siphon" }, // https://wiki.guildwars2.com/images/f/f7/Soul_Siphon.png
	{ 47219, "Desmina's Protection" }, // https://wiki.guildwars2.com/images/b/b3/Desmina%27s_Protection.png
	{ 47122, "Follower's Asylum" }, // https://wiki.guildwars2.com/images/b/b3/Desmina%27s_Protection.png
	{ 48331, "Spirit Form" }, // https://wiki.guildwars2.com/images/2/2e/Spirit_Form_%28Hall_of_Chains%29.png
	{ 48154, "Mortal Coil (River)" }, // https://wiki.guildwars2.com/images/4/41/Mortal_Coil.png
	{ 47758, "Energy Threshold (River)" }, // https://wiki.guildwars2.com/images/2/2e/Spirit_Form_%28Hall_of_Chains%29.png
	// Broken King          
	{ 47776, "Frozen Wind" }, // https://wiki.guildwars2.com/images/3/3a/Frozen_Wind.png
	{ 47595, "Shield of Ice" }, // https://wiki.guildwars2.com/images/3/38/Shield_of_Ice.png
	{ 47022, "Glaciate" }, // https://wiki.guildwars2.com/images/b/ba/Glaciate.png
	// Eater of Soul         
	{ 48541, "Soul Digestion" }, // https://wiki.guildwars2.com/images/0/08/Soul_Digestion.png
	{ 47090, "Reclaimed Energy" }, // https://wiki.guildwars2.com/images/2/21/Reclaimed_Energy.png
	{ 48583, "Mortal Coil (Statue of Death)" }, // https://wiki.guildwars2.com/images/4/41/Mortal_Coil.png
	{ 47473, "Empowered (Statue of Death)" }, // https://wiki.guildwars2.com/images/d/de/Empowered_%28Statue_of_Death%29.png
	//  Eyes
	{ 47635, "Last Grasp (Judgment)" }, // https://wiki.guildwars2.com/images/2/26/Last_Grasp.png
	{ 47278, "Last Grasp (Fate)" }, // https://wiki.guildwars2.com/images/2/26/Last_Grasp.png
	{ 47251, "Exposed (Statue of Darkness)" }, // https://wiki.guildwars2.com/images/4/42/Exposed_%28Statue_of_Darkness%29.png
	{ 48779, "Light Carrier" }, // https://wiki.guildwars2.com/images/f/f1/Torch_Fielder.png
	{ 48477, "Empowered (Light Thief)" }, // https://wiki.guildwars2.com/images/0/08/Soul_Digestion.png
	// Dhuum
	{ 48281, "Mortal Coil (Dhuum)" }, // https://wiki.guildwars2.com/images/4/48/Compromised.png
	{ 46950, "Fractured Spirit" }, // https://wiki.guildwars2.com/images/c/c3/Fractured_Spirit.png
	{ 47476, "Residual Affliction" }, // https://wiki.guildwars2.com/images/1/12/Residual_Affliction.png
	{ 47646, "Arcing Affliction" }, // https://wiki.guildwars2.com/images/f/f0/Arcing_Affliction.png
	{ 47929, "One-Track Mind" }, // https://wiki.guildwars2.com/images/6/68/Tracked.png
	{ 47181, "Imminent Demise" }, // https://wiki.guildwars2.com/images/5/58/Superheated_Metal.png
	{ 47202, "Lethal Report" }, // https://wiki.guildwars2.com/images/0/02/Mantra_of_Signets.png
	{ 48773, "Hastened Demise" }, // https://wiki.guildwars2.com/images/5/5b/Hastened_Demise.png
	{ 49125, "Echo's Pick up" }, // https://wiki.guildwars2.com/images/4/45/Unstable.png
	{ 48848, "Energy Threshold" }, // https://wiki.guildwars2.com/images/2/2e/Spirit_Form_%28Hall_of_Chains%29.png
	//////////////////////////////////////////////
	// CA
	{ 52667, "Greatsword Power" }, // https://wiki.guildwars2.com/images/3/3b/Greatsword_Power_%28effect%29.png
	{ 53030, "Fractured - Enemy" }, // https://wiki.guildwars2.com/images/7/78/Branded_Aura.png
	{ 52213, "Fractured - Allied" }, // https://wiki.guildwars2.com/images/7/78/Branded_Aura.png
	{ 52754, "Conjured Shield" }, // https://wiki.guildwars2.com/images/8/83/Conjured_Shield_%28effect%29.png
	{ 52973, "Conjured Protection" }, // https://wiki.guildwars2.com/images/8/83/Bloodstone-Infused_shield.png
	{ 53003, "Shielded" }, // https://wiki.guildwars2.com/images/4/47/Golem-Powered_Shielding.png
	{ 52074, "Augmented Power" }, // https://wiki.guildwars2.com/images/4/47/Golem-Powered_Shielding.png
	{ 53075, "Locked On" }, // https://wiki.guildwars2.com/images/3/39/Target%21.png
	{ 52255, "CA Invul" }, // https://wiki.guildwars2.com/images/d/d3/Blood_Fueled.png
	{ 52430, "Arm Up" }, // https://wiki.guildwars2.com/images/d/d3/Blood_Fueled.png
	{ 52943, "Fixation" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	// Twin Largos
	{ 52211, "Aquatic Aura (Kenut)" }, // https://wiki.guildwars2.com/images/4/44/Expose_Weakness.png
	{ 52929, "Aquatic Aura (Nikare)" }, // https://wiki.guildwars2.com/images/f/fd/Fractured_%28effect%29.png
	{ 51935, "Waterlogged" }, // https://wiki.guildwars2.com/images/8/89/Waterlogged.png
	{ 52626, "Enraged (Twin Largos)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	// Qadim
	{ 52568, "Flame Armor" }, // https://wiki.guildwars2.com/images/e/e7/Magma_Orb.png
	{ 52588, "Fiery Surge" }, // https://wiki.guildwars2.com/images/f/f9/Fiery_Surge.png
	{ 52035, "Power of the Lamp" }, // https://wiki.guildwars2.com/images/e/e5/Break_Out%21.png
	{ 52408, "Unbearable Flames" }, // https://wiki.guildwars2.com/images/2/21/Expel_Excess_Magic_Poison.png
	{ 52464, "Parry" }, // https://wiki.guildwars2.com/images/2/28/Parry_%28effect%29.png
	{ 52863, "Mythwright Surge" }, // https://wiki.guildwars2.com/images/7/7a/Swiftness_%28effect%29.png
	{ 51726, "Lamp Bond" }, // https://wiki.guildwars2.com/images/d/db/Lamp_Bond.png
	{ 52040, "Enraged (Wywern)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	{ 51933, "Enraged (Qadim)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	{ 52075, "Resistance (Lava Elemental)" }, // https://wiki.guildwars2.com/images/1/18/Fire_Shield.png
	{ 53048, "Shielded (Lava Elemental)" }, // https://wiki.guildwars2.com/images/1/18/Fire_Shield.png
	//////////////////////////////////////////////
	// Adina
	{ 56204, "Pillar Pandemonium" }, // https://wiki.guildwars2.com/images/d/d9/Captain%27s_Inspiration.png
	{ 56593, "Radiant Blindness" }, // https://wiki.guildwars2.com/images/6/6c/Radiant_Blindness.png
	{ 56099, "Diamond Palisade (Damage)" }, // https://wiki.guildwars2.com/images/5/5f/Monster_Skill.png
	{ 56636, "Diamond Palisade" }, // https://wiki.guildwars2.com/images/5/5f/Monster_Skill.png
	{ 56440, "Eroding Curse" }, // https://wiki.guildwars2.com/images/d/de/Toxic_Gas.png
	// Sabir
	{ 56100, "Ion Shield" }, // https://wiki.guildwars2.com/images/9/94/Ion_Shield.png
	{ 56123, "Violent Currents" }, // https://wiki.guildwars2.com/images/0/06/Violent_Currents.png
	{ 56172, "Repulsion Field" }, // https://wiki.guildwars2.com/images/2/24/Targeted.png
	{ 56391, "Electrical Repulsion" }, // https://wiki.guildwars2.com/images/d/dd/Xera%27s_Fury.png
	{ 56474, "Electro-Repulsion" }, // https://wiki.guildwars2.com/images/4/42/Exposed_%28Statue_of_Darkness%29.png
	{ 56216, "Eye of the Storm" }, // https://wiki.guildwars2.com/images/5/52/Mending_Waters_%28effect%29.png
	{ 56394, "Bolt Break" }, // https://wiki.guildwars2.com/images/7/74/Mesmer_icon_white.png
	// Peerless Qadim
	{ 56582, "Erratic Energy" }, // https://wiki.guildwars2.com/images/4/45/Unstable.png
	{ 56104, "Power Share" }, // https://wiki.guildwars2.com/images/2/24/Targeted.png
	{ 56118, "Sapping Surge" }, // https://wiki.guildwars2.com/images/6/6f/Guilt_Exploitation.png
	{ 56182, "Chaos Corrosion" }, // https://wiki.guildwars2.com/images/f/fd/Fractured_%28effect%29.png
	{ 56510, "Fixated (Qadim the Peerless)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	{ 56475, "Magma Drop" }, // https://wiki.guildwars2.com/images/2/24/Targeted.png
	{ 56424, "Critical Mass" }, // https://wiki.guildwars2.com/images/b/bf/Orb_of_Ascension_%28effect%29.png
	{ 56609, "Kinetic Abundance" }, // https://wiki.guildwars2.com/images/6/64/Kinetic_Abundance.png
	{ 56540, "Enfeebled Force" }, // https://wiki.guildwars2.com/images/b/b6/Enfeebled_Force.png
	{ 56360, "Backlashing Beam" }, // https://wiki.guildwars2.com/images/0/04/Xera%27s_Boon.png
	{ 56257, "Clutched by Chaos" }, // https://wiki.guildwars2.com/images/8/87/Protective_Shadow.png
	{ 56237, "Cleansed Conductor" }, // https://wiki.guildwars2.com/images/a/a9/Magic_Blast_Intensity.png
	{ 56655, "Poisoned Power" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	{ 56250, "Incorporeal" }, // https://wiki.guildwars2.com/images/8/8b/Magic_Aura.png
	{ 56264, "Flare-Up" }, // https://wiki.guildwars2.com/images/8/8b/Magic_Aura.png
	{ 56467, "Unbridled Chaos" }, // https://wiki.guildwars2.com/images/4/42/Exposed_%28Statue_of_Darkness%29.png
	//////////////////////////////////////////////
	// Fractals 
	{ 33652, "Rigorous Certainty" }, // https://wiki.guildwars2.com/images/6/60/Desert_Carapace.png
	{ 47248, "Fractal Savant" }, // https://wiki.guildwars2.com/images/c/cb/Malign_9_Agony_Infusion.png
	{ 48191, "Fractal Prodigy" }, // https://wiki.guildwars2.com/images/1/11/Mighty_9_Agony_Infusion.png
	{ 48414, "Fractal Champion" }, // https://wiki.guildwars2.com/images/3/3d/Precise_9_Agony_Infusion.png
	{ 47222, "Fractal God" }, // https://wiki.guildwars2.com/images/2/22/Healing_9_Agony_Infusion.png
	// Siax 
	{ 36998, "Fixated (Nightmare)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	// Ensolyss 
	{ 37498, "Determination (Ensolyss)" }, // https://wiki.guildwars2.com/images/4/41/Gambit_Exhausted.png
	// Artsariiv
	{ 36498, "Enraged (Fractal)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	{ 38880, "Corporeal Reassignment" }, // https://wiki.guildwars2.com/images/9/94/Redirect_Anomaly.png
	{ 39442, "Blinding Radiance" }, // https://wiki.guildwars2.com/images/5/5f/Monster_Skill.png
	{ 38841, "Determination (Viirastra)" }, // https://wiki.guildwars2.com/images/4/41/Gambit_Exhausted.png
	// Arkk 
	{ 39558, "Fixated (Bloom 3)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	{ 39928, "Fixated (Bloom 2)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	{ 39131, "Fixated (Bloom 1)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	{ 38985, "Fixated (Bloom 4)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	{ 39268, "Cosmic Meteor" }, // https://wiki.guildwars2.com/images/5/5f/Monster_Skill.png
	{ 39111, "Diaphanous Shielding" }, // https://wiki.guildwars2.com/images/5/57/Diaphanous_Shielding.png
	{ 33750, "Electrocuted" }, // https://wiki.guildwars2.com/images/9/91/Air_Attunement.png
	// Ai, Keeper of the Peak
	{ 61402, "Tidal Barrier" }, // https://wiki.guildwars2.com/images/b/b1/Primed_Bottle.png
	{ 61224, "Whirlwind Shield" }, // https://wiki.guildwars2.com/images/b/b1/Primed_Bottle.png
	{ 61220, "Resilient Form" }, // https://wiki.guildwars2.com/images/1/13/Crowd_Favor.png
	{ 61435, "Cacophonous Mind" }, // https://wiki.guildwars2.com/images/1/13/Crowd_Favor.png
	{ 61208, "Crushing Guilt" }, // https://wiki.guildwars2.com/images/1/13/Crowd_Favor.png
	{ 61304, "Fixated (Fear 3)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	{ 61306, "Fixated (Fear 2)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	{ 61503, "Fixated (Fear 1)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	{ 61566, "Fixated (Fear 4)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	{ 61444, "Charged Leap" }, // https://wiki.guildwars2.com/images/1/13/Crowd_Favor.png
	{ 61512, "Tidal Bargain" }, // https://wiki.guildwars2.com/images/1/13/Crowd_Favor.png
	//////////////////////////////////////////////
	// Icebrood
	{ 57969, "Hypothermia" }, // https://wiki.guildwars2.com/images/d/d5/Hypothermia_%28story_effect%29.png
	// Fraenir of Jormag
	{ 58376, "Frozen" }, // https://wiki.guildwars2.com/images/6/6a/Frostbite_%28Bitterfrost_Frontier%29.png
	{ 58276, "Snowblind" }, // https://wiki.guildwars2.com/images/6/6a/Frostbite_%28Bitterfrost_Frontier%29.png
	// Voice and Claw            
	{ 58619, "Enraged (V&C)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	// Boneskinner     
	{ 58860, "Tormenting Aura" }, // https://wiki.guildwars2.com/images/c/c0/Darkness.png
	// Whisper of Jormag
	{ 59223, "Whisper Teleport Out" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	{ 59054, "Whisper Teleport Back" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	{ 59105, "Frigid Vortex" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	{ 59100, "Chains of Frost Active" }, // https://wiki.guildwars2.com/images/6/63/Use_Soul_Binder.png
	{ 59120, "Chains of Frost Application" }, // https://wiki.guildwars2.com/images/5/5f/Monster_Skill.png
	{ 59073, "Brain Freeze" }, // https://wiki.guildwars2.com/images/6/6a/Frostbite_%28Bitterfrost_Frontier%29.png
	// Frezie      
	{ 53510, "Icy Barrier" }, // https://wiki.guildwars2.com/images/3/38/Shield_of_Ice.png
	// Mai Trin
	{ 65900, "Shared Destruction" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	{ 64843, "Immune to damage and conditions." }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	{ 65238, "Mai Trin ?????" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	{ 66409, "Chaos and Destruction" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	// Ankka
	{ 67447, "Necrotic Ritual" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	{ 66962, "Ankka ???" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	{ 64179, "Hallucinations" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	{ 63621, "Energy Transfer" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	// Minister Li   
	{ 65869, "Target Order: 1" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	{ 65088, "Target Order: 2" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	{ 64377, "Target Order: 3" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	{ 64999, "Target Order: 4" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	{ 64465, "Stronger Together" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	{ 63840, "Vitality Equalizer 1" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	{ 66894, "Vitality Equalizer 2" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	{ 65250, "Destructive Aura" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	{ 67047, "Equalization Matrix" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	{ 64834, "Lethal Inspiration" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	{ 65662, "Extreme Vulnerability" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	{ 66140, "Fixated (Kaineng Overlook)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	//     
	{ 64524, "Influence of the Void" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	{ 60517, "Targeted (Dragon Void)" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	{ 65729, "Void Repulsion 1" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	{ 65577, "Void Repulsion 2" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	{ 63605, "Aerial Defense" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	{ 66800, "Void Immunity" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	{ 65563, "Void Shell" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
};

std::vector<Buff> Buffs::GenerateBuffsList()
{
	const std::map<std::string, glm::vec4> atlasElements
	{
		#include <assets/atlas.inc>
	};

	std::vector<Buff> buffs;
	buffs.assign(g_Buffs.begin(), g_Buffs.end());
	
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