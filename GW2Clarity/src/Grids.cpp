#include <Core.h>
#include <Grids.h>
#include <ImGuiExtensions.h>
#include <SimpleIni.h>
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <range/v3/all.hpp>
#include <variant>

namespace GW2Clarity
{
template<VectorLike<float, 2> T>
T AdjustToArea(float w, float h, float availW)
{
    T dims(w, h);
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

Grids::Grids(ComPtr<ID3D11Device>& dev, const Buffs* buffs, const Styles* styles)
    : enableBetterFiltering_("Enable better texture filtering", "better_tex_filtering", "Grids", true)
    , buffs_(buffs)
    , styles_(styles)
    , gridRenderer_(dev, buffs)
    , selector_(buffs, "")
{
    Input::i().mouseButtonEvent().AddCallback(
        [&](EventKey ek, bool&)
        {
            bool wasHolding = holdingMouseButton_ != ScanCode::NONE;
            if (ek.sc == ScanCode::LBUTTON || ek.sc == ScanCode::RBUTTON)
                holdingMouseButton_ = ek.down ? (holdingMouseButton_ | ek.sc) : (holdingMouseButton_ & ~ek.sc);
            bool isHolding = holdingMouseButton_ != ScanCode::NONE;
            if (!wasHolding && isHolding)
                heldMousePos_ = ImGui::GetIO().MousePos;
        });

    Load();

    SettingsMenu::i().AddImplementer(this);
}

Grids::~Grids()
{
    SettingsMenu::f([&](auto& i) { i.RemoveImplementer(this); });
}

void Grids::DrawEditingGrid()
{
    if (selectedId_.grid != UnselectedSubId)
    {
        const glm::vec2 screen{ ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y };
        const glm::vec2 mouse{ ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y };

        const auto&     sp      = grid().spacing;
        auto            c       = grid().ComputeOrigin(*this, !testMouseMode_, screen, mouse);
        auto*           cmdList = ImGui::GetBackgroundDrawList();
        cmdList->PushClipRectFullScreen();

        int  i     = 0;
        auto color = [&i]() { return i != 0 ? (i % 4 == 0 ? 0xCCFFFFFF : 0x55FFFFFF) : 0xFFFFFFFF; };
        for (float x = 0.f; x < c.x; x += sp.x)
        {
            cmdList->AddLine(ImVec2(c.x + x, 0.f), ImVec2(c.x + x, screen.y), color(), 1.2f);
            cmdList->AddLine(ImVec2(c.x - x, 0.f), ImVec2(c.x - x, screen.y), color(), 1.2f);
            i++;
        }

        i = 0;
        for (float y = 0.f; y < c.y; y += sp.y)
        {
            cmdList->AddLine(ImVec2(0.f, c.y + y), ImVec2(screen.x, c.y + y), color(), 1.2f);
            cmdList->AddLine(ImVec2(0.f, c.y - y), ImVec2(screen.x, c.y - y), color(), 1.2f);
            i++;
        }

        cmdList->AddCircle(ImVec2(c.x, c.y), 4.f, 0xFF0000FF);

        cmdList->PopClipRect();
    }

    if (draggingMouseBoundaries_)
    {
        auto* cmdList = ImGui::GetBackgroundDrawList();
        cmdList->PushClipRectFullScreen();
        cmdList->AddRect(ToImGui(grid().mouseClipMin), ToImGui(grid().mouseClipMax), 0xFFFF0000);
        cmdList->PopClipRect();

        draggingMouseBoundaries_ = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
        if (!draggingMouseBoundaries_)
            needsSaving_ = true;
    }
}

void Grids::DrawGridList()
{
    if (ImGui::BeginTable("##GridsAndItems", 2))
    {
        auto newCurrentHovered = Unselected();
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Grids");
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Items");

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        if (ImGui::BeginListBox("##GridsList", ImVec2(-FLT_MIN, 0.f)))
        {
            for (auto it : grids_ | ranges::views::enumerate)
            {
                // Need explicit types to shut up IntelliSense, still present as of 17.5.1
                short gid = short(it.first);
                Grid& g   = it.second;

                auto  u   = Unselected(gid);
                if (ImGui::Selectable(std::format("{} ({}x{})##{}", g.name, g.spacing.x, g.spacing.y, gid).c_str(), selectedId_ == u || currentHovered_ == u,
                                      ImGuiSelectableFlags_AllowItemOverlap))
                {
                    selectedId_ = u;
                }

                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) || ImGui::IsItemActive())
                {
                    auto& style = ImGui::GetStyle();
                    auto  orig  = style.Colors[ImGuiCol_Button];
                    style.Colors[ImGuiCol_Button] *= ImVec4(0.5f, 0.5f, 0.5f, 1.f);
                    if (ImGuiClose(std::format("CloseGrid{}", gid).c_str(), 0.75f, false))
                    {
                        selectedId_ = u;
                        Core::i().DisplayDeletionMenu({ g.name, "grid", "", selectedId_ });
                    }
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
                        newCurrentHovered = u;

                    style.Colors[ImGuiCol_Button] = orig;
                }
            }

            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                selectedId_ = Unselected();
            }

            ImGui::EndListBox();
        }
        ImGui::TableNextColumn();

        const bool disableItemsList = selectedId_.grid < 0;

        ImGui::BeginDisabled(disableItemsList);
        if (ImGui::BeginListBox("##ItemsList", ImVec2(-FLT_MIN, 0.f)))
        {
            if (selectedId_.grid != UnselectedSubId)
            {
                auto& g   = grid();
                auto  gid = selectedId_.grid;
                for (auto it : g.items | ranges::views::enumerate)
                {
                    // Need explicit types to shut up IntelliSense, still present as of 17.5.1
                    short       iid = it.first;
                    Item&       i   = it.second;

                    Id          id{ gid, iid };
                    std::string name = std::format("{} ({}, {})##{}", i.buff->name.c_str(), i.pos.x, i.pos.y, iid);
                    if (ImGui::Selectable(name.c_str(), selectedId_ == id || currentHovered_ == id, ImGuiSelectableFlags_AllowItemOverlap))
                    {
                        selectedId_ = id;
                    }

                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) || ImGui::IsItemActive())
                    {
                        auto& style = ImGui::GetStyle();
                        auto  orig  = style.Colors[ImGuiCol_Button];
                        style.Colors[ImGuiCol_Button] *= ImVec4(0.5f, 0.5f, 0.5f, 1.f);
                        if (ImGuiClose(std::format("CloseItem{}", id.id).c_str(), 0.75f, false))
                        {
                            selectedId_ = id;
                            Core::i().DisplayDeletionMenu({ name, "item", std::format(" from grid '{}'", g.name), selectedId_ });
                        }
                        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
                            newCurrentHovered = id;

                        style.Colors[ImGuiCol_Button] = orig;
                    }
                }

                if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
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
        if (ImGui::Button("New grid"))
        {
            grids_.emplace_back();
            selectedId_  = Unselected(grids_.size() - 1);
            needsSaving_ = true;
        }
        ImGui::TableNextColumn();
        ImGui::BeginDisabled(disableItemsList);
        if (ImGui::Button("New item"))
        {
            grid().items.emplace_back();
            selectedId_  = { selectedId_.grid, grid().items.size() - 1 };
            needsSaving_ = true;
        }
        ImGui::EndDisabled();

        ImGui::EndTable();

        currentHovered_ = newCurrentHovered;
    }
}

void Grids::DrawItems(ComPtr<ID3D11DeviceContext>& ctx, const Layouts::Layout* layout, bool shouldIgnoreLayout)
{
    bool editMode = selectedId_.grid != UnselectedSubId;
#ifdef _DEBUG
    bool showDebugGrid = debugGridFilter_.length() >= 3;
#else
    bool showDebugGrid = false;
#endif

    if (layout || shouldIgnoreLayout || editMode || showDebugGrid)
    {
        auto  currentTime        = TimeInMilliseconds();
        float editingBorderCycle = sin(float(currentTime) * 2.f * M_PI / 1000.f) * 0.5f + 0.5f;

        if (!MumbleLink::i().isInCompetitiveMode() && (editMode || shouldIgnoreLayout || showDebugGrid || !layout->combatOnly || MumbleLink::i().isInCombat()))
        {
            const glm::vec2 screen{ ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y };
            const glm::vec2 mouse{ ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y };

            auto            drawItem = [&](const glm::ivec2& spacing, const Item& i, const glm::vec2& gridOrigin, int count, bool editing)
            {
                glm::vec2        pos = gridOrigin + glm::vec2(i.pos * spacing);

                auto             adj = AdjustToArea<glm::vec2>(128.f, 128.f, float(spacing.x));

                GridInstanceData inst;
                inst.posDims = glm::vec4(pos, adj) / glm::vec4(screen, screen);
                inst.uv      = i.buff->uv;
                if ((inst.showNumber = i.buff->ShowNumber(count)))
                    inst.numberUV = buffs_->GetNumber(count);

                styles_->ApplyStyle(i.style, count, inst);

                if (editing)
                {
                    inst.borderColor     = glm::mix(inst.borderColor, glm::vec4(1, 0, 0, 1), editingBorderCycle);
                    inst.borderThickness = std::max(inst.borderThickness, 1.f);
                }

                gridRenderer_.Add(std::move(inst));
            };

            auto drawGrid = [&](const Grid& g, short gid)
            {
                auto gridOrigin = g.ComputeOrigin(*this, editMode, screen, mouse);

                for (auto it : g.items | ranges::views::enumerate)
                {
                    // Need explicit types to shut up IntelliSense, still present as of 17.5.1
                    short       iid     = it.first;
                    const Item& i       = it.second;

                    bool        editing = editMode && selectedId_ == Id{ gid, iid };
                    int         count   = 0;
                    if (editing)
                        count = editingItemFakeCount_;
                    else if (i.buff)
                        count = std::accumulate(i.additionalBuffs.begin(), i.additionalBuffs.end(), i.buff->GetStacks(buffs_->activeBuffs()),
                                                [&](int a, const Buff* b) { return a + b->GetStacks(buffs_->activeBuffs()); });

                    drawItem(g.spacing, i, gridOrigin, count, editing);
                }
            };

            if (editMode)
                drawGrid(grid(), selectedId_.grid);
            else if (shouldIgnoreLayout)
                for (const auto& g : grids_)
                    drawGrid(g, UnselectedSubId);
            else if (layout)
                for (int gid : layout->grids)
                    drawGrid(grids_[gid], UnselectedSubId);

            gridRenderer_.Draw(ctx, enableBetterFiltering_.value());
#if 0
#ifdef _DEBUG
                const Buff* hoveredBuff = nullptr;
                if (showDebugGrid)
                {
                    const auto lowerCaseDebugGridFilter = ToLower(debugGridFilter_);
                    glm::ivec2 dir(1, 0);
                    int        steps = 1, stepCount = 1;
                    Item       fakeItem{
                        glm::ivec2(0), nullptr, {                           },
                          { { 1, ImVec4(1, 1, 1, 0.33f) }, { 100, ImVec4(1, 1, 1, 1) } }
                    };

                    for (const auto& b : buffs_)
                    {
                        if (b.id == 0xFFFFFFFF || !ToLower(b.name).contains(lowerCaseDebugGridFilter) && !ToLower(b.category).contains(lowerCaseDebugGridFilter))
                            continue;

                        fakeItem.buff = &b;

                        int count     = b.GetStacks(activeBuffs_);
                        drawItem(glm::ivec2(64), fakeItem, screen * 0.5f, count, false);

                        glm::vec2 minPos = glm::vec2(fakeItem.pos * 64) + screen * 0.5f;
                        glm::vec2 maxPos = minPos + 64.f;

                        if (glm::all(glm::greaterThanEqual(baseMouse, minPos)) && glm::all(glm::lessThan(baseMouse, maxPos)))
                            hoveredBuff = &b;

                        fakeItem.pos += dir;
                        steps--;
                        if (steps == 0)
                        {
                            dir = glm::ivec2(-dir.y, dir.x);
                            if (dir.x != 0)
                                stepCount += 1;
                            steps = stepCount;
                        }
                    }
                }
#endif

                for (const auto& dd : delayedDraws)
                {
                    ImGui::SetCursorPos(dd.pos);
                    ImGui::Image(numbersAtlas_.srv.Get(), dd.adj, dd.uv1, dd.uv2);
                }

#ifdef _DEBUG
                if (hoveredBuff)
                    ImGui::SetTooltip("%s", hoveredBuff->name.c_str());
#endif
#endif
        }
    }
}

void Grids::Delete(Id id)
{
    if (id.item >= 0)
    {
        auto& g = grid(id);
        g.items.erase(g.items.begin() + id.item);

        if (id == selectedId_)
            selectedId_ = Unselected(id.grid);

        needsSaving_ = true;
    }
    else
    {
        grids_.erase(grids_.begin() + id.grid);

        if (id == selectedId_)
            selectedId_ = Unselected();
        needsSaving_ = true;
    }
}

void Grids::StyleDeleted(uint id)
{
    for (auto& g : grids_)
    {
        for (auto& i : g.items)
        {
            if (i.style == id)
                i.style = 0;
        }
    }

    needsSaving_ = true;
}

void Grids::DrawMenu(Keybind** currentEditedKeybind)
{
    if (selectedId_.grid != UnselectedSubId)
    {
        const auto& sp    = grid().spacing;
        auto        mouse = ImGui::GetIO().MousePos - ImGui::GetIO().DisplaySize * 0.5f;

        auto        pos   = glm::ivec2(glm::floor(glm::vec2(mouse.x, mouse.y) / glm::vec2(sp)));

        ImGui::SetNextWindowPos(ToImGui(glm::vec2(pos * sp) - glm::vec2(0.f, sp.y * 0.2f) + FromImGui(ImGui::GetIO().DisplaySize * 0.5f)), ImGuiCond_Always, ImVec2(0.5f, 1.f));
        if (ImGui::Begin("##GridElementTooltip", nullptr, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize))
        {
            auto it = ranges::find_if(grid().items, [&](const auto& i) { return i.pos.x == pos.x && i.pos.y == pos.y; });

            if (it != grid().items.end())
            {
                ImGui::TextUnformatted(it->buff->name.c_str());
                ImGui::Text("(%d, %d)", it->pos.x, it->pos.y);
            }
            else
            {
                ImGui::TextUnformatted("<no buff>");
                ImGui::Text("(%d, %d)", pos.x, pos.y);
            }
        }
        ImGui::End();
    }

    DrawEditingGrid();

    ImGuiConfigurationWrapper(ImGui::Checkbox, enableBetterFiltering_);
    ImGuiHelpTooltip("Enables higher quality texture filtering, improving the icons' appearance at a cost to performance.");

    auto saveCheck = [this](bool changed)
    {
        needsSaving_ = needsSaving_ || changed;
        return changed;
    };

    {
        DrawGridList();

        bool editingGrid = selectedId_.grid >= 0 && selectedId_.item == UnselectedSubId;
        bool editingItem = selectedId_.item >= 0;
        if (!editingGrid)
            testMouseMode_ = false;
        if (editingGrid)
        {
            auto& editGrid = grid();
            ImGuiTitle(std::format("Editing Grid '{}'", editGrid.name).c_str(), 0.75f);

            saveCheck(ImGui::InputText("Grid Name", &editGrid.name));
            ImGui::NewLine();

            // If drag was triggered, stay on as long as dragging occurs, otherwise disable
            draggingMouseBoundaries_ = draggingMouseBoundaries_ ? ImGui::IsMouseDragging(ImGuiMouseButton_Left) : false;

            saveCheck(ImGui::Checkbox("Square Grid", &editGrid.square));
            if (editGrid.square)
                saveCheck(ImGui::DragInt("Grid Scale", (int*)&editGrid.spacing, 0.1f, 1, 2048));
            else
                saveCheck(ImGui::DragInt2("Grid Scale", glm::value_ptr(editGrid.spacing), 0.1f, 1, 2048));

            if (editGrid.square)
                editGrid.spacing.y = editGrid.spacing.x;

            saveCheck(ImGui::DragInt2("Grid Offset", glm::value_ptr(editGrid.offset), 0.1f, -int(ImGui::GetIO().DisplaySize.x) / 2, int(ImGui::GetIO().DisplaySize.x) / 2));

            saveCheck(ImGui::Checkbox("Attached to Mouse", &editGrid.attached));
            if (editGrid.attached)
            {
                ImGui::Indent();

                saveCheck(ImGui::DragFloat("Center Weight", &editGrid.centralWeight, 0.01f, 0.f, 1.f));
                ImGuiHelpTooltip("Reduces the impact of the mouse cursor's location: 0.0 locks to the mouse perfectly whereas 1.0 locks to the center of the screen.");

                saveCheck(ImGui::Checkbox("Track Mouse While Button Is Held", &editGrid.trackMouseWhileHeld));
                ImGuiHelpTooltip("Controls whether to follow the mouse motion when it is hidden while a mouse button is held.");

                bool showMouseClip     = editGrid.mouseClipMin.x != std::numeric_limits<int>::max();
                bool lastShowMouseClip = showMouseClip;
                saveCheck(ImGui::Checkbox("Enable Mouse Boundaries", &showMouseClip));
                ImGuiHelpTooltip("Mouse boundaries constrain the effective mouse location within a certain region of the screen. Useful to prevent an attached grid from moving "
                                 "too far to the side of the screen.");
                if (showMouseClip && !lastShowMouseClip)
                {
                    editGrid.mouseClipMin = glm::ivec2{ 0 };
                    editGrid.mouseClipMax = glm::ivec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);
                }
                else if (!showMouseClip && lastShowMouseClip)
                {
                    editGrid.mouseClipMin = glm::ivec2{ std::numeric_limits<int>::max() };
                    editGrid.mouseClipMax = glm::ivec2{ std::numeric_limits<int>::min() };
                }

                if (showMouseClip)
                {
                    ImGui::TextUnformatted("Mouse boundaries");
                    ImGui::Indent();
                    bool editingAny = false;
                    editingAny |= saveCheck(ImGui::DragInt("Mouse Left", &editGrid.mouseClipMin.x, 1.f, 0, int(ImGui::GetIO().DisplaySize.x)));
                    editingAny |= saveCheck(ImGui::DragInt("Mouse Right", &editGrid.mouseClipMax.x, 1.f, 0, int(ImGui::GetIO().DisplaySize.x)));
                    editingAny |= saveCheck(ImGui::DragInt("Mouse Top", &editGrid.mouseClipMin.y, 1.f, 0, int(ImGui::GetIO().DisplaySize.y)));
                    editingAny |= saveCheck(ImGui::DragInt("Mouse Bottom", &editGrid.mouseClipMax.y, 1.f, 0, int(ImGui::GetIO().DisplaySize.y)));
                    ImGui::Unindent();

                    if (editingAny)
                        draggingMouseBoundaries_ = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
                }

                if (ImGui::Button(testMouseMode_ ? "Stop Testing" : "Begin Mouse Test"))
                    testMouseMode_ = !testMouseMode_;

                ImGui::Unindent();
            }
        }
        else if (editingItem)
        {
            auto& editItem = item();
            ImGuiTitle(std::format("Editing Item '{}' of '{}'", editItem.buff->name, grid().name).c_str(), 0.75f);

            auto buffCombo = [&](auto& buff, int id, const char* name)
            {
                if (saveCheck(selector_.Draw(std::format("{}##{}", name, id).c_str())))
                    buff = selector_.selectedBuff();
            };

            buffCombo(editItem.buff, -1, "Main buff");
            int removeId = -1;
            for (auto it : editItem.additionalBuffs | ranges::views::enumerate)
            {
                // Need explicit types to shut up IntelliSense, still present as of 17.5.1
                short        n         = short(it.first);
                const Buff*& extraBuff = it.second;

                buffCombo(extraBuff, int(n), "Secondary buff");
                ImGui::SameLine();
                if (ImGuiClose(std::format("RemoveExtraBuff{}", n).c_str()))
                    removeId = int(n);
            }
            if (removeId != -1)
                editItem.additionalBuffs.erase(editItem.additionalBuffs.begin() + removeId);

            if (ImGui::Button("Add secondary buff"))
                editItem.additionalBuffs.push_back(&Buffs::UnknownBuff);
            ImGuiHelpTooltip(
                "Secondary buffs will activate the buff on screen, but without changing the icon. Useful to combine multiple related effects (e.g. Fixated) in one icon.");

            ImGui::NewLine();

            saveCheck(ImGui::DragInt2("Location", glm::value_ptr(editItem.pos), 0.1f));

            if (ImGui::BeginCombo("Style", styles_->style(editItem.style).name.c_str()))
            {
                for (auto it : styles_->styles() | ranges::views::enumerate)
                {
                    uint         sid = it.first;
                    const Style& s   = it.second;
                    if (saveCheck(ImGui::Selectable(s.name.c_str(), sid == editItem.style)))
                        editItem.style = sid;
                }

                ImGui::EndCombo();
            }
        }
    }

    mstime currentTime = TimeInMilliseconds();
    if (needsSaving_ && lastSaveTime_ + SaveDelay <= currentTime)
        Save();
}

void Grids::Draw(ComPtr<ID3D11DeviceContext>& ctx, const Layouts::Layout* layout, bool shouldIgnoreLayout)
{
    if (!SettingsMenu::i().isVisible())
    {
        selectedId_    = Unselected();
        testMouseMode_ = false;
    }

    DrawItems(ctx, layout, shouldIgnoreLayout);

    firstDraw_ = false;
}

void Grids::Load()
{
    using namespace nlohmann;
    grids_.clear();
    selectedId_ = Unselected();

    auto& cfg   = JSONConfigurationFile::i();
    cfg.Reload();

    auto maybeAt = []<typename D>(const json& j, const char* n, const D& def, const std::variant<std::monostate, std::function<D(const json&)>>& cvt = {})
    {
        auto it = j.find(n);
        if (it == j.end())
            return def;
        if (cvt.index() == 0)
            return static_cast<D>(*it);
        else
            return std::get<1>(cvt)(*it);
    };

    auto getivec2  = [](const json& j) { return glm::ivec2(j[0].get<int>(), j[1].get<int>()); };
    auto getivec4  = [](const json& j) { return glm::ivec4(j[0].get<int>(), j[1].get<int>(), j[2].get<int>(), j[3].get<int>()); };
    auto getImVec4 = [](const json& j) { return ImVec4(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>()); };
    auto getvec4   = [](const json& j) { return glm::vec4(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>()); };
    auto getBuff   = [this](const json& j) -> const Buff*
    {
        int  id = j;
        auto it = buffs_->buffsMap().find(id);
        if (it != buffs_->buffsMap().end())
            return it->second;
        else
            return nullptr;
    };

    const auto& grids = cfg.json()["buff_grids"];
    for (const auto& gIn : grids)
    {
        Grid g;
        g.spacing             = maybeAt(gIn, "spacing", glm::ivec2(32, 32), { getivec2 });
        g.offset              = maybeAt(gIn, "offset", glm::ivec2(), { getivec2 });
        g.attached            = maybeAt(gIn, "attached", false);
        g.centralWeight       = maybeAt(gIn, "central_weight", 0.f);
        g.mouseClipMin        = maybeAt(gIn, "mouse_clip_min", glm::ivec2{ std::numeric_limits<int>::max() }, { getivec2 });
        g.mouseClipMax        = maybeAt(gIn, "mouse_clip_max", glm::ivec2{ std::numeric_limits<int>::min() }, { getivec2 });
        g.trackMouseWhileHeld = maybeAt(gIn, "track_mouse_while_held", true);
        g.square              = maybeAt(gIn, "square", true);
        g.name                = gIn["name"];

        for (const auto& iIn : gIn["items"])
        {
            Item i;
            i.pos   = getivec2(iIn["pos"]);
            i.buff  = getBuff(iIn["buff_id"]);
            i.style = styles_->FindStyle(maybeAt(iIn, "style", std::string{}));

            if (iIn.contains("additional_buff_ids"))
                for (auto& bIn : iIn["additional_buff_ids"])
                    if (const Buff* b = getBuff(bIn); b)
                        i.additionalBuffs.push_back(b);

            if (!i.buff)
            {
                i.buff = &Buffs::UnknownBuff;
                LogWarn("Configuration has unknown buff: Grid '{}', location ({}, {}), buff ID '{}'.", g.name, i.pos.x, i.pos.y, static_cast<std::string>(iIn["buff_id"]));
            }

            if (!i.buff && !i.additionalBuffs.empty())
            {
                i.buff = i.additionalBuffs.back();
                i.additionalBuffs.pop_back();
            }

            g.items.push_back(i);
        }
        grids_.push_back(g);
    }
}

void Grids::Save()
{
    using namespace nlohmann;

    auto& cfg   = JSONConfigurationFile::i();
    auto& grids = cfg.json()["buff_grids"];
    grids       = json::array();
    for (const auto& g : grids_)
    {
        json grid;
        grid["spacing"]                = { g.spacing.x, g.spacing.y };
        grid["offset"]                 = { g.offset.x, g.offset.y };
        grid["attached"]               = g.attached;
        grid["central_weight"]         = g.centralWeight;
        grid["mouse_clip_min"]         = { g.mouseClipMin.x, g.mouseClipMin.y };
        grid["mouse_clip_max"]         = { g.mouseClipMax.x, g.mouseClipMax.y };
        grid["track_mouse_while_held"] = g.trackMouseWhileHeld;
        grid["square"]                 = g.square;
        grid["name"]                   = g.name;

        json& gridItems                = grid["items"];

        for (const auto& i : g.items)
        {
            json item;
            item["pos"]     = { i.pos.x, i.pos.y };
            item["buff_id"] = i.buff->id;
            item["style"]   = styles_->style(i.style).name;

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

    needsSaving_  = false;
    lastSaveTime_ = TimeInMilliseconds();
}

} // namespace GW2Clarity