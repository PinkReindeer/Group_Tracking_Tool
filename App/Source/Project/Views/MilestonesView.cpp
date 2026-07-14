#include "imgui.h"

#include "MilestonesView.h"
#include "imgui.h"
#include "IconsFontAwesome6.h"

void MilestonesView::OnRender(const char* projectName, const char* createdDate)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	float totalWidth = ImGui::GetContentRegionAvail().x;

	ImVec4 cyanColor = ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f); // #00ADB5
	ImVec4 grayText = ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f); // #BBC9CA
	ImVec4 whiteText = ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f); // #E2E2E2
	ImVec4 borderColor = ImVec4(60.0f / 255.0f, 73.0f / 255.0f, 74.0f / 255.0f, 1.0f); // #3C494A

	const char* displayName = (projectName && projectName[0] != '\0') ? projectName : "Project";
	const char* displayDate = (createdDate && createdDate[0] != '\0') ? createdDate : "—";

	// --- Header Section ---
	ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
	ImGui::SetWindowFontScale(1.1f);
	ImGui::Text("%s", displayName);
	ImGui::SetWindowFontScale(1.0f);
	ImGui::PopStyleColor();

	ImGui::SameLine(0.0f, 24.0f);

	// Project created date (from database)
	ImGui::PushStyleColor(ImGuiCol_Text, grayText);
	ImGui::Text("%s %s", ICON_FA_CALENDAR, displayDate);
	ImGui::PopStyleColor();

	// Right aligned button
	float btnWidth = 140.0f;
	ImGui::SameLine(totalWidth - btnWidth);
	
	// Solid Cyan button (+ New Milestone)
	ImGui::PushStyleColor(ImGuiCol_Button, cyanColor);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 193.0f/255.0f, 201.0f/255.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 153.0f/255.0f, 161.0f/255.0f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(18.0f/255.0f, 20.0f/255.0f, 20.0f/255.0f, 1.0f)); // Dark text
	if (ImGui::Button(ICON_FA_PLUS " NEW MILESTONE", ImVec2(btnWidth, 32.0f))) {}
	ImGui::PopStyleColor(4);
	ImGui::PopStyleVar();

	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	// Separator Line
	ImVec2 cursor = ImGui::GetCursorScreenPos();
	drawList->AddLine(cursor, ImVec2(cursor.x + totalWidth, cursor.y), ImGui::GetColorU32(borderColor), 1.0f);
	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	// --- Active Milestones Title ---
	ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
	ImGui::Text(ICON_FA_FLAG " Active Milestones");
	ImGui::PopStyleColor();
	
	ImGui::SameLine(totalWidth - 60.0f);
	ImGui::PushStyleColor(ImGuiCol_Text, grayText);
	ImGui::Text("Total: 02");
	ImGui::PopStyleColor();

	auto DrawRowSeparator = [&]() {
		ImGui::Dummy(ImVec2(0.0f, 8.0f));
		ImVec2 p = ImGui::GetCursorScreenPos();
		drawList->AddLine(p, ImVec2(p.x + totalWidth, p.y), ImGui::GetColorU32(ImVec4(40.0f/255.0f, 43.0f/255.0f, 43.0f/255.0f, 1.0f)), 1.0f);
		ImGui::Dummy(ImVec2(0.0f, 8.0f));
	};

	DrawRowSeparator();

	// Helper for Outline Badge
	auto DrawOutlineBadge = [&](const char* label, ImVec4 outlineColor, ImVec4 textColor, float width) {
		ImVec2 pos = ImGui::GetCursorScreenPos();
		pos.y -= 2.0f;
		drawList->AddRect(pos, ImVec2(pos.x + width, pos.y + 20.0f), ImGui::GetColorU32(outlineColor), 4.0f, 0, 1.0f);
		
		ImVec2 textSize = ImGui::CalcTextSize(label);
		drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 0.8f, ImVec2(pos.x + (width - textSize.x * 0.8f) * 0.5f, pos.y + 3.0f), ImGui::GetColorU32(textColor), label);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + width + 10.0f);
	};

	// --- Milestone 1: Authentication Logic ---
	ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
	ImGui::Text("Authentication Logic");
	ImGui::PopStyleColor();
	
	ImGui::SameLine(totalWidth - 80.0f);
	DrawOutlineBadge("IN PROGRESS", cyanColor, cyanColor, 80.0f);
	
	ImGui::Dummy(ImVec2(0.0f, 5.0f));
	
	// Progress Bar
	ImVec2 pbPos = ImGui::GetCursorScreenPos();
	float pbHeight = 4.0f;
	drawList->AddRectFilled(pbPos, ImVec2(pbPos.x + totalWidth, pbPos.y + pbHeight), ImGui::GetColorU32(ImVec4(40.0f/255.0f, 43.0f/255.0f, 43.0f/255.0f, 1.0f))); // Background
	drawList->AddRectFilled(pbPos, ImVec2(pbPos.x + totalWidth * 0.5f, pbPos.y + pbHeight), ImGui::GetColorU32(cyanColor)); // Fill 50%
	
	ImGui::Dummy(ImVec2(0.0f, 8.0f));
	
	// Subtasks text
	ImGui::PushStyleColor(ImGuiCol_Text, grayText);
	ImGui::Text("Subtasks: 1/2");
	ImGui::SameLine(totalWidth - 30.0f);
	ImGui::Text("50%%");
	ImGui::PopStyleColor();

	DrawRowSeparator();

	// --- Milestone 2: UI System ---
	ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
	ImGui::Text("UI System");
	ImGui::PopStyleColor();
	
	ImGui::SameLine(totalWidth - 80.0f);
	DrawOutlineBadge("NOT STARTED", grayText, grayText, 80.0f);
	
	ImGui::Dummy(ImVec2(0.0f, 5.0f));
	
	// Progress Bar
	pbPos = ImGui::GetCursorScreenPos();
	drawList->AddRectFilled(pbPos, ImVec2(pbPos.x + totalWidth, pbPos.y + pbHeight), ImGui::GetColorU32(ImVec4(40.0f/255.0f, 43.0f/255.0f, 43.0f/255.0f, 1.0f))); // Background (0% fill)
	
	ImGui::Dummy(ImVec2(0.0f, 8.0f));
	
	// Subtasks text
	ImGui::PushStyleColor(ImGuiCol_Text, grayText);
	ImGui::Text("Subtasks: 0/1");
	ImGui::SameLine(totalWidth - 20.0f);
	ImGui::Text("0%%");
	ImGui::PopStyleColor();

	DrawRowSeparator();
}
