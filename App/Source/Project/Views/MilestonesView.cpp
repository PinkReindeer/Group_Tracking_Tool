#include "MemberView.h"
#include "MilestonesView.h"
#include "imgui.h"
#include "IconsFontAwesome6.h"
#include "Database/Database.h"

void MilestonesView::OnRender(int projectId, const char* projectName, const char* createdDate)
{
    std::vector<TrackingTool::MemberInfo> members;
    TrackingTool::Database::GetProjectMembers(projectId, members);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    float totalWidth = ImGui::GetContentRegionAvail().x;
    ImVec2 pMin = ImGui::GetCursorScreenPos();

    ImVec4 cyanColor = ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f);
    ImVec4 grayText = ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f);
    ImVec4 whiteText = ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f);
    ImVec4 borderColor = ImVec4(60.0f / 255.0f, 73.0f / 255.0f, 74.0f / 255.0f, 1.0f);
    ImVec4 redColor = ImVec4(255.0f / 255.0f, 11.0f / 255.0f, 11.0f / 255.0f, 1.0f);

    const char* displayName = (projectName && projectName[0] != '\0') ? projectName : "Project";
    const char* displayDate = (createdDate && createdDate[0] != '\0') ? createdDate : "—";

    // --- Header Section ---
    ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
    ImGui::SetWindowFontScale(1.1f);
    ImGui::Text("%s", displayName);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 24.0f);

    ImGui::PushStyleColor(ImGuiCol_Text, grayText);
    ImGui::Text("%s %s", ICON_FA_CALENDAR, displayDate);
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0.0f, 10.0f));

    // Separator Line
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    drawList->AddLine(cursor, ImVec2(cursor.x + totalWidth, cursor.y), ImGui::GetColorU32(borderColor), 1.0f);
    ImGui::Dummy(ImVec2(0.0f, 10.0f));

    // --- Table Header ---
    ImGui::PushStyleColor(ImGuiCol_Text, cyanColor);
    
    ImGui::SetCursorPosX(30.0f);
    ImGui::Text("APPLICANT NAME");
    
    ImGui::SameLine(totalWidth * 0.5f);
    ImGui::Text("TIMESTAMP");

    ImGui::SameLine(totalWidth - 80.0f);
    ImGui::Text("ACTIONS");

    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0.0f, 10.0f));
    cursor = ImGui::GetCursorScreenPos();
    drawList->AddLine(cursor, ImVec2(cursor.x + totalWidth, cursor.y), ImGui::GetColorU32(borderColor), 1.0f);
    
    for (const auto& member : members)
    {
        ImGui::Dummy(ImVec2(0.0f, 15.0f));
        
        // Tên
        ImGui::SetCursorPosX(30.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
        ImGui::Text("%s", member.Name.c_str());
        ImGui::PopStyleColor();

        ImGui::SameLine(totalWidth * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, grayText);
        ImGui::Text("%s", member.JoinDate.c_str());
        ImGui::PopStyleColor();

        ImGui::SameLine(totalWidth - 80.0f);
        
        ImGui::PushStyleColor(ImGuiCol_Text, redColor);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Border, redColor);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        
        std::string btnId = "KICK##" + member.Name;
        if (ImGui::Button(btnId.c_str(), ImVec2(50.0f, 24.0f)))
        {
            printf("Kick clicked for: %s\n", member.Name.c_str());
        }
        
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
    }
}