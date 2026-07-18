#include "imgui.h"
#include "IconsFontAwesome6.h"

#include "DashboardLayer.h"

#include "Platform/Application.h"
#include "Project/ProjectLayer.h"
#include "Service/AuthService.h"
#include "Service/ProjectService.h"
#include "Utils/TimeUtils.h"

#include <cctype>
#include <cstdio>
#include <cstring>

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

	// Case-insensitive ASCII compare against a literal — no heap allocation.
	bool EqualsIgnoreCase(const std::string& value, const char* literal)
	{
		size_t i = 0;
		for (; i < value.size() && literal[i] != '\0'; ++i)
		{
			const unsigned char a = static_cast<unsigned char>(value[i]);
			const unsigned char b = static_cast<unsigned char>(literal[i]);
			if (std::tolower(a) != std::tolower(b))
				return false;
		}
		return i == value.size() && literal[i] == '\0';
	}

	// Overdue when deadline is before today and the task is not already done.
	bool IsTaskOverdue(const TrackingTool::TaskInfo& task, const char* todayMmDdYyyy)
	{
		if (task.Deadline.empty() || EqualsIgnoreCase(task.Status, "done"))
			return false;
		if (!todayMmDdYyyy || !TrackingTool::Utils::IsValidMmDdYyyy(task.Deadline.c_str()))
			return false;
		return TrackingTool::Utils::CompareMmDdYyyy(task.Deadline.c_str(), todayMmDdYyyy) < 0;
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

void DashboardLayer::RefreshProjects(bool forceRefresh, bool showNotification)
{
	std::string message;
	std::vector<TrackingTool::ProjectInfo> projects;

	if (TrackingTool::ProjectService::GetUserProjects(projects, message, forceRefresh))
	{
		m_Projects = std::move(projects);
		RefreshTaskStats(forceRefresh);
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

void DashboardLayer::RefreshTaskStats(bool forceRefresh)
{
	m_PendingTaskCount = 0;
	m_InProgressTaskCount = 0;
	m_OverdueTaskCount = 0;

	if (!TrackingTool::AuthService::IsLoggedIn() || m_Projects.empty())
		return;

	const std::string& userName = TrackingTool::AuthService::GetLoggedInUser();
	const char* today = TrackingTool::Utils::GetTodayMmDdYyyy();

	for (const TrackingTool::ProjectInfo& project : m_Projects)
	{
		std::vector<TrackingTool::TaskInfo> tasks;
		std::string message;
		if (!TrackingTool::ProjectService::GetProjectTasks(project.Id, tasks, message, forceRefresh))
			continue;

		for (const TrackingTool::TaskInfo& task : tasks)
		{
			// Personal dashboard: only count tasks assigned to the logged-in user.
			if (!EqualsIgnoreCase(task.AssignedMemberName, userName.c_str()))
				continue;

			if (EqualsIgnoreCase(task.Status, "pending"))
				++m_PendingTaskCount;
			else if (EqualsIgnoreCase(task.Status, "in progress"))
				++m_InProgressTaskCount;

			if (IsTaskOverdue(task, today))
				++m_OverdueTaskCount;
		}
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
	char pendingCountStr[16];
	char inProgressCountStr[16];
	char overdueCountStr[16];
	std::snprintf(projectCountStr, sizeof(projectCountStr), "%zu", m_Projects.size());
	std::snprintf(pendingCountStr, sizeof(pendingCountStr), "%d", m_PendingTaskCount);
	std::snprintf(inProgressCountStr, sizeof(inProgressCountStr), "%d", m_InProgressTaskCount);
	std::snprintf(overdueCountStr, sizeof(overdueCountStr), "%d", m_OverdueTaskCount);
	DrawStatBox(0, "PROJECT", projectCountStr, cyanColor, cyanColor, cyanColor);
	DrawStatBox(1, "PENDING TASKS", pendingCountStr, grayText, whiteText, ImVec4(80.0f/255.0f, 80.0f/255.0f, 80.0f/255.0f, 1.0f));
	DrawStatBox(2, "IN PROGRESS", inProgressCountStr, blueColor, blueColor, blueColor);
	DrawStatBox(3, "OVERDUE", overdueCountStr, redColor, redColor, redColor);

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
		RefreshProjects(true, true);
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
	if (ImGui::Button(ICON_FA_RIGHT_TO_BRACKET " Join Project", ImVec2(btnWidth, 32.0f)))
	{
		ImGui::OpenPopup("Join Project");
	}
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

	// Shared modal chrome (Join + Create)
	const ImVec2 center = ImGui::GetMainViewport()->GetCenter();

	ImGui::PushStyleColor(ImGuiCol_PopupBg, boxBgColor);
	ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 24.0f));

	// Modal popup for joining a project by invite code
	// SetNextWindowPos must be immediately before BeginPopupModal (same pattern as Create).
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal("Join Project", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
	{
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetWindowFontScale(1.2f);
		ImGui::Text(ICON_FA_RIGHT_TO_BRACKET " Join Project");
		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 8.0f));

		// Cap wrap width so AlwaysAutoResize doesn't stretch to the full parent width
		// (which made the dialog look off-center compared to Create Project).
		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 350.0f);
		ImGui::TextWrapped("Enter the 6-character invite code shared by the project leader.");
		ImGui::PopTextWrapPos();
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 15.0f));

		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(15.0f / 255.0f, 16.0f / 255.0f, 16.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(35.0f / 255.0f, 38.0f / 255.0f, 38.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(45.0f / 255.0f, 48.0f / 255.0f, 48.0f / 255.0f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 10.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::Text("PROJECT CODE");
		ImGui::PopStyleColor();

		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetNextItemWidth(350.0f);
		if (ImGui::IsWindowAppearing())
			ImGui::SetKeyboardFocusHere();
		const bool submitJoin = ImGui::InputText("##JoinProjectCode", m_JoinProjectCode, IM_ARRAYSIZE(m_JoinProjectCode),
			ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsUppercase);
		ImGui::PopStyleColor();

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(3);

		ImGui::Dummy(ImVec2(0.0f, 20.0f));

		const float modalBtnWidth = (350.0f - 10.0f) / 2.0f;

		auto tryJoinProject = [&]() {
			std::string message;
			std::string projectName;
			if (TrackingTool::ProjectService::JoinProject(m_JoinProjectCode, message, projectName))
			{
				TrackingTool::Application::Get().PushNotification(message, NotificationType::Info);
				RefreshProjects(false);
				memset(m_JoinProjectCode, 0, sizeof(m_JoinProjectCode));
				ImGui::CloseCurrentPopup();
			}
			else
			{
				TrackingTool::Application::Get().PushNotification(message, NotificationType::Error);
			}
		};

		// Cancel
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

		// Join
		ImGui::PushStyleColor(ImGuiCol_Button, cyanColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 193.0f / 255.0f, 201.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 153.0f / 255.0f, 161.0f / 255.0f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(18.0f / 255.0f, 20.0f / 255.0f, 20.0f / 255.0f, 1.0f));
		if (ImGui::Button("Join", ImVec2(modalBtnWidth, 36.0f)) || submitJoin)
		{
			tryJoinProject();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::PopStyleColor(4);
		ImGui::PopStyleVar();

		ImGui::EndPopup();
	}

	// Modal popup for creating a project
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
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
	// Horizontal gap is applied via SameLine; vertical gap between rows uses ItemSpacing.y.
	constexpr float kProjectCardGapX = 20.0f;
	constexpr float kProjectCardGapY = 24.0f;
	ImGui::PushStyleColor(ImGuiCol_ChildBg, boxBgColor);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
	ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(kProjectCardGapX, kProjectCardGapY));

	float windowVisibleX2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
	for (size_t i = 0; i < m_Projects.size(); ++i)
	{
		const auto& project = m_Projects[i];
		bool menuButtonHovered = false;
		char menuPopupId[64];
		std::snprintf(menuPopupId, sizeof(menuPopupId), "##ProjectActions%d", project.Id);

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
			ImGui::GetWindowDrawList()->AddRectFilled(roleBadgePos, ImVec2(roleBadgePos.x + roleBadgeSize.x, roleBadgePos.y + roleBadgeSize.y),
				ImGui::GetColorU32(ImVec4(0.0f, 173.0f/255.0f, 181.0f/255.0f, 0.2f)), 9.0f);
			ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 0.7f, ImVec2(roleBadgePos.x + 8.0f, roleBadgePos.y + 2.0f),
				ImGui::GetColorU32(cyanColor), roleLabel);

			// 3-dots overflow menu (Edit / Delete) — excluded from card navigation
			ImGui::SetCursorPos(ImVec2(300.0f - 30.0f, 18.0f));
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_Text, grayText);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));

			char menuBtnId[64];
			std::snprintf(menuBtnId, sizeof(menuBtnId), ICON_FA_ELLIPSIS_VERTICAL "##menu%d", project.Id);

			if (ImGui::Button(menuBtnId))
				ImGui::OpenPopup(menuPopupId);
			menuButtonHovered = ImGui::IsItemHovered();
			if (menuButtonHovered)
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

			ImGui::PopStyleVar();
			ImGui::PopStyleColor(4);

			ImGui::PushStyleColor(ImGuiCol_PopupBg, boxBgColor);
			ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 0.25f));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 0.40f));
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 0.55f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));

			if (ImGui::BeginPopup(menuPopupId))
			{
				const bool canManage = isLeader;

				ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
				if (ImGui::Selectable(ICON_FA_PEN "  Edit", false, canManage ? 0 : ImGuiSelectableFlags_Disabled, ImVec2(100.0f, 0.0f)))
				{
					m_ActionProjectId = project.Id;
					m_ActionProjectName = project.Name;
					std::memset(m_EditProjectName, 0, sizeof(m_EditProjectName));
					std::memset(m_EditProjectDescription, 0, sizeof(m_EditProjectDescription));
					std::snprintf(m_EditProjectName, sizeof(m_EditProjectName), "%s", project.Name.c_str());
					std::snprintf(m_EditProjectDescription, sizeof(m_EditProjectDescription), "%s", project.Description.c_str());
					m_OpenEditProject = true;
				}
				ImGui::PopStyleColor();

				ImGui::PushStyleColor(ImGuiCol_Text, redColor);
				if (ImGui::Selectable(ICON_FA_TRASH "  Delete", false, canManage ? 0 : ImGuiSelectableFlags_Disabled, ImVec2(100.0f, 0.0f)))
				{
					m_ActionProjectId = project.Id;
					m_ActionProjectName = project.Name;
					m_OpenDeleteProject = true;
				}
				ImGui::PopStyleColor();

				if (!canManage)
				{
					ImGui::PushStyleColor(ImGuiCol_Text, grayText);
					ImGui::SetWindowFontScale(0.75f);
					ImGui::TextUnformatted("Leader only");
					ImGui::SetWindowFontScale(1.0f);
					ImGui::PopStyleColor();
				}

				ImGui::EndPopup();
			}

			ImGui::PopStyleVar(3);
			ImGui::PopStyleColor(5);

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

		// Card click open that project (skip menu and its popup).
		const bool cardHovered = ImGui::IsItemHovered();
		if (cardHovered)
		{
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			const ImVec2 cardMin = ImGui::GetItemRectMin();
			const ImVec2 cardMax = ImGui::GetItemRectMax();
			ImGui::GetWindowDrawList()->AddRect(cardMin, cardMax, ImGui::GetColorU32(cyanColor), 4.0f, 0, 1.5f);

			if (!menuButtonHovered && !ImGui::IsPopupOpen(menuPopupId) &&
				ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				TrackingTool::ProjectService::SetActiveProject(project);
				TransitionTo<ProjectLayer>();
			}
		}

		float last_item_x2 = ImGui::GetItemRectMax().x;
		float next_item_x2 = last_item_x2 + kProjectCardGapX + 300.0f;
		if (i + 1 < m_Projects.size() && next_item_x2 < windowVisibleX2)
			ImGui::SameLine(0, kProjectCardGapX);
	}

	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor(2);

	// Deferred modals (stable ID stack outside per-card child/popup).
	RenderEditProjectModal();
	RenderDeleteProjectModal();
}

void DashboardLayer::RenderEditProjectModal()
{
	if (m_OpenEditProject)
	{
		ImGui::OpenPopup("Edit Project");
		m_OpenEditProject = false;
	}

	const ImVec4 whiteText = ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f);
	const ImVec4 grayText = ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f);
	const ImVec4 cyanColor = ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f);
	const ImVec4 boxBgColor = ImVec4(26.0f / 255.0f, 28.0f / 255.0f, 28.0f / 255.0f, 1.0f);
	const ImVec4 borderColor = ImVec4(60.0f / 255.0f, 73.0f / 255.0f, 74.0f / 255.0f, 1.0f);

	const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	ImGui::PushStyleColor(ImGuiCol_PopupBg, boxBgColor);
	ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 24.0f));

	if (ImGui::BeginPopupModal("Edit Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
	{
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetWindowFontScale(1.2f);
		ImGui::Text(ICON_FA_PEN " Edit Project");
		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 15.0f));

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
		if (ImGui::IsWindowAppearing())
			ImGui::SetKeyboardFocusHere();
		ImGui::InputText("##EditProjectName", m_EditProjectName, IM_ARRAYSIZE(m_EditProjectName));
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 10.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::Text("DESCRIPTION (OPTIONAL)");
		ImGui::PopStyleColor();

		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::InputTextMultiline("##EditProjectDesc", m_EditProjectDescription, IM_ARRAYSIZE(m_EditProjectDescription), ImVec2(350.0f, 100.0f));
		ImGui::PopStyleColor();

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(3);

		ImGui::Dummy(ImVec2(0.0f, 20.0f));

		const float modalBtnWidth = (350.0f - 10.0f) / 2.0f;

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

		ImGui::PushStyleColor(ImGuiCol_Button, cyanColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 193.0f / 255.0f, 201.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 153.0f / 255.0f, 161.0f / 255.0f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(18.0f / 255.0f, 20.0f / 255.0f, 20.0f / 255.0f, 1.0f));
		if (ImGui::Button("Save", ImVec2(modalBtnWidth, 36.0f)))
		{
			std::string message;
			if (TrackingTool::ProjectService::UpdateProject(m_ActionProjectId, m_EditProjectName, m_EditProjectDescription, message))
			{
				TrackingTool::Application::Get().PushNotification(message, NotificationType::Info);
				RefreshProjects(false);
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
}

void DashboardLayer::RenderDeleteProjectModal()
{
	if (m_OpenDeleteProject)
	{
		ImGui::OpenPopup("Delete Project");
		m_OpenDeleteProject = false;
	}

	const ImVec4 whiteText = ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f);
	const ImVec4 grayText = ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f);
	const ImVec4 redColor = ImVec4(238.0f / 255.0f, 56.0f / 255.0f, 56.0f / 255.0f, 1.0f);
	const ImVec4 boxBgColor = ImVec4(26.0f / 255.0f, 28.0f / 255.0f, 28.0f / 255.0f, 1.0f);
	const ImVec4 borderColor = ImVec4(60.0f / 255.0f, 73.0f / 255.0f, 74.0f / 255.0f, 1.0f);

	const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	ImGui::PushStyleColor(ImGuiCol_PopupBg, boxBgColor);
	ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 24.0f));

	if (ImGui::BeginPopupModal("Delete Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
	{
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetWindowFontScale(1.2f);
		ImGui::Text(ICON_FA_TRASH " Delete Project");
		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 8.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 350.0f);
		ImGui::TextWrapped("Are you sure you want to permanently delete \"%s\"? This removes the project and all memberships. This action cannot be undone.",
			m_ActionProjectName.c_str());
		ImGui::PopTextWrapPos();
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 20.0f));

		const float modalBtnWidth = (350.0f - 10.0f) / 2.0f;

		// Cancel (default focus for destructive action)
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

		ImGui::PushStyleColor(ImGuiCol_Button, redColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(255.0f / 255.0f, 76.0f / 255.0f, 76.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(200.0f / 255.0f, 40.0f / 255.0f, 40.0f / 255.0f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		if (ImGui::Button("Delete", ImVec2(modalBtnWidth, 36.0f)))
		{
			std::string message;
			if (TrackingTool::ProjectService::DeleteProject(m_ActionProjectId, message))
			{
				TrackingTool::Application::Get().PushNotification(message, NotificationType::Info);
				RefreshProjects(false);
				ImGui::CloseCurrentPopup();
			}
			else
			{
				TrackingTool::Application::Get().PushNotification(message, NotificationType::Error);
			}
		}
		ImGui::PopStyleColor(4);
		ImGui::PopStyleVar();

		ImGui::EndPopup();
	}

	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(2);
}
