#include <glad/gl.h>

#include "imgui.h"
#include "IconsFontAwesome6.h"

#include "AppLayoutLayer.h"

void AppLayoutLayer::OnRender()
{
	glClearColor(18.0f / 255.0f, 20.0f / 255.0f, 20.0f / 255.0f, 1.0f); // #121414
	glClear(GL_COLOR_BUFFER_BIT);

	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	ImGui::SetNextWindowPos(viewport->WorkPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(viewport->WorkSize, ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.0f); // Transparent main window

	ImGuiWindowFlags mainFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	
	if (ImGui::Begin("AppLayoutContainer", nullptr, mainFlags))
	{
		RenderSideNavBar();
		
		ImGui::SameLine(0.0f, 0.0f);

		ImGui::BeginGroup();
		RenderTopNavBar();
		
		// Content Area
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(18.0f / 255.0f, 20.0f / 255.0f, 20.0f / 255.0f, 1.0f)); // #121414
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(30.0f, 30.0f));
		if (ImGui::BeginChild("ContentArea", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding))
		{
			OnRenderContent();
		}
		ImGui::EndChild();
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
		
		ImGui::EndGroup();
	}
	ImGui::End();

	ImGui::PopStyleVar();
}

void AppLayoutLayer::RenderSideNavBar()
{
	float sidebarWidth = 240.0f;
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(26.0f / 255.0f, 28.0f / 255.0f, 28.0f / 255.0f, 1.0f)); // #1A1C1C
	
	if (ImGui::BeginChild("SideNavBar", ImVec2(sidebarWidth, 0), false, ImGuiWindowFlags_NoScrollbar))
	{
		ImVec2 childSize = ImGui::GetWindowSize();
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 pMin = ImGui::GetWindowPos();
		ImVec2 pMax = ImVec2(pMin.x + childSize.x, pMin.y + childSize.y);
		
		// Draw right border
		drawList->AddLine(ImVec2(pMax.x, pMin.y), ImVec2(pMax.x, pMax.y), IM_COL32(60, 73, 74, 255), 1.0f); // #3C494A
		
		ImGui::Dummy(ImVec2(0.0f, 30.0f)); // Top padding
		
		ImGui::SetCursorPosX(30.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f)); // #E2E2E2
		ImGui::SetWindowFontScale(1.1f);
		ImGui::Text("TRACKING TOOL");
		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopStyleColor();

		ImGui::SetCursorPosX(30.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f)); // #00ADB5
		ImGui::SetWindowFontScale(0.7f);
		ImGui::Text("TECHNICAL OPERATIONS");
		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 40.0f));

		// Menu Items
		float menuItemHeight = 40.0f;
		
		// 1. Dashboard (Active)
		ImVec2 cursorPos = ImGui::GetCursorPos();
		ImVec2 itemPMin = ImVec2(pMin.x, pMin.y + cursorPos.y);
		ImVec2 itemPMax = ImVec2(pMin.x + sidebarWidth, itemPMin.y + menuItemHeight);
		
		// Draw active background
		drawList->AddRectFilled(itemPMin, itemPMax, IM_COL32(18, 20, 20, 255)); // #121414
		// Draw active left border accent
		drawList->AddRectFilled(itemPMin, ImVec2(itemPMin.x + 4.0f, itemPMax.y), IM_COL32(0, 173, 181, 255)); // #00ADB5
		
		ImGui::SetCursorPos(ImVec2(30.0f, cursorPos.y + (menuItemHeight - ImGui::GetTextLineHeight()) * 0.5f));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f));
		ImGui::Text(ICON_FA_TABLE_CELLS_LARGE "   Dashboard");
		ImGui::PopStyleColor();

		ImGui::SetCursorPosY(cursorPos.y + menuItemHeight);

		// 2. Project
		cursorPos = ImGui::GetCursorPos();
		ImGui::SetCursorPos(ImVec2(30.0f, cursorPos.y + (menuItemHeight - ImGui::GetTextLineHeight()) * 0.5f));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f)); // #BBC9CA
		ImGui::Text(ICON_FA_FOLDER "   Project");
		ImGui::PopStyleColor();

		ImGui::SetCursorPosY(cursorPos.y + menuItemHeight);

		// Help at bottom
		ImGui::SetCursorPosY(childSize.y - 40.0f - ImGui::GetTextLineHeight());
		ImGui::SetCursorPosX(30.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f));
		ImGui::Text(ICON_FA_CIRCLE_QUESTION "   Help");
		ImGui::PopStyleColor();
	}
	ImGui::EndChild();
	
	ImGui::PopStyleColor();
}

void AppLayoutLayer::RenderTopNavBar()
{
	float topbarHeight = 70.0f;
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(26.0f / 255.0f, 28.0f / 255.0f, 28.0f / 255.0f, 1.0f)); // #1A1C1C
	
	if (ImGui::BeginChild("TopNavBar", ImVec2(0, topbarHeight), false, ImGuiWindowFlags_NoScrollbar))
	{
		ImVec2 childSize = ImGui::GetWindowSize();
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 pMin = ImGui::GetWindowPos();
		ImVec2 pMax = ImVec2(pMin.x + childSize.x, pMin.y + childSize.y);
		
		// Draw bottom border
		drawList->AddLine(ImVec2(pMin.x, pMax.y), ImVec2(pMax.x, pMax.y), IM_COL32(60, 73, 74, 255), 1.0f); // #3C494A

		float centerY = (topbarHeight - ImGui::GetTextLineHeight()) * 0.5f;

		// Title
		ImGui::SetCursorPos(ImVec2(30.0f, centerY));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f)); // #00ADB5
		ImGui::SetWindowFontScale(1.2f);
		ImGui::Text("Personal Dashboard");
		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopStyleColor();

		// Right side items
		const char* nameText = "Alex Rivera";
		float rightPadding = 30.0f;
		float nameWidth = ImGui::CalcTextSize(nameText).x;
		
		ImGui::SetCursorPos(ImVec2(childSize.x - rightPadding - nameWidth, centerY));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f)); // #E2E2E2
		ImGui::Text("%s", nameText);
		ImGui::PopStyleColor();
		
		// Gear Icon
		float gearWidth = ImGui::CalcTextSize(ICON_FA_GEAR).x;
		ImGui::SetCursorPos(ImVec2(childSize.x - rightPadding - nameWidth - gearWidth - 20.0f, centerY));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f)); // #BBC9CA
		ImGui::Text(ICON_FA_GEAR);
		ImGui::PopStyleColor();
		
		// Bell Icon
		float bellWidth = ImGui::CalcTextSize(ICON_FA_BELL).x;
		ImGui::SetCursorPos(ImVec2(childSize.x - rightPadding - nameWidth - gearWidth - 20.0f - bellWidth - 20.0f, centerY));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f));
		ImGui::Text(ICON_FA_BELL);
		ImGui::PopStyleColor();

		// Allow extensions
		OnRenderTopNavBarExtensions();
	}
	ImGui::EndChild();
	
	ImGui::PopStyleColor();
}