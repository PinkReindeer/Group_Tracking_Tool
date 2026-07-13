#include <glad/gl.h>

#include "imgui.h"
#include "IconsFontAwesome6.h"

#include "AppLayoutLayer.h"
#include "Service/AuthService.h"

#include "Authentication/AuthenticationLayer.h"
#include "Dashboard/DashboardLayer.h"
#include "Project/ProjectLayer.h"

void AppLayoutLayer::OnRender()
{
	glClearColor(18.0f / 255.0f, 20.0f / 255.0f, 20.0f / 255.0f, 1.0f); // #121414
	glClear(GL_COLOR_BUFFER_BIT);

	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	ImGui::SetNextWindowPos(viewport->WorkPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(viewport->WorkSize, ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.0f); // Transparent main window

	ImGuiWindowFlags mainFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

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

		// Modal must live on the root layout window so its ID stack is stable
		// across gear-menu popup close / top-bar child teardown.
		RenderLogoutConfirmModal();
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
		drawList->AddLine(ImVec2(pMax.x, pMin.y), ImVec2(pMax.x, pMax.y), IM_COL32(60, 73, 74, 255), 5.0f); // #3C494A
		
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
		const char* activeMenu = GetActiveSidebarMenu();
		
		// 1. Dashboard
		ImVec2 cursorPos = ImGui::GetCursorPos();
		ImVec2 itemPMin = ImVec2(pMin.x, pMin.y + cursorPos.y);
		ImVec2 itemPMax = ImVec2(pMin.x + sidebarWidth - 2.0f, itemPMin.y + menuItemHeight);
		
		bool isDashboardActive = (strcmp(activeMenu, "Dashboard") == 0);
		if (isDashboardActive)
		{
			drawList->AddRectFilled(itemPMin, itemPMax, IM_COL32(18, 20, 20, 255)); // #121414
			drawList->AddRectFilled(itemPMin, ImVec2(itemPMin.x + 4.0f, itemPMax.y), IM_COL32(0, 173, 181, 255)); // #00ADB5
		}
		
		ImGui::SetCursorPos(ImVec2(30.0f, cursorPos.y + (menuItemHeight - ImGui::GetTextLineHeight()) * 0.5f));
		if (isDashboardActive)
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f));
		else
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f));
			
		ImGui::Text(ICON_FA_TABLE_CELLS_LARGE "   Dashboard");
		ImGui::PopStyleColor();

		ImGui::SetCursorPos(cursorPos);
		if (ImGui::InvisibleButton("##BtnDashboard", ImVec2(sidebarWidth, menuItemHeight)))
		{
			if (!isDashboardActive) TransitionTo<DashboardLayer>();
		}
		if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

		ImGui::SetCursorPosY(cursorPos.y + menuItemHeight);

		// 2. Project
		cursorPos = ImGui::GetCursorPos();
		itemPMin = ImVec2(pMin.x, pMin.y + cursorPos.y);
		itemPMax = ImVec2(pMin.x + sidebarWidth - 2.0f, itemPMin.y + menuItemHeight);

		bool isProjectActive = (strcmp(activeMenu, "Project") == 0);
		if (isProjectActive)
		{
			drawList->AddRectFilled(itemPMin, itemPMax, IM_COL32(18, 20, 20, 255)); // #121414
			drawList->AddRectFilled(itemPMin, ImVec2(itemPMin.x + 4.0f, itemPMax.y), IM_COL32(0, 173, 181, 255)); // #00ADB5
		}

		ImGui::SetCursorPos(ImVec2(30.0f, cursorPos.y + (menuItemHeight - ImGui::GetTextLineHeight()) * 0.5f));
		if (isProjectActive)
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f));
		else
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f));
		
		ImGui::Text(ICON_FA_FOLDER "   Project");
		ImGui::PopStyleColor();

		ImGui::SetCursorPos(cursorPos);
		if (ImGui::InvisibleButton("##BtnProject", ImVec2(sidebarWidth, menuItemHeight)))
		{
			if (!isProjectActive) TransitionTo<ProjectLayer>();
		}
		if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

		ImGui::SetCursorPosY(cursorPos.y + menuItemHeight);

		// Help at bottom
		ImGui::SetCursorPosY(childSize.y - 40.0f - ImGui::GetTextLineHeight());
		ImGui::SetCursorPosX(30.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f));
		ImGui::Text(ICON_FA_CIRCLE_QUESTION "   Help");
		ImGui::PopStyleColor();
		
		// Add invisible button for Help too just for hover effect
		ImGui::SetCursorPos(ImVec2(0.0f, childSize.y - 40.0f - ImGui::GetTextLineHeight() - 10.0f));
		if (ImGui::InvisibleButton("##BtnHelp", ImVec2(sidebarWidth, menuItemHeight))) {}
		if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	}
	ImGui::EndChild();
	
	ImGui::PopStyleColor();
}

void AppLayoutLayer::RenderTopNavBar()
{
	float topbarHeight = 41.0f;
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(18.0f / 255.0f, 20.0f / 255.0f, 20.0f / 255.0f, 1.0f)); // #1A1C1C
	
	if (ImGui::BeginChild("TopNavBar", ImVec2(0, topbarHeight), false, ImGuiWindowFlags_NoScrollbar))
	{
		ImVec2 childSize = ImGui::GetWindowSize();
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 pMin = ImGui::GetWindowPos();
		ImVec2 pMax = ImVec2(pMin.x + childSize.x, pMin.y + childSize.y);
		
		// Draw bottom border
		drawList->AddLine(ImVec2(pMin.x, pMax.y), ImVec2(pMax.x, pMax.y), IM_COL32(60, 73, 74, 255), 3.0f); // #3C494A

		float centerY = (topbarHeight - ImGui::GetTextLineHeight()) * 0.5f;

		// Title
		ImGui::SetCursorPos(ImVec2(30.0f, centerY));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f)); // #00ADB5
		ImGui::SetWindowFontScale(1.2f);
		ImGui::Text("%s", GetTopNavBarTitle());
		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopStyleColor();

		// Right side items — GetLoggedInUser() returns a const ref to cached state (no per-frame alloc).
		const std::string& loggedInUser = TrackingTool::AuthService::GetLoggedInUser();
		const char* nameText = loggedInUser.c_str();
		float rightPadding = 30.0f;
		float nameWidth = ImGui::CalcTextSize(nameText).x;
		
		ImGui::SetCursorPos(ImVec2(childSize.x - rightPadding - nameWidth, centerY));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f)); // #E2E2E2
		ImGui::TextUnformatted(nameText);
		ImGui::PopStyleColor();
		
		// Gear Icon (settings menu: Logout, ...)
		float gearWidth = ImGui::CalcTextSize(ICON_FA_GEAR).x;
		const float gearX = childSize.x - rightPadding - nameWidth - gearWidth - 20.0f;
		ImGui::SetCursorPos(ImVec2(gearX, centerY));

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f)); // #BBC9CA
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
		if (ImGui::Button(ICON_FA_GEAR "##Settings"))
		{
			ImGui::OpenPopup("##UserSettings");
		}
		if (ImGui::IsItemHovered())
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
		ImGui::PopStyleVar();
		ImGui::PopStyleColor(4);

		// Settings dropdown anchored under the gear button
		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(26.0f / 255.0f, 28.0f / 255.0f, 28.0f / 255.0f, 1.0f)); // #1A1C1C
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(60.0f / 255.0f, 73.0f / 255.0f, 74.0f / 255.0f, 1.0f)); // #3C494A
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 0.25f));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 0.40f));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 0.55f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));

		if (ImGui::BeginPopup("##UserSettings"))
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f)); // #E2E2E2
			if (ImGui::Selectable(ICON_FA_RIGHT_FROM_BRACKET "  Logout", false, 0, ImVec2(140.0f, 0.0f)))
			{
				// Defer OpenPopup for the modal to the root layout window.
				m_OpenLogoutConfirm = true;
			}
			ImGui::PopStyleColor();
			ImGui::EndPopup();
		}

		ImGui::PopStyleVar(3);
		ImGui::PopStyleColor(5);
		
		// Bell Icon
		float bellWidth = ImGui::CalcTextSize(ICON_FA_BELL).x;
		ImGui::SetCursorPos(ImVec2(gearX - bellWidth - 20.0f, centerY));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f));
		ImGui::TextUnformatted(ICON_FA_BELL);
		ImGui::PopStyleColor();

		// Allow extensions
		OnRenderTopNavBarExtensions();
	}
	ImGui::EndChild();
	
	ImGui::PopStyleColor();
}

void AppLayoutLayer::RenderLogoutConfirmModal()
{
	if (m_OpenLogoutConfirm)
	{
		ImGui::OpenPopup("Confirm Logout");
		m_OpenLogoutConfirm = false;
	}

	const ImVec4 whiteText = ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f); // #E2E2E2
	const ImVec4 grayText = ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f); // #BBC9CA
	const ImVec4 boxBgColor = ImVec4(26.0f / 255.0f, 28.0f / 255.0f, 28.0f / 255.0f, 1.0f); // #1A1C1C
	const ImVec4 borderColor = ImVec4(60.0f / 255.0f, 73.0f / 255.0f, 74.0f / 255.0f, 1.0f); // #3C494A
	const ImVec4 redColor = ImVec4(238.0f / 255.0f, 56.0f / 255.0f, 56.0f / 255.0f, 1.0f); // #EE3838

	const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	ImGui::PushStyleColor(ImGuiCol_PopupBg, boxBgColor);
	ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 24.0f));

	if (ImGui::BeginPopupModal("Confirm Logout", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
	{
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetWindowFontScale(1.2f);
		ImGui::Text(ICON_FA_RIGHT_FROM_BRACKET " Confirm Logout");
		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 8.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 350.0f);
		ImGui::TextWrapped("Are you sure you want to log out? You will need to sign in again to continue.");
		ImGui::PopTextWrapPos();
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 20.0f));

		const float modalBtnWidth = (350.0f - 10.0f) / 2.0f;

		// Cancel (default focus — safer for a destructive action)
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		if (ImGui::Button("Cancel", ImVec2(modalBtnWidth, 36.0f)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(2);

		ImGui::SameLine(0, 10.0f);

		// Confirm logout (destructive)
		ImGui::PushStyleColor(ImGuiCol_Button, redColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(255.0f / 255.0f, 76.0f / 255.0f, 76.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(200.0f / 255.0f, 40.0f / 255.0f, 40.0f / 255.0f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		if (ImGui::Button("Logout", ImVec2(modalBtnWidth, 36.0f)))
		{
			ImGui::CloseCurrentPopup();
			TrackingTool::AuthService::Logout();
			TransitionTo<AuthenticationLayer>();
		}
		ImGui::PopStyleColor(4);
		ImGui::PopStyleVar();

		ImGui::EndPopup();
	}

	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(2);
}