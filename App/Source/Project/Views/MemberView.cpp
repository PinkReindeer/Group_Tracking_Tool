#include "MemberView.h"
#include "imgui.h"
#include "IconsFontAwesome6.h"
#include "Database/Database.h" 

void MemberView::OnRender(int projectId, const char* projectName, const char* createdDate)
{
    static bool needsRefresh = true; 
    static std::vector<TrackingTool::MemberInfo> members;

    if (needsRefresh) {
        members.clear();
        TrackingTool::Database::GetProjectMembers(projectId, members);
        needsRefresh = false; 
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    float totalWidth = ImGui::GetContentRegionAvail().x;

    // --- CẤU HÌNH MÀU SẮC ---
    ImVec4 cyanColor = ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f); 
    ImVec4 grayText = ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f);
    ImVec4 whiteText = ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f);
    ImVec4 borderColor = ImVec4(60.0f / 255.0f, 73.0f / 255.0f, 74.0f / 255.0f, 1.0f);
    ImVec4 redColor = ImVec4(255.0f / 255.0f, 11.0f / 255.0f, 11.0f / 255.0f, 1.0f);

    // --- Header ---
    ImGui::TextColored(whiteText, "%s", (projectName && projectName[0] != '\0') ? projectName : "Project");
    ImGui::SameLine(0.0f, 24.0f);
    ImGui::TextColored(grayText, "%s %s", ICON_FA_CALENDAR, (createdDate && createdDate[0] != '\0') ? createdDate : "—");

    ImGui::Dummy(ImVec2(0.0f, 10.0f));
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    drawList->AddLine(cursor, ImVec2(cursor.x + totalWidth, cursor.y), ImGui::GetColorU32(borderColor), 1.0f);
    ImGui::Dummy(ImVec2(0.0f, 10.0f));

    // --- Table Header ---
    ImGui::PushStyleColor(ImGuiCol_Text, cyanColor);
    ImGui::SetCursorPosX(30.0f); ImGui::Text("APPLICANT NAME");
    ImGui::SameLine(totalWidth * 0.5f); ImGui::Text("TIMESTAMP");
    ImGui::SameLine(totalWidth - 80.0f); ImGui::Text("ACTIONS");
    ImGui::PopStyleColor();

    for (const auto& member : members)
    {
        ImGui::Dummy(ImVec2(0.0f, 10.0f));
        
        ImGui::SetCursorPosX(30.0f);
        ImGui::TextColored(whiteText, "%s", member.Name.c_str());

        ImGui::SameLine(totalWidth * 0.5f);
        ImGui::TextColored(grayText, "%s", member.JoinDate.c_str());

        ImGui::SameLine(totalWidth - 70.0f);
        
        std::string btnId = "Kick##" + member.Name; 
        
        ImGui::PushStyleColor(ImGuiCol_Button, redColor);
        if (ImGui::Button(btnId.c_str(), ImVec2(60.0f, 24.0f)))
        {
            if (TrackingTool::Database::RemoveMember(projectId, member.Name))
            {
                printf("Successfully removed: %s\n", member.Name.c_str());
                needsRefresh = true; 
            }
        }
        ImGui::PopStyleColor(1); 
    }
}