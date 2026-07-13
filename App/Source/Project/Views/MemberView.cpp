#include "MemberView.h"
#include "imgui.h"
#include "IconsFontAwesome6.h"

void MemberView::OnRender()
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	float totalWidth = ImGui::GetContentRegionAvail().x;
	ImVec2 pMin = ImGui::GetCursorScreenPos();

	ImVec4 cyanColor = ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f); // #00ADB5
	ImVec4 grayText = ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f); // #BBC9CA
	ImVec4 whiteText = ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f); // #E2E2E2
	ImVec4 borderColor = ImVec4(60.0f / 255.0f, 73.0f / 255.0f, 74.0f / 255.0f, 1.0f); // #3C494A
	ImVec4 redColor = ImVec4(255.0f / 255.0f, 11.0f / 255.0f, 11.0f / 255.0f, 1.0f); // #FF0B0B

	// --- Header Section ---
	ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
	ImGui::SetWindowFontScale(1.1f);
	ImGui::Text("Group Tracking Tool");
	ImGui::SetWindowFontScale(1.0f);
	ImGui::PopStyleColor();

	ImGui::SameLine(220.0f);

	// Date Range
	ImGui::PushStyleColor(ImGuiCol_Text, grayText);
	ImGui::Text(ICON_FA_CALENDAR " JUN 12 - OCT 28, 2026");
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
	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	// --- Row 1 ---
	ImGui::SetCursorPosX(30.0f);
	ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
	ImGui::Text("Alex\nRivera");
	ImGui::PopStyleColor();

	ImGui::SameLine(totalWidth * 0.5f);
	ImGui::PushStyleColor(ImGuiCol_Text, grayText);
	ImGui::Text("2023-10-\n24\n14:22:01");
	ImGui::PopStyleColor();

	// Right aligned KICK button
	ImGui::SameLine(totalWidth - 60.0f);
	ImVec2 btnPos = ImGui::GetCursorScreenPos();
	btnPos.y += 10.0f; // vertically center a bit
	
	drawList->AddRect(btnPos, ImVec2(btnPos.x + 50.0f, btnPos.y + 24.0f), ImGui::GetColorU32(redColor), 2.0f, 0, 1.0f);
	drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 0.8f, ImVec2(btnPos.x + 12.0f, btnPos.y + 5.0f), ImGui::GetColorU32(redColor), "KICK");
}
