#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>

#include <Grids.h>
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

Grids::Grids(ComPtr<ID3D11Device>& dev)
	: buffs_(GenerateBuffsList())
	, buffsMap_(GenerateBuffsMap(buffs_))
	, numbersMap_(GenerateNumbersMap())
{
	buffsAtlas_ = CreateTextureFromResource(dev.Get(), Core::i().dllModule(), IDR_BUFFS);
	numbersAtlas_ = CreateTextureFromResource(dev.Get(), Core::i().dllModule(), IDR_NUMBERS);

	Input::i().mouseButtonEvent().AddCallback([&](EventKey ek, bool&) {
		bool wasHolding = holdingMouseButton_ != ScanCode::NONE;
		if(ek.sc == ScanCode::LBUTTON || ek.sc == ScanCode::RBUTTON)
			holdingMouseButton_ = ek.down ? (holdingMouseButton_ | ek.sc) : (holdingMouseButton_ & ~ek.sc);
		bool isHolding = holdingMouseButton_ != ScanCode::NONE;
		if(!wasHolding && isHolding)
			heldMousePos_ = ImGui::GetIO().MousePos;
	});

	Load();
#ifdef _DEBUG
	LoadNames();
#endif

	SettingsMenu::i().AddImplementer(this);
}

Grids::~Grids()
{
	SettingsMenu::f([&](auto& i) { i.RemoveImplementer(this); });
}

#ifdef _DEBUG
void Grids::LoadNames()
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

void Grids::SaveNames()
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

void Grids::DrawBuffAnalyzer()
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

void Grids::DrawEditingGrid()
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

	if(draggingMouseBoundaries_) {
		auto* cmdList = ImGui::GetWindowDrawList();
		cmdList->PushClipRectFullScreen();
		cmdList->AddRect(ToImGui(getG(selectedId_).mouseClipMin), ToImGui(getG(selectedId_).mouseClipMax), 0xFFFF0000);
		cmdList->PopClipRect();

		draggingMouseBoundaries_ = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
		if (!draggingMouseBoundaries_)
			needsSaving_ = true;
	}
}

void Grids::PlaceItem()
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

void Grids::DrawGridList()
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
				}

				if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) || ImGui::IsItemActive()) {
					auto& style = ImGui::GetStyle();
					auto orig = style.Colors[ImGuiCol_Button];
					style.Colors[ImGuiCol_Button] *= ImVec4(0.5f, 0.5f, 0.5f, 1.f);
					if(ImGuiClose(std::format("CloseGrid{}", gid).c_str(), 0.75f, false))
					{
						selectedId_ = u;
						Core::i().DisplayDeletionMenu(u);
					}
					if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
						newCurrentHovered = u;

					style.Colors[ImGuiCol_Button] = orig;
				}
			}

			if(ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				selectedId_ = Unselected();
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
					}

					if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) || ImGui::IsItemActive()) {
						auto& style = ImGui::GetStyle();
						auto orig = style.Colors[ImGuiCol_Button];
						style.Colors[ImGuiCol_Button] *= ImVec4(0.5f, 0.5f, 0.5f, 1.f);
						if(ImGuiClose(std::format("CloseItem{}", id.id).c_str(), 0.75f, false))
						{
							selectedId_ = id;
							Core::i().DisplayDeletionMenu(id);
						}
						if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
							newCurrentHovered = id;

						style.Colors[ImGuiCol_Button] = orig;
					}
				}

				if(ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
					selectedId_ = Unselected(gid);
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
		}
		ImGui::TableNextColumn();
		ImGui::BeginDisabled(disableItemsList);
		if(ImGui::Button("New item")) {
			selectedId_ = New(selectedId_.grid);
		}
		ImGui::EndDisabled();

		ImGui::EndTable();

		currentHovered_ = newCurrentHovered;
	}
}

void Grids::DrawItems(const Sets::Set* set, bool shouldIgnoreSet)
{
	bool editMode = selectedId_.grid != UnselectedSubId;

	if (set || shouldIgnoreSet || editMode)
	{
		if(!MumbleLink::i().isInCompetitiveMode() && (editMode || shouldIgnoreSet || !set->combatOnly || MumbleLink::i().isInCombat()))
		{
			glm::vec2 screen{ ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y };
			glm::vec2 baseMouse{ ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y };

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

				auto drawItem = [&](const Grid& g, const Item& i, const glm::vec2& gridOrigin, int count, bool editing) {

					auto thresh = std::ranges::find_if(i.thresholds, [=](const auto& t) { return count < t.threshold; });
					ImVec4 tint(1, 1, 1, 1);
					if (thresh != i.thresholds.end())
						tint = thresh->tint;

					glm::vec2 pos = gridOrigin + glm::vec2(i.pos * g.spacing);

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

				auto drawGrid = [&](const Grid& g, short gid)
				{
					glm::vec2 gridOrigin;
					if(!g.attached || (editMode && !testMouseMode_))
						gridOrigin = screen * 0.5f + glm::vec2(g.offset);
					else {
						if(!g.trackMouseWhileHeld && holdingMouseButton_ != ScanCode::NONE)
							gridOrigin = glm::vec2{ heldMousePos_.x, heldMousePos_.y };
						else
							gridOrigin = baseMouse;

						if(g.mouseClipMin.x != std::numeric_limits<int>::max()) {
							gridOrigin = glm::max(gridOrigin, glm::vec2(g.mouseClipMin));
							gridOrigin = glm::min(gridOrigin, glm::vec2(g.mouseClipMax));
						}

						if(g.centralWeight > 0.f)
							gridOrigin = glm::mix(gridOrigin, screen * 0.5f, g.centralWeight);
					}

					for (const auto&& [iid, i] : g.items | ranges::views::enumerate)
					{
						bool editing = editMode && selectedId_ == Id{ gid, iid };
						int count = 0;
						if (editing)
							count = editingItemFakeCount_;
						else
							count = std::accumulate(
								i.additionalBuffs.begin(),
								i.additionalBuffs.end(),
								activeBuffs_[i.buff->id],
								[&](int a, const Buff* b) {
									return a + (b->extraIds.empty() ? activeBuffs_[b->id] : std::accumulate(
										b->extraIds.begin(),
										b->extraIds.end(),
										activeBuffs_[b->id],
										[&](int a, uint b) { return a + activeBuffs_[b]; }));
								});

						drawItem(g, i, gridOrigin, count, editing);
					}
					if (editMode && selectedId_ == New(gid))
						drawItem(g, creatingItem_, screen * 0.5f, editingItemFakeCount_, true);
				};

				if (editMode)
					drawGrid(getG(selectedId_), selectedId_.grid);
				else if(shouldIgnoreSet)
					for (const auto& g : grids_)
						drawGrid(g, UnselectedSubId);
				else
					for (int gid : set->grids)
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

void Grids::Delete(Id id)
{
	if (id.item >= 0)
	{
		auto& g = getG(id);
		g.items.erase(g.items.begin() + id.item);

		if(id == selectedId_)
			selectedId_ = Unselected(id.grid);

		needsSaving_ = true;
	}
	else
	{
		grids_.erase(grids_.begin() + id.grid);

		if(id == selectedId_)
			selectedId_ = Unselected();
		needsSaving_ = true;
	}
}

void Grids::DrawMenu(Keybind** currentEditedKeybind)
{
	DrawEditingGrid();
	PlaceItem();

	auto saveCheck = [this](bool changed) { needsSaving_ = needsSaving_ || changed; return changed; };

	{
		DrawGridList();

		bool editingGrid = selectedId_.grid >= 0 && selectedId_.item == UnselectedSubId;
		bool editingItem = selectedId_.item >= 0;
		if(!editingGrid)
			testMouseMode_ = false;
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
			draggingMouseBoundaries_ = draggingMouseBoundaries_ ? ImGui::IsMouseDragging(ImGuiMouseButton_Left) : false;

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
			if(editGrid.attached) {
				ImGui::Indent();

				saveCheck(ImGui::DragFloat("Center Weight", &editGrid.centralWeight, 0.01f, 0.f, 1.f));
				ImGuiHelpTooltip("Reduces the impact of the mouse cursor's location: 0.0 locks to the mouse perfectly whereas 1.0 locks to the center of the screen.");
				
				saveCheck(ImGui::Checkbox("Track Mouse While Button Is Held", &editGrid.trackMouseWhileHeld));
				ImGuiHelpTooltip("Controls whether to follow the mouse motion when it is hidden while a mouse button is held.");

				bool showMouseClip = editGrid.mouseClipMin.x != std::numeric_limits<int>::max();
				bool lastShowMouseClip = showMouseClip;
				saveCheck(ImGui::Checkbox("Enable Mouse Boundaries", &showMouseClip));
				ImGuiHelpTooltip("Mouse boundaries constrain the effective mouse location within a certain region of the screen. Useful to prevent an attached grid from moving too far to the side of the screen.");
				if(showMouseClip && !lastShowMouseClip) {
					editGrid.mouseClipMin = glm::ivec2{ 0 };
					editGrid.mouseClipMax = glm::ivec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);
				}
				else if(!showMouseClip && lastShowMouseClip) {
					editGrid.mouseClipMin = glm::ivec2{ std::numeric_limits<int>::max() };
					editGrid.mouseClipMax = glm::ivec2{ std::numeric_limits<int>::min() };
				}

				if(showMouseClip) {
					ImGui::TextUnformatted("Mouse boundaries");
					ImGui::Indent();
					bool editingAny = false;
					editingAny |= saveCheck(ImGui::DragInt("Mouse Left", &editGrid.mouseClipMin.x, 1.f, 0, int(ImGui::GetIO().DisplaySize.x)));
					editingAny |= saveCheck(ImGui::DragInt("Mouse Right", &editGrid.mouseClipMax.x, 1.f, 0, int(ImGui::GetIO().DisplaySize.x)));
					editingAny |= saveCheck(ImGui::DragInt("Mouse Top", &editGrid.mouseClipMin.y, 1.f, 0, int(ImGui::GetIO().DisplaySize.y)));
					editingAny |= saveCheck(ImGui::DragInt("Mouse Bottom", &editGrid.mouseClipMax.y, 1.f, 0, int(ImGui::GetIO().DisplaySize.y)));
					ImGui::Unindent();

					if(editingAny)
						draggingMouseBoundaries_ = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
				}

				if(ImGui::Button(testMouseMode_ ? "Stop Testing" : "Begin Mouse Test"))
					testMouseMode_ = !testMouseMode_;

				ImGui::Unindent();
			}
			
			ImGui::PushFont(Core::i().fontBold());
			if (selectedId_.grid == NewSubId && ImGui::Button("Create Grid"))
			{
				grids_.push_back(creatingGrid_);
				creatingGrid_ = {};
				selectedId_ = Unselected(grids_.size() - 1);
				needsSaving_ = true;
			}
			ImGui::PopFont();
		}
		else if (editingItem || selectedId_.item == NewSubId)
		{
			auto& editItem = editingItem ? getI(selectedId_) : creatingItem_;
			if (editingItem)
				ImGuiTitle(std::format("Editing Item '{}' of '{}'", editItem.buff->name, getG(selectedId_).name).c_str(), 0.75f);
			else
				ImGuiTitle(std::format("New Item in '{}'", getG(selectedId_).name).c_str(), 0.75f);

			auto buffCombo = [&](auto& buff, int id, const char* name)
			{
				if (ImGui::BeginCombo(std::format("{}##{}", name, id).c_str(), buff->name.c_str()))
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

			buffCombo(editItem.buff, -1, "Main buff");
			int removeId = -1;
			for (auto&& [n, extraBuff] : editItem.additionalBuffs | ranges::views::enumerate)
			{
				buffCombo(extraBuff, int(n), "Secondary buff");
				ImGui::SameLine();
				if (ImGuiClose(std::format("RemoveExtraBuff{}", n).c_str()))
					removeId = int(n);
			}
			if (removeId != -1)
				editItem.additionalBuffs.erase(editItem.additionalBuffs.begin() + removeId);

			if (ImGui::Button("Add secondary buff"))
				editItem.additionalBuffs.push_back(&UnknownBuff);
			ImGuiHelpTooltip("Secondary buffs will activate the buff on screen, but without changing the icon. Useful to combine multiple related effects (e.g. Fixated) in one icon.");

			ImGui::NewLine();

			if (ImGui::Button("Place...")) {
				testMouseMode_ = false;
				placingItem_ = true;
			}
			ImGui::SameLine();
			saveCheck(ImGui::DragInt2("Location", glm::value_ptr(editItem.pos), 0.1f));

			ImGui::NewLine();
			
			ImGui::PushFont(Core::i().fontBold());
			ImGui::TextUnformatted("Thresholds");
			ImGui::PopFont();
			ImGui::Indent();

			ImGui::TextUnformatted("Simulate thresholds using ");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(100.f);
			ImGui::DragInt("stacks##Simulatedstacks", &editingItemFakeCount_, 0.1f, 0, editItem.buff ? editItem.buff->maxStacks : std::numeric_limits<int>::max());
			ImGuiHelpTooltip("Helper feature to tune threshold effects. Will simulate what the buff icon would look like with the given number of stacks.");
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

			ImGui::Unindent();

			ImGui::Separator();

			ImGui::PushFont(Core::i().fontBold());
			if (selectedId_.item == NewSubId && ImGui::Button("Create Item"))
			{
				getG(selectedId_).items.push_back(creatingItem_);
				creatingItem_ = {};
				selectedId_ = { selectedId_.grid, getG(selectedId_).items.size() - 1 };
				needsSaving_ = true;
			}
			ImGui::PopFont();
		}
	}

	mstime currentTime = TimeInMilliseconds();
	if (needsSaving_ && lastSaveTime_ + SaveDelay <= currentTime)
		Save();
}

void Grids::Draw(ComPtr<ID3D11DeviceContext>& ctx, const Sets::Set* set, bool shouldIgnoreSet)
{
	if (!SettingsMenu::i().isVisible())
	{
		selectedId_ = Unselected();
		testMouseMode_ = false;
	}

	DrawItems(set, shouldIgnoreSet);

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

void Grids::UpdateBuffsTable(StackedBuff* buffs)
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

void Grids::Load()
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
	auto getivec4 = [](const json& j) { return glm::ivec4(j[0].get<int>(), j[1].get<int>(), j[2].get<int>(), j[3].get<int>()); };
	auto getImVec4 = [](const json& j) { return ImVec4(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>()); };

	const auto& grids = cfg.json()["buff_grids"];
	for (const auto& gIn : grids)
	{
		Grid g;
		g.spacing = maybe_at(gIn, "spacing", glm::ivec2(32, 32), { getivec2 });
		g.offset = maybe_at(gIn, "offset", glm::ivec2(), { getivec2 });
		g.attached = maybe_at(gIn, "attached", false);
		g.centralWeight = maybe_at(gIn, "central_weight", 0.f);
		g.mouseClipMin = maybe_at(gIn, "mouse_clip_min", glm::ivec2{ std::numeric_limits<int>::max() }, { getivec2 });
		g.mouseClipMax = maybe_at(gIn, "mouse_clip_max", glm::ivec2{ std::numeric_limits<int>::min() }, { getivec2 });
		g.trackMouseWhileHeld = maybe_at(gIn, "track_mouse_while_held", true);
		g.square = maybe_at(gIn, "square", true);
		g.name = gIn["name"];

		for (const auto& iIn : gIn["items"])
		{
			Item i;
			i.pos = getivec2(iIn["pos"]);
			i.buff = buffsMap_.at(iIn["buff_id"]);
			i.thresholds.clear();

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

void Grids::Save()
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
		grid["central_weight"] = g.centralWeight;
		grid["mouse_clip_min"] = { g.mouseClipMin.x, g.mouseClipMin.y };
		grid["mouse_clip_max"] = { g.mouseClipMax.x, g.mouseClipMax.y };
		grid["track_mouse_while_held"] = g.trackMouseWhileHeld;
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

#include "BuffsList.inc"

std::vector<Buff> Grids::GenerateBuffsList()
{
	const std::map<std::string, glm::vec4> atlasElements
	{
		#include <assets/atlas.inc>
	};

	std::vector<Buff> buffs;
	buffs.assign(g_Buffs.begin(), g_Buffs.end());

#ifdef _DEBUG
	std::set<std::string> buffNames;
	for(auto& b : buffs)
	{
		auto r = buffNames.insert(b.name);
		if(!r.second)
			LogWarn("Duplicate buff name: {}", b.name);
	}
#endif

	for (auto& b : buffs)
	{
		auto it = atlasElements.find(b.atlasEntry);
		b.uv = it != atlasElements.end() ? it->second : glm::vec4 { 0.f, 0.f, 0.f, 0.f };
	}

	return buffs;
}

std::map<int, const Buff*> Grids::GenerateBuffsMap(const std::vector<Buff>& lst)
{
	std::map<int, const Buff*> m;
	for (auto& b : lst)
	{
		m[b.id] = &b;
	}
	return m;
}

}