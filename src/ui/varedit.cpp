#include "varedit.h"
#include "widgets.h"
#include "hkx/hkclass.inl"

#include <imgui.h>
#include <extern/imgui_stdlib.h>
#include <extern/font_awesome_5.h>

namespace Haviour
{
namespace Ui
{
VarEdit* VarEdit::getSingleton()
{
    static VarEdit edit;
    return std::addressof(edit);
}

void VarEdit::show()
{
    if (ImGui::Begin("Variable/Event List", &m_show, ImGuiWindowFlags_NoScrollbar))
    {
        auto file_manager = Hkx::HkxFileManager::getSingleton();
        if (file_manager->isFileSelected())
        {
            if (ImGui::BeginTable("varlisttbl", 2))
            {
                ImGui::TableNextColumn();
                ImGui::RadioButton("Variables", &m_show_list, kShowVariables);
                ImGui::RadioButton("Character Properties", &m_show_list, kShowProperties);
                addTooltip("This edits character properties within the behaviour file itself,\nnot the ones in the actual character.");
                ImGui::RadioButton("Attributes", &m_show_list, kShowAttributes);
                ImGui::Separator();
                switch (m_show_list)
                {
                    case kShowVariables:
                        showVarList();
                        break;
                    case kShowProperties:
                        showPropList();
                        break;
                    default: ImGui::TextDisabled("Not supported yet.");
                }
                ImGui::TableNextColumn();
                showEvtList();
                ImGui::EndTable();
            }
        }
        else
        {
            ImGui::TextDisabled("No loaded file.");
        }
    }
    ImGui::End();
}

void VarEdit::showVarList()
{
    ImGui::PushID("varedit");
    constexpr auto table_flag =
        ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody;

    bool  scroll_to_bottom = false;
    auto  file_manager     = Hkx::HkxFileManager::getSingleton();
    auto& current_file     = file_manager->getCurrentFile();

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Variables"), ImGui::SameLine();
    if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
        ImGui::OpenPopup("Type Select");
    addTooltip("Add new variable");
    if (ImGui::BeginPopup("Type Select"))
    {
        for (auto data_type : Hkx::e_variableType)
            if ((data_type != "VARIABLE_TYPE_INVALID") && ImGui::Selectable(data_type.data()))
            {
                ImGui::CloseCurrentPopup();
                scroll_to_bottom = true;
                current_file.m_var_manager.addEntry(Hkx::getVarTypeEnum(data_type));
                break;
            }
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FILTER))
        current_file.cleanupVariables();
    addTooltip("Remove unused variables");
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_HASHTAG))
        current_file.reindexVariables();
    addTooltip("Reindex variables\nDiscard all variables marked obsolete");

    ImGui::InputText("Filter", &m_var_filter), ImGui::SameLine();
    ImGui::Button(ICON_FA_QUESTION_CIRCLE);
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextColored(g_color_invalid, "Invalid");
        ImGui::TextColored(g_color_bool, "Boolean");
        ImGui::TextColored(g_color_int, "Int8/16/32");
        ImGui::TextColored(g_color_float, "Real");
        ImGui::TextColored(g_color_attr, "Pointer");
        ImGui::TextColored(g_color_quad, "Vector/Quaternion");
        ImGui::EndTooltip();
    }
    ImGui::Separator();

    if (ImGui::BeginTable("##VarList", 2, table_flag, ImVec2(-FLT_MIN, -FLT_MIN)))
    {
        ImGui::TableSetupColumn("id", ImGuiTableColumnFlags_WidthFixed, 36);
        ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();

        auto var_list = current_file.m_var_manager.getEntryList();
        std::erase_if(var_list,
                      [=](auto& var) {
                          auto var_disp_name = std::format("{:3} {}", var.m_index, var.get<Hkx::PropName>().text().as_string()); // :3
                          return !(var.m_valid &&
                                   (m_var_filter.empty() ||
                                    !std::ranges::search(var_disp_name, m_var_filter, [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }).empty()));
                      });

        ImGuiListClipper clipper;
        clipper.Begin(var_list.size());
        while (clipper.Step())
            for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
            {
                auto& var = var_list[row_n];

                ImGui::TableNextColumn();

                auto var_type      = var.get<Hkx::PropVarInfo>().getByName("type").text().as_string();
                auto var_type_enum = Hkx::getVarTypeEnum(var_type);
                if (var_type_enum < 0)
                    ImGui::PushStyleColor(ImGuiCol_Text, g_color_invalid);
                else if (var_type_enum < 1)
                    ImGui::PushStyleColor(ImGuiCol_Text, g_color_bool);
                else if (var_type_enum < 4)
                    ImGui::PushStyleColor(ImGuiCol_Text, g_color_int);
                else if (var_type_enum < 5)
                    ImGui::PushStyleColor(ImGuiCol_Text, g_color_float);
                else if (var_type_enum < 6)
                    ImGui::PushStyleColor(ImGuiCol_Text, g_color_attr);
                else
                    ImGui::PushStyleColor(ImGuiCol_Text, g_color_quad);
                ImGui::Text("%d", var.m_index);
                addTooltip(var_type);
                ImGui::PopStyleColor();

                ImGui::TableNextColumn();
                const bool is_selected = false;
                if (ImGui::Selectable(std::format("{}##{}", var.get<Hkx::PropName>().text().as_string(), var.m_index).c_str(), is_selected))
                {
                    m_var_current = var;
                    ImGui::OpenPopup("Editing Varibale");
                }
                addTooltip("Click to edit");
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
        varEditPopup("Editing Varibale", m_var_current, current_file);

        if (scroll_to_bottom)
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndTable();
    }
    ImGui::PopID();
}

void VarEdit::showEvtList()
{
    ImGui::PushID("evtlist");
    constexpr auto table_flag =
        ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody;

    bool  scroll_to_bottom = false;
    auto  file_manager     = Hkx::HkxFileManager::getSingleton();
    auto& current_file     = file_manager->getCurrentFile();

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Animation Events"), ImGui::SameLine();
    if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
    {
        current_file.m_evt_manager.addEntry();
        scroll_to_bottom = true;
    }
    addTooltip("Add new event");
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FILTER))
        current_file.cleanupEvents();
    addTooltip("Remove unused events");
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_HASHTAG))
        current_file.reindexEvents();
    addTooltip("Reindex events\nDiscard all events marked obsolete");

    ImGui::InputText("Filter", &m_evt_filter);

    ImGui::Separator();

    if (ImGui::BeginTable("##EvtList", 2, table_flag, ImVec2(-FLT_MIN, -FLT_MIN)))
    {
        ImGui::TableSetupColumn("id", ImGuiTableColumnFlags_WidthFixed, 36);
        ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();

        auto evt_list = current_file.m_evt_manager.getEntryList();

        std::erase_if(evt_list,
                      [=](auto& evt) {
                          auto disp_name = std::format("{:3} {}", evt.m_index, evt.get<Hkx::PropName>().text().as_string()); // :3
                          return !(evt.m_valid &&
                                   (m_evt_filter.empty() ||
                                    !std::ranges::search(disp_name, m_evt_filter, [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }).empty()));
                      });

        ImGuiListClipper clipper;
        clipper.Begin(evt_list.size());
        while (clipper.Step())
            for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
            {
                auto& evt = evt_list[row_n];

                ImGui::TableNextColumn();
                ImGui::Text("%d", evt.m_index);

                ImGui::TableNextColumn();
                const bool is_selected = false;
                if (ImGui::Selectable(std::format("{}##{}", evt.get<Hkx::PropName>().text().as_string(), evt.m_index).c_str(), is_selected))
                {
                    m_evt_current = evt;
                    ImGui::OpenPopup("Editing Event");
                }
                addTooltip("Click to edit");
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
        evtEditPopup("Editing Event", m_evt_current, current_file);

        if (scroll_to_bottom)
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndTable();
    }
    ImGui::PopID();
}

// Clone code
void VarEdit::showPropList()
{
    ImGui::PushID("proplist");
    constexpr auto table_flag =
        ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody;

    bool  scroll_to_bottom = false;
    auto  file_manager     = Hkx::HkxFileManager::getSingleton();
    auto& current_file     = file_manager->getCurrentFile();

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Character Properties"), ImGui::SameLine();
    if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
    {
        current_file.m_prop_manager.addEntry();
        scroll_to_bottom = true;
    }
    addTooltip("Add new property");
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FILTER))
        current_file.cleanupProps();
    addTooltip("Remove unused properties");
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_HASHTAG))
        current_file.reindexProperties();
    addTooltip("Reindex properties\nDiscard all events properties obsolete");

    ImGui::InputText("Filter", &m_prop_filter);

    ImGui::Separator();

    if (ImGui::BeginTable("##PropList", 2, table_flag, ImVec2(-FLT_MIN, -FLT_MIN)))
    {
        ImGui::TableSetupColumn("id", ImGuiTableColumnFlags_WidthFixed, 36);
        ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();

        auto prop_list = current_file.m_prop_manager.getEntryList();
        std::erase_if(
            prop_list,
            [=](auto& prop) {
                auto disp_name = std::format("{:3} {}", prop.m_index, prop.get<Hkx::PropName>().text().as_string()); // :3
                return !(prop.m_valid &&
                         (m_prop_filter.empty() ||
                          !std::ranges::search(disp_name, m_prop_filter, [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }).empty()));
            });

        ImGuiListClipper clipper;
        clipper.Begin(prop_list.size());
        while (clipper.Step())
            for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
            {
                auto& prop = prop_list[row_n];

                ImGui::TableNextColumn();
                ImGui::Text("%d", prop.m_index);

                ImGui::TableNextColumn();
                const bool is_selected = false;
                if (ImGui::Selectable(std::format("{}##{}", prop.get<Hkx::PropName>().text().as_string(), prop.m_index).c_str(), is_selected))
                {
                    m_prop_current = prop;
                    ImGui::OpenPopup("Editing Property");
                }
                addTooltip("Click to edit");
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
        propEditPopup("Editing Property", m_prop_current, current_file);

        if (scroll_to_bottom)
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndTable();
    }
    ImGui::PopID();
}
} // namespace Ui
} // namespace Haviour