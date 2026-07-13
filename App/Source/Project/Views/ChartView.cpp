#include "ChartView.h"
#include "imgui.h"
#include "IconsFontAwesome6.h"

void ChartView::OnRender()
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	float totalWidth = ImGui::GetContentRegionAvail().x;
	ImVec2 pMin = ImGui::GetCursorScreenPos();

	ImVec4 cyanColor = ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f); // #00ADB5
	ImVec4 grayText = ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f); // #BBC9CA
	ImVec4 whiteText = ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f); // #E2E2E2
	ImVec4 borderColor = ImVec4(60.0f / 255.0f, 73.0f / 255.0f, 74.0f / 255.0f, 1.0f); // #3C494A
	ImVec4 greenBar = ImVec4(0.0f, 255.0f/255.0f, 100.0f/255.0f, 0.4f);
	ImVec4 greenText = ImVec4(0.0f, 255.0f/255.0f, 100.0f/255.0f, 1.0f);

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

	// --- Gantt Chart Area ---
	float sidebarWidth = 240.0f; // Width for Task Hierarchy
	float dayWidth = (totalWidth - sidebarWidth) / 5.0f; // 5 columns
	float rowHeight = 40.0f;

	// Table Header
	ImGui::PushStyleColor(ImGuiCol_Text, grayText);
	ImGui::SetCursorPos(ImVec2(30.0f, ImGui::GetCursorPosY()));
	ImGui::Text("TASK NOMENCLATURE / HIERARCHY");
	
	const char* days[] = {"MON\n12 OCT", "TUE\n13 OCT", "WED\n14 OCT", "THU\n15 OCT", "FRI\n16 OCT"};
	for (int i = 0; i < 5; i++)
	{
		ImGui::SameLine(sidebarWidth + i * dayWidth + dayWidth * 0.5f - 20.0f);
		if (i == 1) // Today
			ImGui::PushStyleColor(ImGuiCol_Text, cyanColor);
		ImGui::Text("%s", days[i]);
		if (i == 1)
			ImGui::PopStyleColor();
	}
	ImGui::PopStyleColor();

	ImGui::Dummy(ImVec2(0.0f, 15.0f));
	
	// Draw horizontal line
	cursor = ImGui::GetCursorScreenPos();
	drawList->AddLine(cursor, ImVec2(cursor.x + totalWidth, cursor.y), ImGui::GetColorU32(borderColor), 1.0f);

	// Draw vertical column lines
	for (int i = 0; i <= 5; i++)
	{
		float vx = cursor.x + sidebarWidth + i * dayWidth;
		drawList->AddLine(ImVec2(vx, cursor.y), ImVec2(vx, cursor.y + rowHeight * 3.0f), ImGui::GetColorU32(borderColor), 1.0f);
	}

	// Draw "Today" vertical cyan line (middle of TUE 13 OCT)
	float todayX = cursor.x + sidebarWidth + 1.5f * dayWidth;
	drawList->AddLine(ImVec2(todayX, cursor.y), ImVec2(todayX, cursor.y + rowHeight * 3.0f), ImGui::GetColorU32(cyanColor), 2.0f);
	drawList->AddCircleFilled(ImVec2(todayX, cursor.y), 4.0f, ImGui::GetColorU32(cyanColor));

	// Row 1: Authentication Logic
	ImGui::Dummy(ImVec2(0.0f, rowHeight * 0.2f));
	ImGui::SetCursorPosX(30.0f);
	ImGui::PushStyleColor(ImGuiCol_Text, cyanColor);
	ImGui::Text(ICON_FA_CHEVRON_DOWN " Authentication Logic");
	ImGui::PopStyleColor();
	ImGui::Dummy(ImVec2(0.0f, rowHeight * 0.2f));

	cursor = ImGui::GetCursorScreenPos();
	drawList->AddLine(cursor, ImVec2(cursor.x + totalWidth, cursor.y), ImGui::GetColorU32(borderColor), 1.0f);

	// Row 2: Login
	ImGui::Dummy(ImVec2(0.0f, rowHeight * 0.2f));
	ImGui::SetCursorPosX(50.0f);
	ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
	ImGui::Text("Login");
	ImGui::PopStyleColor();
	ImGui::Dummy(ImVec2(0.0f, rowHeight * 0.2f));

	cursor = ImGui::GetCursorScreenPos();
	drawList->AddLine(cursor, ImVec2(cursor.x + totalWidth, cursor.y), ImGui::GetColorU32(borderColor), 1.0f);

	// Row 3: Register
	ImVec2 row3Start = ImGui::GetCursorScreenPos();
	ImGui::Dummy(ImVec2(0.0f, rowHeight * 0.2f));
	ImGui::SetCursorPosX(50.0f);
	ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
	ImGui::Text("Register");
	ImGui::PopStyleColor();
	ImGui::Dummy(ImVec2(0.0f, rowHeight * 0.2f));

	// Draw Gantt Bar for Register (From Mon to middle of Wed)
	float barStartX = row3Start.x + sidebarWidth + 0.1f * dayWidth;
	float barEndX = row3Start.x + sidebarWidth + 2.5f * dayWidth;
	drawList->AddRectFilled(ImVec2(barStartX, row3Start.y + 4.0f), ImVec2(barEndX, row3Start.y + rowHeight - 4.0f), ImGui::GetColorU32(greenBar), 4.0f);
	drawList->AddText(ImVec2(barStartX + 10.0f, row3Start.y + 10.0f), ImGui::GetColorU32(greenText), "Register Logic Completed");
	drawList->AddText(ImVec2(barEndX - 40.0f, row3Start.y + 10.0f), ImGui::GetColorU32(greenText), "100%");
	// Draw diamond endpoint
	drawList->AddQuadFilled(
		ImVec2(barEndX + 4.0f, row3Start.y + rowHeight * 0.5f),
		ImVec2(barEndX - 2.0f, row3Start.y + rowHeight * 0.5f + 6.0f),
		ImVec2(barEndX - 8.0f, row3Start.y + rowHeight * 0.5f),
		ImVec2(barEndX - 2.0f, row3Start.y + rowHeight * 0.5f - 6.0f),
		ImGui::GetColorU32(greenText));

	cursor = ImGui::GetCursorScreenPos();
	drawList->AddLine(cursor, ImVec2(cursor.x + totalWidth, cursor.y), ImGui::GetColorU32(borderColor), 1.0f);

	// Row 4: UI System
	ImGui::Dummy(ImVec2(0.0f, rowHeight * 0.2f));
	ImGui::SetCursorPosX(30.0f);
	ImGui::PushStyleColor(ImGuiCol_Text, cyanColor);
	ImGui::Text(ICON_FA_CHEVRON_DOWN " UI System");
	ImGui::PopStyleColor();
	ImGui::Dummy(ImVec2(0.0f, rowHeight * 0.2f));

	cursor = ImGui::GetCursorScreenPos();
	drawList->AddLine(cursor, ImVec2(cursor.x + totalWidth, cursor.y), ImGui::GetColorU32(borderColor), 1.0f);

	// Row 5: Personal Dashboard
	ImGui::Dummy(ImVec2(0.0f, rowHeight * 0.2f));
	ImGui::SetCursorPosX(50.0f);
	ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
	ImGui::Text("Personal Dashboard");
	ImGui::PopStyleColor();
	ImGui::Dummy(ImVec2(0.0f, rowHeight * 0.2f));

	cursor = ImGui::GetCursorScreenPos();
	drawList->AddLine(cursor, ImVec2(cursor.x + totalWidth, cursor.y), ImGui::GetColorU32(borderColor), 1.0f);
}
