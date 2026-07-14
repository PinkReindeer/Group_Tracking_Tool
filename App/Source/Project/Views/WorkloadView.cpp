#include "imgui.h"

#include "WorkloadView.h"
#include "imgui.h"
#include "IconsFontAwesome6.h"

void WorkloadView::OnRender(const char* projectName, const char* createdDate)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	float totalWidth = ImGui::GetContentRegionAvail().x;
	ImVec2 pMin = ImGui::GetCursorScreenPos();

	ImVec4 cyanColor = ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f); // #00ADB5
	ImVec4 grayText = ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f); // #BBC9CA
	ImVec4 whiteText = ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f); // #E2E2E2
	ImVec4 borderColor = ImVec4(60.0f / 255.0f, 73.0f / 255.0f, 74.0f / 255.0f, 1.0f); // #3C494A
	ImVec4 darkBg = ImVec4(26.0f / 255.0f, 28.0f / 255.0f, 28.0f / 255.0f, 1.0f); // #1A1C1C
	ImVec4 cardBorderColor = ImVec4(56.0f / 255.0f, 57.0f / 255.0f, 58.0f / 255.0f, 1.0f); // #38393A

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

	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	// Separator Line
	ImVec2 cursor = ImGui::GetCursorScreenPos();
	drawList->AddLine(cursor, ImVec2(cursor.x + totalWidth, cursor.y), ImGui::GetColorU32(borderColor), 1.0f);
	ImGui::Dummy(ImVec2(0.0f, 20.0f));

	// --- Summary Cards ---
	float cardWidth = (totalWidth - 40.0f) / 3.0f; // 3 cards, 20px gaps
	float cardHeight = 100.0f;

	auto DrawCard = [&](const char* title, const char* value, ImVec4 accentColor, ImVec4 borderCol) {
		ImVec2 pos = ImGui::GetCursorScreenPos();
		drawList->AddRectFilled(pos, ImVec2(pos.x + cardWidth, pos.y + cardHeight), ImGui::GetColorU32(darkBg));
		drawList->AddRect(pos, ImVec2(pos.x + cardWidth, pos.y + cardHeight), ImGui::GetColorU32(borderCol), 0.0f, 0, 2.0f);
		
		// Accent text
		drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 0.9f, ImVec2(pos.x + 20.0f, pos.y + 20.0f), ImGui::GetColorU32(accentColor), title);
		
		// Value text
		ImGui::SetWindowFontScale(2.5f);
		drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(pos.x + 20.0f, pos.y + 45.0f), ImGui::GetColorU32(whiteText), value);
		ImGui::SetWindowFontScale(1.0f);

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + cardWidth + 20.0f);
	};

	DrawCard("PENDING TASKS", "2", grayText, cardBorderColor);
	DrawCard("IN PROGRESS", "0", cyanColor, cyanColor); // Cyan border for IN PROGRESS
	DrawCard("COMPLETED", "1", cyanColor, cardBorderColor); // Wait, design has cyan text for completed title

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + cardHeight + 40.0f);

	// --- Member Row ---
	// Member Info
	ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
	ImGui::Text("Alex Rivera");
	ImGui::PopStyleColor();

	ImGui::PushStyleColor(ImGuiCol_Text, grayText);
	ImGui::SetWindowFontScale(0.8f);
	ImGui::Text("Leader");
	ImGui::SetWindowFontScale(1.0f);
	ImGui::PopStyleColor();

	// Right aligned BALANCED badge
	ImVec2 badgePos = ImVec2(ImGui::GetCursorScreenPos().x + totalWidth - 80.0f, ImGui::GetCursorScreenPos().y - 30.0f);
	ImVec4 greenBadge = ImVec4(0.0f, 200.0f/255.0f, 100.0f/255.0f, 1.0f);
	drawList->AddRect(badgePos, ImVec2(badgePos.x + 80.0f, badgePos.y + 24.0f), ImGui::GetColorU32(greenBadge), 4.0f, 0, 1.0f);
	drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 0.8f, ImVec2(badgePos.x + 15.0f, badgePos.y + 5.0f), ImGui::GetColorU32(greenBadge), "BALANCED");

	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	// Active Tasks label
	ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
	ImGui::SetWindowFontScale(0.9f);
	ImGui::Text("Active Tasks");
	ImGui::PopStyleColor();

	ImGui::SameLine(totalWidth - 20.0f);
	ImGui::PushStyleColor(ImGuiCol_Text, grayText);
	ImGui::Text("2");
	ImGui::PopStyleColor();
	ImGui::SetWindowFontScale(1.0f);

	ImGui::Dummy(ImVec2(0.0f, 5.0f));

	// Progress Bar (Cyan)
	ImVec2 pbPos = ImGui::GetCursorScreenPos();
	float pbHeight = 4.0f;
	drawList->AddRectFilled(pbPos, ImVec2(pbPos.x + totalWidth, pbPos.y + pbHeight), ImGui::GetColorU32(ImVec4(40.0f/255.0f, 43.0f/255.0f, 43.0f/255.0f, 1.0f))); // Background
	drawList->AddRectFilled(pbPos, ImVec2(pbPos.x + totalWidth, pbPos.y + pbHeight), ImGui::GetColorU32(cyanColor)); // Filled
	
	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	// Breakdown Text
	ImGui::PushStyleColor(ImGuiCol_Text, grayText);
	ImGui::SetWindowFontScale(0.9f);
	ImGui::Text("Pending: 2  In Progress: 0  Done: 0");
	ImGui::SetWindowFontScale(1.0f);
	ImGui::PopStyleColor();
}
