#include "imgui.h"
#include "IconsFontAwesome6.h"

#include "DashboardLayer.h"

void DashboardLayer::OnUpdate(float ts)
{
}

void DashboardLayer::OnRenderContent()
{
	// Setup Colors
	ImVec4 cyanColor = ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f); // #00ADB5
	ImVec4 grayText = ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f); // #BBC9CA
	ImVec4 whiteText = ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f); // #E2E2E2
	ImVec4 blueColor = ImVec4(60.0f / 255.0f, 176.0f / 255.0f, 255.0f / 255.0f, 1.0f); // #3CB0FF
	ImVec4 redColor = ImVec4(238.0f / 255.0f, 56.0f / 255.0f, 56.0f / 255.0f, 1.0f); // #EE3838
	ImVec4 boxBgColor = ImVec4(26.0f / 255.0f, 28.0f / 255.0f, 28.0f / 255.0f, 1.0f); // #1A1C1C
	ImVec4 borderColor = ImVec4(60.0f / 255.0f, 73.0f / 255.0f, 74.0f / 255.0f, 1.0f); // #3C494A

	// Draw list for custom shapes
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	// Quick Stats Bento Header
	float totalWidth = ImGui::GetContentRegionAvail().x;
	float gap = 20.0f;
	float boxWidth = (totalWidth - (gap * 3.0f)) / 4.0f;
	float boxHeight = 100.0f;

	ImVec2 cursorStart = ImGui::GetCursorScreenPos();
	
	// Helper lambda for stat boxes
	auto DrawStatBox = [&](int index, const char* title, const char* value, ImVec4 titleColor, ImVec4 valueColor, ImVec4 accentColor) {
		ImVec2 pMin = ImVec2(cursorStart.x + index * (boxWidth + gap), cursorStart.y);
		ImVec2 pMax = ImVec2(pMin.x + boxWidth, pMin.y + boxHeight);

		// Box BG
		drawList->AddRectFilled(pMin, pMax, ImGui::GetColorU32(boxBgColor), 4.0f);
		// Box Border
		drawList->AddRect(pMin, pMax, ImGui::GetColorU32(ImVec4(51.0f/255.0f, 53.0f/255.0f, 53.0f/255.0f, 1.0f)), 4.0f); // Subtle border
		// Left Accent Line
		drawList->AddRectFilled(pMin, ImVec2(pMin.x + 4.0f, pMax.y), ImGui::GetColorU32(accentColor), 4.0f, ImDrawFlags_RoundCornersLeft);

		// Content
		ImVec2 textPos = ImVec2(pMin.x + 20.0f, pMin.y + 20.0f);
		drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 0.8f, textPos, ImGui::GetColorU32(titleColor), title);
		
		textPos.y += 25.0f;
		drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 2.5f, textPos, ImGui::GetColorU32(valueColor), value);
	};

	DrawStatBox(0, "PROJECT", "01", cyanColor, cyanColor, cyanColor);
	DrawStatBox(1, "PENDING TASKS", "0", grayText, whiteText, ImVec4(80.0f/255.0f, 80.0f/255.0f, 80.0f/255.0f, 1.0f));
	DrawStatBox(2, "IN PROGRESS", "0", blueColor, blueColor, blueColor);
	DrawStatBox(3, "OVERDUE", "0", redColor, redColor, redColor);

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + boxHeight + 30.0f);

	// Project Header Section
	ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
	ImGui::SetWindowFontScale(1.2f);
	ImGui::Text("Your Project");
	ImGui::SetWindowFontScale(1.0f);
	ImGui::PopStyleColor();

	ImGui::SameLine();
	
	// Live Refresh Badge
	ImVec2 badgePos = ImGui::GetCursorScreenPos();
	badgePos.y += 2.0f; // vertical tweak
	ImVec2 badgeSize = ImVec2(90.0f, 20.0f);
	drawList->AddRectFilled(badgePos, ImVec2(badgePos.x + badgeSize.x, badgePos.y + badgeSize.y), IM_COL32(51, 53, 53, 255), 4.0f);
	drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 0.7f, ImVec2(badgePos.x + 8.0f, badgePos.y + 3.0f), ImGui::GetColorU32(grayText), "LIVE REFRESH");
	
	// Advance cursor past the badge manually
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + badgeSize.x + 10.0f);

	// Right aligned buttons
	float btnWidth = 140.0f;
	float btnSpacing = 10.0f;
	ImGui::SameLine(totalWidth - (btnWidth * 2.0f) - btnSpacing);
	
	// Outline button (Join Project)
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
	ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
	if (ImGui::Button(ICON_FA_RIGHT_TO_BRACKET " JOIN PROJECT", ImVec2(btnWidth, 32.0f))) {}
	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar(2);
	
	ImGui::SameLine(0, btnSpacing);

	// Solid Cyan button (+ New Project)
	ImGui::PushStyleColor(ImGuiCol_Button, cyanColor);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 193.0f/255.0f, 201.0f/255.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 153.0f/255.0f, 161.0f/255.0f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(18.0f/255.0f, 20.0f/255.0f, 20.0f/255.0f, 1.0f)); // Dark text
	if (ImGui::Button(ICON_FA_PLUS " NEW PROJECT", ImVec2(btnWidth, 32.0f))) {}
	ImGui::PopStyleColor(4);
	ImGui::PopStyleVar();

	ImGui::Dummy(ImVec2(0.0f, 20.0f));

	// Project Grid
	ImGui::PushStyleColor(ImGuiCol_ChildBg, boxBgColor);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
	ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

	if (ImGui::BeginChild("Project1", ImVec2(300.0f, 150.0f), true, ImGuiWindowFlags_NoScrollbar))
	{
		ImGui::SetCursorPos(ImVec2(20.0f, 20.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::Text("Group Tracking Tool");
		ImGui::PopStyleColor();

		ImGui::SameLine();
		
		// Leader Badge
		ImVec2 leaderBadgePos = ImGui::GetCursorScreenPos();
		leaderBadgePos.y -= 2.0f;
		ImVec2 leaderBadgeSize = ImVec2(60.0f, 18.0f);
		ImGui::GetWindowDrawList()->AddRectFilled(leaderBadgePos, ImVec2(leaderBadgePos.x + leaderBadgeSize.x, leaderBadgePos.y + leaderBadgeSize.y), ImGui::GetColorU32(ImVec4(0.0f, 173.0f/255.0f, 181.0f/255.0f, 0.2f)), 9.0f);
		ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 0.7f, ImVec2(leaderBadgePos.x + 8.0f, leaderBadgePos.y + 2.0f), ImGui::GetColorU32(cyanColor), ICON_FA_USER " Leader");

		// 3-dots icon
		ImGui::SetCursorPos(ImVec2(300.0f - 30.0f, 20.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::Text(ICON_FA_ELLIPSIS_VERTICAL);
		ImGui::PopStyleColor();

		ImGui::SetCursorPos(ImVec2(20.0f, 60.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::PushTextWrapPos(280.0f);
		ImGui::Text("A desktop application for group tracking, built with C++20, OpenGL, and Dear ImGui");
		ImGui::PopTextWrapPos();
		ImGui::PopStyleColor();
	}
	ImGui::EndChild();

	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(2);
}
