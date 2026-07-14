#include "TasksView.h"
#include "imgui.h"
#include "IconsFontAwesome6.h"

void TasksView::OnRender(const char* projectName, const char* createdDate)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	float totalWidth = ImGui::GetContentRegionAvail().x;

	ImVec4 cyanColor = ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f); // #00ADB5
	ImVec4 grayText = ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f); // #BBC9CA
	ImVec4 whiteText = ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f); // #E2E2E2
	ImVec4 borderColor = ImVec4(60.0f / 255.0f, 73.0f / 255.0f, 74.0f / 255.0f, 1.0f); // #3C494A

	const char* displayName = (projectName && projectName[0] != '\0') ? projectName : "Project";
	const char* displayDate = (createdDate && createdDate[0] != '\0') ? createdDate : "—";

	// Header Section
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

	// Right aligned buttons
	float btnWidth = 120.0f;
	float filterBtnWidth = 90.0f;
	float btnSpacing = 10.0f;
	ImGui::SameLine(totalWidth - btnWidth - filterBtnWidth - btnSpacing);
	
	// Outline button (Filters)
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
	ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
	if (ImGui::Button(ICON_FA_FILTER " Filters", ImVec2(filterBtnWidth, 32.0f))) {}
	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar(2);
	
	ImGui::SameLine(0, btnSpacing);

	// Solid Cyan button (+ New Task)
	ImGui::PushStyleColor(ImGuiCol_Button, cyanColor);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 193.0f/255.0f, 201.0f/255.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 153.0f/255.0f, 161.0f/255.0f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(18.0f/255.0f, 20.0f/255.0f, 20.0f/255.0f, 1.0f)); // Dark text
	if (ImGui::Button(ICON_FA_PLUS " New Task", ImVec2(btnWidth, 32.0f))) {}
	ImGui::PopStyleColor(4);
	ImGui::PopStyleVar();

	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	// Separator Line
	ImVec2 cursor = ImGui::GetCursorScreenPos();
	drawList->AddLine(cursor, ImVec2(cursor.x + totalWidth, cursor.y), ImGui::GetColorU32(borderColor), 1.0f);
	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	// Helper for badges
	auto DrawBadge = [&](const char* label, ImVec4 bgColor, ImVec4 textColor) {
		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImVec2 size = ImVec2(65.0f, 20.0f);
		pos.y -= 2.0f;
		drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImGui::GetColorU32(bgColor), 4.0f);
		drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 0.8f, ImVec2(pos.x + 8.0f, pos.y + 3.0f), ImGui::GetColorU32(textColor), label);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + size.x + 10.0f);
	};

	// Helper for drawing rows
	auto DrawRowSeparator = [&]() {
		ImGui::Dummy(ImVec2(0.0f, 8.0f));
		ImVec2 p = ImGui::GetCursorScreenPos();
		drawList->AddLine(p, ImVec2(p.x + totalWidth, p.y), ImGui::GetColorU32(ImVec4(40.0f/255.0f, 43.0f/255.0f, 43.0f/255.0f, 1.0f)), 1.0f);
		ImGui::Dummy(ImVec2(0.0f, 8.0f));
	};

	auto DrawActions = [&]() {
		ImGui::SameLine(totalWidth - 60.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::Text(ICON_FA_PEN_TO_SQUARE "  " ICON_FA_TRASH);
		ImGui::PopStyleColor();
	};

	// Group 1: Authentication Logic
	ImGui::PushStyleColor(ImGuiCol_Text, cyanColor);
	ImGui::Text(ICON_FA_CHEVRON_DOWN " Authentication Logic");
	ImGui::PopStyleColor();
	DrawRowSeparator();

	// Task 1
	ImGui::SetCursorPosX(30.0f); // indent
	ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
	ImGui::Text("Login");
	ImGui::PopStyleColor();
	
	ImGui::SameLine(180.0f);
	DrawBadge("Pending", ImVec4(60.0f/255.0f, 65.0f/255.0f, 65.0f/255.0f, 1.0f), grayText);
	DrawActions();
	DrawRowSeparator();

	// Task 2
	ImGui::SetCursorPosX(30.0f); // indent
	ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
	ImGui::Text("Register");
	ImGui::PopStyleColor();
	
	ImGui::SameLine(180.0f);
	DrawBadge("Completed", ImVec4(0.0f, 100.0f/255.0f, 50.0f/255.0f, 0.5f), ImVec4(0.0f, 255.0f/255.0f, 100.0f/255.0f, 1.0f));
	DrawActions();
	DrawRowSeparator();

	// Group 2: UI System
	ImGui::PushStyleColor(ImGuiCol_Text, cyanColor);
	ImGui::Text(ICON_FA_CHEVRON_DOWN " UI System");
	ImGui::PopStyleColor();
	DrawRowSeparator();

	// Task 3
	ImGui::SetCursorPosX(30.0f); // indent
	ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
	ImGui::Text("Personal Dashboard");
	ImGui::PopStyleColor();
	
	ImGui::SameLine(180.0f);
	DrawBadge("Pending", ImVec4(60.0f/255.0f, 65.0f/255.0f, 65.0f/255.0f, 1.0f), grayText);
	DrawActions();
	DrawRowSeparator();
}
