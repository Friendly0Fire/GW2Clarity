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
#include <variant>
#include <range/v3/all.hpp>

namespace GW2Clarity
{
ImVec2 AdjustToArea(float w, float h, float availW)
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
			if (buff == 0)
				hiddenBuffs_.insert(id);
	}
	ImGui::SameLine();
	if (ImGui::Button("Hide currently active"))
	{
		for (auto& [id, buff] : activeBuffs_)
			if (buff > 0)
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
		ImGui::TableSetupColumn("Chat Link", ImGuiTableColumnFlags_WidthStretch, 5.f);
		ImGui::TableHeadersRow();
		for (auto& [id, buff] : activeBuffs_)
		{
			if (buff == 0 && hideInactive_ || hiddenBuffs_.count(id) > 0)
				continue;

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);

			ImGui::Text("%u", id);

			ImGui::TableNextColumn();

			ImGui::Text("%d", buff);

			ImGui::TableNextColumn();

			if (auto it = buffsMap_.find(id); it != buffsMap_.end())
				ImGui::TextUnformatted(it->second->name.c_str());
			else {
				auto& str = buffNames_[id];
				if (ImGui::InputText(std::format("##Name{}", id).c_str(), &str))
					SaveNames();
			}

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
		const auto& off = getG(selectedId_).offset;
		auto d = ImGui::GetIO().DisplaySize;
		auto c = d * 0.5f + ImVec2(off.x, off.y);

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
	if(ImGui::BeginTable("##GridsAndItems", 2)) {
		auto newCurrentHovered = Unselected();
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Grids");
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Items");

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		if(ImGui::BeginListBox("##GridsList", ImVec2(-FLT_MIN, 0.f))) {
			for (auto&& [gid, g] : grids_ | ranges::views::enumerate) {
				auto u = Unselected(gid);
				if (ImGui::Selectable(std::format("{} ({}x{})##{}", g.name, g.spacing.x, g.spacing.y, gid).c_str(), selectedId_ == u || currentHovered_ == u, ImGuiSelectableFlags_AllowItemOverlap)) {
					selectedId_ = u;
					selectedSetId_ = UnselectedSubId;
				}

				if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) || ImGui::IsItemActive()) {
					auto& style = ImGui::GetStyle();
					auto orig = style.Colors[ImGuiCol_Button];
					style.Colors[ImGuiCol_Button] *= ImVec4(0.5f, 0.5f, 0.5f, 1.f);
					if(ImGuiClose(std::format("CloseGrid{}", gid).c_str(), 0.75f, false))
					{
						selectedId_ = u;
						selectedSetId_ = UnselectedSubId;
						ImGui::OpenPopup(confirmDeletionPopupID_);
					}
					if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
						newCurrentHovered = u;

					style.Colors[ImGuiCol_Button] = orig;
				}
			}

			if(ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				selectedId_ = Unselected();
				selectedSetId_ = UnselectedSubId;
			}

			ImGui::EndListBox();
		}
		ImGui::TableNextColumn();

		const bool disableItemsList = selectedId_.grid < 0;

		ImGui::BeginDisabled(disableItemsList);
		if(ImGui::BeginListBox("##ItemsList", ImVec2(-FLT_MIN, 0.f))) {
			if(selectedId_.grid != UnselectedSubId)
			{
				auto& g = getG(selectedId_);
				auto gid = selectedId_.grid;
				for (auto&& [iid, i] : g.items | ranges::views::enumerate)
				{
					Id id{ gid, iid };
					if (ImGui::Selectable(std::format("{} ({}, {})##{}", i.buff->name.c_str(), i.pos.x, i.pos.y, iid).c_str(), selectedId_ == id || currentHovered_ == id, ImGuiSelectableFlags_AllowItemOverlap))
					{
						selectedId_ = id;
						selectedSetId_ = UnselectedSubId;
					}

					if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) || ImGui::IsItemActive()) {
						auto& style = ImGui::GetStyle();
						auto orig = style.Colors[ImGuiCol_Button];
						style.Colors[ImGuiCol_Button] *= ImVec4(0.5f, 0.5f, 0.5f, 1.f);
						if(ImGuiClose(std::format("CloseItem{}", id.id).c_str(), 0.75f, false))
						{
							selectedId_ = id;
							selectedSetId_ = UnselectedSubId;
							ImGui::OpenPopup(confirmDeletionPopupID_);
						}
						if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
							newCurrentHovered = id;

						style.Colors[ImGuiCol_Button] = orig;
					}
				}

				if(ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
					selectedId_ = Unselected(gid);
					selectedSetId_ = UnselectedSubId;
				}
			}
			else
				ImGui::TextUnformatted("<no grid selected>");
			ImGui::EndListBox();
		}
		ImGui::EndDisabled();

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		if(ImGui::Button("New grid")) {
			selectedId_ = New();
			selectedSetId_ = UnselectedSubId;
		}
		ImGui::TableNextColumn();
		ImGui::BeginDisabled(disableItemsList);
		if(ImGui::Button("New item")) {
			selectedId_ = New(selectedId_.grid);
			selectedSetId_ = UnselectedSubId;
		}
		ImGui::EndDisabled();

		ImGui::EndTable();

		currentHovered_ = newCurrentHovered;
	}
}

void Buffs::DrawItems()
{
	bool editMode = selectedId_.grid != UnselectedSubId;

	if (currentSetId_ != UnselectedSubId || editMode)
	{
		if(!MumbleLink::i().isInCompetitiveMode() && (editMode || !sets_[currentSetId_].combatOnly || MumbleLink::i().isInCombat()))
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

					glm::vec2 pos = (g.attached && !placingItem_ ? mouse : screen * 0.5f) + glm::vec2(i.pos * g.spacing) + glm::vec2(g.offset);

					auto adj = AdjustToArea(128.f, 128.f, float(g.spacing.x));

					if (editing)
						ImGui::GetWindowDrawList()->AddRect(ToImGui(pos), ToImGui(pos) + adj, 0xFF0000FF);

					ImGui::SetCursorPos(ToImGui(pos));
					ImGui::Image(buffsAtlas_.srv.Get(), adj, ToImGui(i.buff->uv.xy), ToImGui(i.buff->uv.zw), tint);
					if (count > 1 && i.buff->maxStacks > 1)
					{
						count = std::min({ count, i.buff->maxStacks, int(numbersMap_.size()) - 1 });
						delayedDraws.emplace_back(adj, ToImGui(pos), ToImGui(numbersMap_[count].xy), ToImGui(numbersMap_[count].zw));
					}
				};

				auto drawGrid = [&](Grid& g, short gid)
				{
					for (const auto&& [iid, i] : g.items | ranges::views::enumerate)
					{
						bool editing = editMode && selectedId_ == Id{ gid, iid };
						int count = 0;
						if (editing)
							count = editingItemFakeCount_;
						else
							count = std::accumulate(i.additionalBuffs.begin(), i.additionalBuffs.end(), activeBuffs_[i.buff->id], [&](int a, const Buff* b) { return a + activeBuffs_[b->id]; });

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
		id = Unselected(id.grid);

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
	if(!confirmDeletionPopupID_)
		confirmDeletionPopupID_ = ImGui::GetID(ConfirmDeletionPopupName);
	if (ImGui::BeginPopupModal(ConfirmDeletionPopupName))
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
		if (ImGui::Button("No"))
			ImGui::CloseCurrentPopup();

		ImGui::EndPopup();
	}

	DrawEditingGrid();
	PlaceItem();

	auto saveCheck = [this](bool changed) { needsSaving_ = needsSaving_ || changed; return changed; };

	{
		ImGuiTitle("Grid Editor");

		DrawGridList();

		bool editingGrid = selectedId_.grid >= 0 && selectedId_.item == UnselectedSubId;
		bool editingItem = selectedId_.item >= 0;
		if (editingGrid || selectedId_.grid == NewSubId)
		{
			auto& editGrid = editingGrid ? getG(selectedId_) : creatingGrid_;
			if (editingGrid)
				ImGuiTitle(std::format("Editing Grid '{}'", editGrid.name).c_str(), 0.75f);
			else
				ImGuiTitle("New Grid", 0.75f);
			
			saveCheck(ImGui::InputText("Grid Name", &editGrid.name));
			ImGui::NewLine();

			// If drag was triggered, stay on as long as dragging occurs, otherwise disable
			draggingGridScale_ = draggingGridScale_ ? ImGui::IsMouseDragging(ImGuiMouseButton_Left) : false;

			saveCheck(ImGui::Checkbox("Square Grid", &editGrid.square));
			if (editGrid.square && saveCheck(ImGui::DragInt("Grid Scale", (int*)&editGrid.spacing, 0.1f, 1, 2048)) ||
				!editGrid.square && saveCheck(ImGui::DragInt2("Grid Scale", glm::value_ptr(editGrid.spacing), 0.1f, 1, 2048)))
			{
				draggingGridScale_ |= ImGui::IsMouseDragging(ImGuiMouseButton_Left);
			}
			if (editGrid.square)
				editGrid.spacing.y = editGrid.spacing.x;

			if(saveCheck(ImGui::DragInt2("Grid Offset", glm::value_ptr(editGrid.offset), 0.1f, -ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.x / 2)))
			{
				draggingGridScale_ |= ImGui::IsMouseDragging(ImGuiMouseButton_Left);
			}

			saveCheck(ImGui::Checkbox("Attached to Mouse", &editGrid.attached));

			if (selectedId_.grid == NewSubId && ImGui::Button("Create Grid"))
			{
				grids_.push_back(creatingGrid_);
				creatingGrid_ = {};
				selectedId_ = Unselected(grids_.size() - 1);
				needsSaving_ = true;
			}
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
					ImGui::InputText("Search...", buffSearch_, sizeof(buffSearch_));
					std::string_view bs = buffSearch_;

					for (auto& b : buffs_)
					{
						if(b.id == 0xFFFFFFFF) {
							if(bs.empty())
							{
								ImGui::PushFont(Core::i().fontBold());
								ImGui::Text(b.name.c_str());
								ImGui::PopFont();
								ImGui::Separator();
							}
							continue;
						}

						if(!bs.empty() && std::search(
							b.name.begin(), b.name.end(),
							bs.begin(), bs.end(),
							[](char l, char r) { return std::tolower(l) == std::tolower(r); }) == b.name.end())
							continue;

						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.f);

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
				buffCombo(extraBuff, int(n));
				ImGui::SameLine();
				if (ImGuiClose(std::format("RemoveExtraBuff{}", n).c_str()))
					removeId = int(n);
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
				selectedId_ = { selectedId_.grid, getG(selectedId_).items.size() - 1 };
				needsSaving_ = true;
			}
		}
	}
	{
		ImGuiTitle("Set Editor");
		if(ImGui::BeginListBox("##SetsList", ImVec2(-FLT_MIN, 0.f))) {
			short newCurrentHovered = currentHoveredSet_;
			for (auto&& [sid, s] : sets_ | ranges::views::enumerate)
			{
				if (ImGui::Selectable(std::format("{}##Set", s.name).c_str(), selectedSetId_ == sid || currentHoveredSet_ == sid, ImGuiSelectableFlags_AllowItemOverlap))
				{
					selectedSetId_ = short(sid);
					selectedId_ = Unselected();
				}

				if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) || ImGui::IsItemActive()) {
					auto& style = ImGui::GetStyle();
					auto orig = style.Colors[ImGuiCol_Button];
					style.Colors[ImGuiCol_Button] *= ImVec4(0.5f, 0.5f, 0.5f, 1.f);
					if(ImGuiClose(std::format("CloseSet{}", sid).c_str(), 0.75f, false))
					{
						selectedId_ = Unselected();
						selectedSetId_ = short(sid);
						ImGui::OpenPopup(confirmDeletionPopupID_);
					}
					if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
						newCurrentHovered = short(sid);

					style.Colors[ImGuiCol_Button] = orig;
				}
			}
			currentHoveredSet_ = newCurrentHovered;
			ImGui::EndListBox();
		}
		if (ImGui::Button("New set"))
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
				bool sel = editSet.grids.count(int(gid)) > 0;
				if (saveCheck(ImGui::Checkbox(std::format("{}##GridInSet", g.name).c_str(), &sel)))
				{
					if (sel)
						editSet.grids.insert(int(gid));
					else
						editSet.grids.erase(int(gid));
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
	{
		selectedId_ = Unselected();
		selectedSetId_ = UnselectedSubId;
	}

	if(showSetSelector_ || firstDraw_)
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
					currentSetId_ = short(i);
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
	if(firstDraw_ || showAnalyzer_)
	{
		if (ImGui::Begin("Buffs Analyzer", &showAnalyzer_))
			DrawBuffAnalyzer();
		ImGui::End();
	}
#endif

	firstDraw_ = false;
}

void Buffs::UpdateBuffsTable(StackedBuff* buffs)
{
#ifdef _DEBUG
	for (auto& b : activeBuffs_)
		b.second = 0;
#else
	activeBuffs_.clear();
#endif
	for (size_t i = 0; buffs[i].id; i++)
		activeBuffs_[buffs[i].id] = buffs[i].count;
}

void Buffs::Load()
{
	using namespace nlohmann;
	grids_.clear();
	selectedId_ = Unselected();

	auto& cfg = JSONConfigurationFile::i();
	cfg.Reload();

	auto maybe_at = []<typename D>(const json& j, const char* n, const D& def, const std::variant<std::monostate, std::function<D(const json&)>>& cvt = {}) {
		auto it   = j.find(n);
		if(it == j.end())
			return def;
		if(cvt.index() == 0)
			return static_cast<D>(*it);
		else
			return std::get<1>(cvt)(*it);
	};

	auto getivec2 = [](const json& j) { return glm::ivec2(j[0].get<int>(), j[1].get<int>()); };
	auto getImVec4 = [](const json& j) { return ImVec4(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>()); };

	const auto& grids = cfg.json()["buff_grids"];
	for (const auto& gIn : grids)
	{
		Grid g;
		g.spacing = maybe_at(gIn, "spacing", glm::ivec2(32, 32), { getivec2 });
		g.offset = maybe_at(gIn, "offset", glm::ivec2(), { getivec2 });
		g.attached = maybe_at(gIn, "attached", false);
		g.square = maybe_at(gIn, "square", true);
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
		s.combatOnly = maybe_at(sIn, "combat_only", true);

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
		grid["offset"] = { g.offset.x, g.offset.y };
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

#include "BuffsList.inc"

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
		auto it = atlasElements.find(b.atlasEntry);
		b.uv = it != atlasElements.end() ? it->second : glm::vec4 { 0.f, 0.f, 0.f, 0.f };
	}

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