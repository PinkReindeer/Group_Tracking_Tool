#include "imgui.h"
#include "IconsFontAwesome6.h"

#include "ProjectLayer.h"
#include "Service/AuthService.h"
#include "Service/ProjectService.h"

#include <cctype>

namespace
{
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

ProjectLayer::ProjectLayer()
{
	// Prefer the project the user opened from the dashboard grid.
	m_HasProject = TrackingTool::ProjectService::GetActiveProject(m_Project);
}

void ProjectLayer::OnUpdate(float ts)
{
}

void ProjectLayer::OnRenderTopNavBarExtensions()
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImVec2 pMin = ImGui::GetWindowPos();
	ImVec2 childSize = ImGui::GetWindowSize();
	float topbarHeight = childSize.y;

	// Draw custom tabs next to the title
	float currentX = 140.0f; // Offset from the title "Project"
	float centerY = (topbarHeight - ImGui::GetTextLineHeight()) * 0.5f;

	auto DrawTab = [&](const char* label, ProjectTab tabVal) {
		ImGui::SetCursorPos(ImVec2(currentX, centerY));
		bool isActive = (m_ActiveTab == tabVal);

		if (isActive)
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f)); // #00ADB5
		else
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f)); // #BBC9CA

		ImVec2 textSize = ImGui::CalcTextSize(label);

		// Invisible button for interaction
		ImGui::SetCursorPos(ImVec2(currentX, 0));
		if (ImGui::InvisibleButton(label, ImVec2(textSize.x, topbarHeight)))
		{
			m_ActiveTab = tabVal;
		}
		if (ImGui::IsItemHovered())
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

		// Draw text
		ImGui::SetCursorPos(ImVec2(currentX, centerY));
		ImGui::Text("%s", label);

		// Draw active bottom border
		if (isActive)
		{
			drawList->AddLine(ImVec2(pMin.x + currentX, pMin.y + topbarHeight - 2.0f),
				ImVec2(pMin.x + currentX + textSize.x, pMin.y + topbarHeight - 2.0f),
				IM_COL32(0, 173, 181, 255), 3.0f);
		}

		ImGui::PopStyleColor();
		currentX += textSize.x + 30.0f; // Spacing between tabs
		};

	DrawTab("Tasks", ProjectTab::Tasks);
	DrawTab("Milestones", ProjectTab::Milestones);
	DrawTab("Chart", ProjectTab::Chart);
	DrawTab("Workload", ProjectTab::Workload);
	DrawTab("Member", ProjectTab::Member);

	// Invite code badge (active project) — snug against the bell/gear cluster on the right.
	if (m_HasProject && !m_Project.Code.empty())
	{
		const char* inviteCode = m_Project.Code.c_str();
		ImVec2 inviteSize = ImGui::CalcTextSize(inviteCode);

		const bool justCopied = ImGui::GetTime() < m_CopyFeedbackUntil;
		const char* actionIcon = justCopied ? ICON_FA_CHECK : ICON_FA_COPY;
		// Use the wider of the two icons so the badge width does not jump on copy feedback.
		const float copyIconW = ImGui::CalcTextSize(ICON_FA_COPY).x;
		const float checkIconW = ImGui::CalcTextSize(ICON_FA_CHECK).x;
		const float iconSize = copyIconW > checkIconW ? copyIconW : checkIconW;

		// Match AppLayoutLayer::RenderTopNavBar right-side layout so the badge sits
		// just left of the notification bell regardless of username length.
		const float rightPadding = 30.0f;
		const float iconGap = 20.0f;
		const float gapBeforeBell = 12.0f;
		const float nameWidth = ImGui::CalcTextSize(TrackingTool::AuthService::GetLoggedInUser().c_str()).x;
		const float gearWidth = ImGui::CalcTextSize(ICON_FA_GEAR).x;
		const float bellWidth = ImGui::CalcTextSize(ICON_FA_BELL).x;
		const float rightControlsWidth = rightPadding + nameWidth + iconGap + gearWidth + iconGap + bellWidth;
		const float rightOffset = rightControlsWidth + gapBeforeBell;

		const float badgePadX = 10.0f;
		const float badgeHeight = 24.0f;
		const float inviteTotalWidth = badgePadX + inviteSize.x + 8.0f + iconSize + badgePadX;
		ImVec2 badgePos = ImVec2(pMin.x + childSize.x - rightOffset - inviteTotalWidth, pMin.y + (topbarHeight - badgeHeight) * 0.5f);

		const ImU32 badgeBg = justCopied ? IM_COL32(0, 173, 181, 40) : IM_COL32(51, 53, 53, 255);
		const ImU32 badgeBorder = justCopied ? IM_COL32(0, 173, 181, 180) : IM_COL32(51, 53, 53, 255);
		drawList->AddRectFilled(badgePos, ImVec2(badgePos.x + inviteTotalWidth, badgePos.y + badgeHeight), badgeBg, 4.0f);
		if (justCopied)
			drawList->AddRect(badgePos, ImVec2(badgePos.x + inviteTotalWidth, badgePos.y + badgeHeight), badgeBorder, 4.0f, 0, 1.0f);

		const ImU32 codeColor = justCopied ? IM_COL32(0, 173, 181, 255) : IM_COL32(187, 201, 202, 255);
		drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(),
			ImVec2(badgePos.x + badgePadX, badgePos.y + (badgeHeight - inviteSize.y) * 0.5f),
			codeColor, inviteCode);

		// Whole badge is clickable for easier copy.
		ImGui::SetCursorScreenPos(badgePos);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.08f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.14f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
		if (ImGui::Button("##CopyProjectCodeBadge", ImVec2(inviteTotalWidth, badgeHeight)))
		{
			ImGui::SetClipboardText(inviteCode);
			m_CopyFeedbackUntil = ImGui::GetTime() + 1.5;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			ImGui::SetTooltip(justCopied ? "Copied!" : "Copy Project Code");
		}
		ImGui::PopStyleVar();
		ImGui::PopStyleColor(3);

		// Action icon overlaid on the right side of the badge (drawn after the invisible hit target).
		const ImU32 iconColor = justCopied ? IM_COL32(0, 173, 181, 255) : IM_COL32(187, 201, 202, 255);
		drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(),
			ImVec2(badgePos.x + inviteTotalWidth - badgePadX - iconSize, badgePos.y + (badgeHeight - ImGui::GetFontSize()) * 0.5f),
			iconColor, actionIcon);
	}
}

void ProjectLayer::OnRenderContent()
{
	if (!m_HasProject)
	{
		const ImVec4 grayText = ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::TextUnformatted("Select a project from the Dashboard to open it here.");
		ImGui::PopStyleColor();
		return;
	}

	const char* projectName = m_Project.Name.c_str();
	const char* createdDate = m_Project.CreatedDate.c_str();

	// Render the content for the currently active tab
	switch (m_ActiveTab)
	{
		case ProjectTab::Tasks: m_TasksView.OnRender(projectName, createdDate, m_Project.Id, IsLeaderRole(m_Project.Role)); break;
		case ProjectTab::Milestones: m_MilestonesView.OnRender(projectName, createdDate, m_Project.Id, IsLeaderRole(m_Project.Role)); break;
		case ProjectTab::Chart: m_ChartView.OnRender(projectName, createdDate, m_Project.Id); break;
		case ProjectTab::Workload: m_WorkloadView.OnRender(projectName, createdDate); break;
		case ProjectTab::Member: m_MemberView.OnRender(m_Project.Id, projectName, createdDate); break;
	}
}