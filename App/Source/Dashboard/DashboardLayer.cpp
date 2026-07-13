#include "imgui.h"
#include "IconsFontAwesome6.h"

#include "DashboardLayer.h"

#include "Platform/Application.h"
#include "Service/ProjectService.h"

#include <cctype>
#include <cstdio>

namespace
{
	// Case-insensitive compare without heap allocation (called every frame per project card).
	bool IsLeaderRole(const std::string& role)
	{
		static constexpr char kLeader[] = "leader";
		if (role.size() != sizeof(kLeader) - 1)
			return false;
		for (size_t i = 0; i < role.size(); ++i)
		{
			if (static_cast<char>(std::tolower(static_cast<unsigned char>(role[i]))) != kLeader[i])
				return false;
		}
		return true;
	}
}

DashboardLayer::DashboardLayer()
{
	// Load projects for the logged-in user when entering the dashboard.
	RefreshProjects(false);
}

void DashboardLayer::OnUpdate(float ts)
{
}

void DashboardLayer::RefreshProjects(bool showNotification)
{
	std::string message;
	std::vector<TrackingTool::ProjectInfo> projects;

	if (TrackingTool::ProjectService::GetUserProjects(projects, message))
	{
		m_Projects = std::move(projects);
		if (showNotification)
		{
			const std::string info = "Projects refreshed (" + std::to_string(m_Projects.size()) + ").";
			TrackingTool::Application::Get().PushNotification(info, NotificationType::Info);
		}
	}
	else if (showNotification)
	{
		TrackingTool::Application::Get().PushNotification(message, NotificationType::Error);
	}
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

	char projectCountStr[16];
	std::snprintf(projectCountStr, sizeof(projectCountStr), "%zu", m_Projects.size());
	DrawStatBox(0, "PROJECT", projectCountStr, cyanColor, cyanColor, cyanColor);
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

	// Live Refresh — clickable control (reloads projects from DB via projectmember)
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(51.0f / 255.0f, 53.0f / 255.0f, 53.0f / 255.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(70.0f / 255.0f, 73.0f / 255.0f, 73.0f / 255.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(40.0f / 255.0f, 42.0f / 255.0f, 42.0f / 255.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Text, grayText);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 3.0f));
	if (ImGui::Button(ICON_FA_ARROWS_ROTATE " Live Refresh"))
	{
		RefreshProjects(true);
	}
	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(4);

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
	if (ImGui::Button(ICON_FA_RIGHT_TO_BRACKET " Join Project", ImVec2(btnWidth, 32.0f))) {}
	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar(2);
	
	ImGui::SameLine(0, btnSpacing);

	// Solid Cyan button (+ New Project)
	ImGui::PushStyleColor(ImGuiCol_Button, cyanColor);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 193.0f/255.0f, 201.0f/255.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 153.0f/255.0f, 161.0f/255.0f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(18.0f/255.0f, 20.0f/255.0f, 20.0f/255.0f, 1.0f)); // Dark text
	if (ImGui::Button(ICON_FA_PLUS " New Project", ImVec2(btnWidth, 32.0f)))
	{
		ImGui::OpenPopup("Create Project");
	}
	ImGui::PopStyleColor(4);
	ImGui::PopStyleVar();

	// Modal popup for creating a project
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	
	ImGui::PushStyleColor(ImGuiCol_PopupBg, boxBgColor);
	ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 24.0f));

	if (ImGui::BeginPopupModal("Create Project", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
	{
		// Header
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetWindowFontScale(1.2f);
		ImGui::Text(ICON_FA_PLUS " Create New Project");
		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 15.0f));

		// Frame styling for inputs
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(15.0f / 255.0f, 16.0f / 255.0f, 16.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(35.0f / 255.0f, 38.0f / 255.0f, 38.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(45.0f / 255.0f, 48.0f / 255.0f, 48.0f / 255.0f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 10.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::Text("PROJECT NAME");
		ImGui::PopStyleColor();

		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetNextItemWidth(350.0f);
		if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
		ImGui::InputText("##ProjectName", m_NewProjectName, IM_ARRAYSIZE(m_NewProjectName));
		ImGui::PopStyleColor();
		
		ImGui::Dummy(ImVec2(0.0f, 10.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::Text("DESCRIPTION (OPTIONAL)");
		ImGui::PopStyleColor();

		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::InputTextMultiline("##ProjectDesc", m_NewProjectDescription, IM_ARRAYSIZE(m_NewProjectDescription), ImVec2(350.0f, 100.0f));
		ImGui::PopStyleColor();
		
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(3);
		
		ImGui::Dummy(ImVec2(0.0f, 20.0f));

		float modalBtnWidth = (350.0f - 10.0f) / 2.0f;

		// Cancel Button
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		if (ImGui::Button("Cancel", ImVec2(modalBtnWidth, 36.0f)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(2);

		ImGui::SameLine(0, 10.0f);

		// Create Button
		ImGui::PushStyleColor(ImGuiCol_Button, cyanColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 193.0f / 255.0f, 201.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 153.0f / 255.0f, 161.0f / 255.0f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(18.0f / 255.0f, 20.0f / 255.0f, 20.0f / 255.0f, 1.0f));
		if (ImGui::Button("Create", ImVec2(modalBtnWidth, 36.0f)))
		{
			std::string name = m_NewProjectName;
			std::string desc = m_NewProjectDescription;
			std::string message;
			std::string code;

			if (TrackingTool::ProjectService::CreateProject(name, desc, message, code))
			{
				TrackingTool::Application::Get().PushNotification(message, NotificationType::Info);
				RefreshProjects(false);

				memset(m_NewProjectName, 0, sizeof(m_NewProjectName));
				memset(m_NewProjectDescription, 0, sizeof(m_NewProjectDescription));
				ImGui::CloseCurrentPopup();
			}
			else
			{
				TrackingTool::Application::Get().PushNotification(message, NotificationType::Error);
			}
		}
		ImGui::SetItemDefaultFocus();
		ImGui::PopStyleColor(4);
		ImGui::PopStyleVar();

		ImGui::EndPopup();
	}
	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(2);

	ImGui::Dummy(ImVec2(0.0f, 20.0f));

	// Project Grid
	ImGui::PushStyleColor(ImGuiCol_ChildBg, boxBgColor);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
	ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

	float windowVisibleX2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
	for (size_t i = 0; i < m_Projects.size(); ++i)
	{
		const auto& project = m_Projects[i];
		// Use stable integer ID — avoids heap-allocating a std::string every frame.
		if (ImGui::BeginChild(static_cast<ImGuiID>(project.Id), ImVec2(300.0f, 150.0f), true, ImGuiWindowFlags_NoScrollbar))
		{
			ImGui::SetCursorPos(ImVec2(20.0f, 20.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
			ImGui::Text("%s", project.Name.c_str());
			ImGui::PopStyleColor();

			ImGui::SameLine();
			
			// Role badge (from projectmember.role)
			const bool isLeader = IsLeaderRole(project.Role);
			const char* roleLabel = isLeader ? ICON_FA_USER " Leader" : ICON_FA_USER " Member";
			ImVec2 roleBadgePos = ImGui::GetCursorScreenPos();
			roleBadgePos.y -= 2.0f;
			ImVec2 roleBadgeSize = ImVec2(isLeader ? 60.0f : 68.0f, 18.0f);
			ImGui::GetWindowDrawList()->AddRectFilled(
				roleBadgePos,
				ImVec2(roleBadgePos.x + roleBadgeSize.x, roleBadgePos.y + roleBadgeSize.y),
				ImGui::GetColorU32(ImVec4(0.0f, 173.0f/255.0f, 181.0f/255.0f, 0.2f)),
				9.0f);
			ImGui::GetWindowDrawList()->AddText(
				ImGui::GetFont(),
				ImGui::GetFontSize() * 0.7f,
				ImVec2(roleBadgePos.x + 8.0f, roleBadgePos.y + 2.0f),
				ImGui::GetColorU32(cyanColor),
				roleLabel);

			// 3-dots icon
			ImGui::SetCursorPos(ImVec2(300.0f - 30.0f, 20.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, grayText);
			ImGui::Text(ICON_FA_ELLIPSIS_VERTICAL);
			ImGui::PopStyleColor();

			// Optional project code under name
			ImGui::SetCursorPos(ImVec2(20.0f, 42.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, grayText);
			ImGui::SetWindowFontScale(0.75f);
			ImGui::Text("Code: %s", project.Code.c_str());
			ImGui::SetWindowFontScale(1.0f);
			ImGui::PopStyleColor();

			ImGui::SetCursorPos(ImVec2(20.0f, 70.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, grayText);
			ImGui::PushTextWrapPos(280.0f);
			ImGui::TextUnformatted(project.Description.empty() ? "No description." : project.Description.c_str());
			ImGui::PopTextWrapPos();
			ImGui::PopStyleColor();
		}
		ImGui::EndChild();

		float last_item_x2 = ImGui::GetItemRectMax().x;
		float next_item_x2 = last_item_x2 + 20.0f + 300.0f;
		if (i + 1 < m_Projects.size() && next_item_x2 < windowVisibleX2)
			ImGui::SameLine(0, 20.0f);
	}

	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(2);
}
