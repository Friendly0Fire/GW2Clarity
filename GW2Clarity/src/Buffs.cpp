#include <Buffs.h>
#include <Core.h>
#include <ImGuiExtensions.h>
#include <cppcodec/base64_rfc4648.hpp>
#include <misc/cpp/imgui_stdlib.h>
#include <shellapi.h>
#include <skyr/percent_encoding/percent_encode.hpp>

namespace GW2Clarity
{

std::vector<glm::vec2> GenerateNumbersMap(glm::vec2& uvSize)
{
    const std::map<std::string, glm::vec2> atlasElements{
#include <assets/numbers.inc>
    };

    uvSize = atlasElements.at("");

    std::vector<glm::vec2> numbers;
    numbers.resize(atlasElements.size() - 1);

    for (const auto& [name, pos] : atlasElements)
    {
        if (name.empty())
            continue;

        int idx          = strtol(name.c_str(), nullptr, 10);
        numbers[idx - 2] = pos;
    }

    return numbers;
}

Buffs::Buffs(ComPtr<ID3D11Device>& dev)
    : buffs_(GenerateBuffsList(buffsAtlasUVSize_))
    , buffsMap_(GenerateBuffsMap(buffs_))
    , numbers_(GenerateNumbersMap(numbersAtlasUVSize_))
{
    buffsAtlas_   = CreateTextureFromResource(dev.Get(), Core::i().dllModule(), IDR_BUFFS);
    numbersAtlas_ = CreateTextureFromResource(dev.Get(), Core::i().dllModule(), IDR_NUMBERS);

#ifdef _DEBUG
    SettingsMenu::i().AddImplementer(this);

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
    namesPath                       = namesPath.remove_filename() / "names.tsv";

    std::ifstream namesFile(namesPath);
    if (namesFile.good())
    {
        std::string              line;
        std::vector<std::string> cols;
        bool                     header = true;
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

void Buffs::SaveNames() const
{
    wchar_t fn[MAX_PATH];
    GetModuleFileName(GetBaseCore().dllModule(), fn, MAX_PATH);

    std::filesystem::path namesPath = fn;
    namesPath                       = namesPath.remove_filename() / "names.tsv";

    std::ofstream namesFile(namesPath, std::ofstream::trunc | std::ofstream::out);
    if (namesFile.good())
    {
        namesFile << "ID\tName" << std::endl;
        for (auto& n : buffNames_)
            namesFile << n.first << "\t" << n.second << std::endl;
    }
}

void Buffs::DrawMenu(Keybind** currentEditedKeybind)
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
            else
            {
                auto& str = buffNames_[id];
                if (ImGui::InputText(std::format("##Name{}", id).c_str(), &str))
                    SaveNames();
            }

            ImGui::TableNextColumn();

            byte chatCode[1 + 3 + 1];
            chatCode[0]             = 0x06;
            chatCode[1 + 3]         = 0x0;
            chatCode[1]             = id & 0xFF;
            chatCode[2]             = (id >> 8) & 0xFF;
            chatCode[3]             = (id >> 16) & 0xFF;

            using base64            = cppcodec::base64_rfc4648;

            std::string chatCodeStr = std::format("[&{}]", base64::encode(chatCode));

            ImGui::TextUnformatted(chatCodeStr.c_str());
            ImGui::SameLine();
            if (ImGui::Button(("Copy##" + chatCodeStr).c_str()))
                ImGui::SetClipboardText(chatCodeStr.c_str());
            ImGui::SameLine();
            if (ImGui::Button(std::format("Say in G{}##{}", guildLogId_, chatCodeStr).c_str()))
            {
                ImGui::SetClipboardText(std::format("/g{} {}: {}", guildLogId_, id, chatCodeStr).c_str());

                auto wait         = [](int i) { std::this_thread::sleep_for(std::chrono::milliseconds(i)); };
                auto sendKeyEvent = [](uint vk, ScanCode sc, bool down)
                {
                    INPUT i;
                    ZeroMemory(&i, sizeof(INPUT));
                    i.type       = INPUT_KEYBOARD;
                    i.ki.wVk     = vk;
                    i.ki.wScan   = ScanCode_t(sc);
                    i.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;

                    SendInput(1, &i, sizeof(INPUT));
                };

                std::thread(
                    [=]
                    {
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
                    })
                    .detach();
            }
            ImGui::SameLine();
            if (ImGui::Button(("Wiki##" + chatCodeStr).c_str()))
            {
                ShellExecute(0, 0, std::format(L"https://wiki.guildwars2.com/index.php?title=Special:Search&search={}", utf8_decode(skyr::percent_encode(chatCodeStr))).c_str(), 0,
                             0, SW_SHOW);
            }
        }
        ImGui::EndTable();
    }
}
#endif

void Buffs::UpdateBuffsTable(StackedBuff* buffs)
{
#ifdef _DEBUG
    for (auto& b : activeBuffs_)
        b.second = 0;
#else
    activeBuffs_.clear();
#endif

    if (buffs[0].id == 0)
    {
        int e              = lastGetBuffsError_;
        lastGetBuffsError_ = buffs[0].count;
        if (lastGetBuffsError_ != e)
        {
            switch (lastGetBuffsError_)
            {
                case -1:
                    Core::i().DisplayErrorPopup("Character context not found.");
                    break;
                case -2:
                    LogInfo("Addon is inactive in competitive modes.");
                    break;
                case -3:
                    LogInfo("Current character not set.");
                    break;
                case -4:
                    Core::i().DisplayErrorPopup("Fatal error while iterating through buff table.");
                    break;
                case -11:
                    Core::i().DisplayErrorPopup("Could not take threads snapshot.");
                    break;
                case -12:
                    Core::i().DisplayErrorPopup("Could not load ntdll.dll.");
                    break;
                case -13:
                    Core::i().DisplayErrorPopup("Could not find NtQueryInformationThread.");
                    break;
                case -14:
                    Core::i().DisplayErrorPopup("No matching thread found.");
                    break;
            }
        }
        return;
    }

    for (size_t i = 0; buffs[i].id; i++)
        activeBuffs_[buffs[i].id] = buffs[i].count;
}

bool Buffs::DrawBuffCombo(const char* name, const Buff*& selectedBuf, std::span<char> searchBuffer) const
{
    bool changed = false;
    if (ImGui::BeginCombo(name, selectedBuf ? selectedBuf->name.c_str() : "<none>"))
    {
        ImGui::InputText("Search...", searchBuffer.data(), searchBuffer.size());
        std::string_view bs{ searchBuffer.data() };

        for (auto& b : buffs_)
        {
            auto caseInsensitive = [](char l, char r) { return std::tolower(l) == std::tolower(r); };
            bool filtered        = !bs.empty() && ranges::search(b.name, bs, caseInsensitive).empty() && ranges::search(b.category, bs, caseInsensitive).empty();

            if (filtered)
                continue;

            if (b.id == 0xFFFFFFFF)
            {
                ImGui::PushFont(Core::i().fontBold());
                ImGui::Text(b.name.c_str());
                ImGui::PopFont();
                ImGui::Separator();
                continue;
            }

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.f);

            ImGui::Image(buffsAtlas_.srv.Get(), ImVec2(32, 32), ToImGui(b.uv.xy), ToImGui(b.uv.xy + buffsAtlasUVSize()));
            ImGui::SameLine();
            if (ImGui::Selectable(b.name.c_str(), false))
            {
                selectedBuf = &b;
                changed     = true;
            }
        }
        ImGui::EndCombo();
    }

    return changed;
}

#include "BuffsList.inc"

std::vector<Buff> Buffs::GenerateBuffsList(glm::vec2& uvSize)
{
    const std::map<std::string, glm::vec2> atlasElements{
#include <assets/atlas.inc>
    };

    uvSize = atlasElements.at("");

    std::vector<Buff> buffs;
    buffs.assign(g_Buffs.begin(), g_Buffs.end());

#ifdef _DEBUG
    std::set<std::string> buffNames;
    for (auto& b : buffs)
    {
        auto r = buffNames.insert(b.name);
        if (!r.second)
            LogWarn("Duplicate buff name: {}", b.name);
    }
#endif

    std::string cat;
    for (auto& b : buffs)
    {
        if (b.id == 0xFFFFFFFF)
            cat = b.name;
        else
            b.category = cat;

        auto it = atlasElements.find(b.atlasEntry);
        b.uv    = it != atlasElements.end() ? it->second : glm::vec4{ 0.f, 0.f, 0.f, 0.f };

        if (b.id != 0xFFFFFFFF && it == atlasElements.end())
            LogWarn("Buff {} ({}) has no atlas icon.", b.name, b.id);
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

} // namespace GW2Clarity