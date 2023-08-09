#pragma once

#include <imgui.h>

#include "Common.h"

namespace GW2Clarity::UI
{

class SelectableListBox
{
public:
    SelectableListBox(i32 id, std::string typeName, std::string title, std::string hintsText)
        : id_ { id }, typeName_ { std::move(typeName) }, title_ { std::move(title) }, hintsText_ { std::move(hintsText) } { }

    template<ranges::input_range T>
    requires ranges::sized_range<T>
    bool Draw(T&& itemNames) {
        if(ImGui::BeginListBox(std::format("##{}List", typeName_).c_str(), ImVec2(-FLT_MIN, 0.f))) {
            for(auto&& [id, name] : itemNames | ranges::views::enumerate) {
                if(ImGui::Selectable(std::format("{}##{}", name, typeName_).c_str(), id_ == id))
                    id_ = id;
            }
            ImGui::EndListBox();
        }
        if(ImGui::Button(std::format("Add {}", typeName_).c_str())) {
            id_ = ranges::size(itemNames);
            return true;
        }

        return false;
    }

    [[nodiscard]] i32 id() const { return id_; }

private:
    i32 id_ = -1;
    std::string typeName_;
    std::string title_;
    std::string hintsText_;
};

} // namespace GW2Clarity::UI