#include "imgui.h"
#include "IconsFontAwesome6.h"

#include "ProjectLayer.h"

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

	// Render Invite Code Badge right-aligned before the gear icon
	// Get available width. In AppLayoutLayer, the bell/gear takes some space.
	// Let's place it using absolute position relative to the right edge.
	// Alex Rivera (approx 80px) + Gear (20px) + Bell (20px) + padding = approx 200px from right.
	const char* inviteCode = "1H4XDD";
	ImVec2 inviteSize = ImGui::CalcTextSize(inviteCode);
	float inviteTotalWidth = inviteSize.x + 40.0f; // Padding + icon
	float rightOffset = 220.0f; // Before the bell icon which is around 170px from right

	ImVec2 badgePos = ImVec2(pMin.x + childSize.x - rightOffset - inviteTotalWidth, pMin.y + (topbarHeight - 24.0f) * 0.5f);
	
	// Badge background
	drawList->AddRectFilled(badgePos, ImVec2(badgePos.x + inviteTotalWidth, badgePos.y + 24.0f), IM_COL32(51, 53, 53, 255), 4.0f);
	
	// Badge text
	drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(badgePos.x + 10.0f, badgePos.y + 4.0f), IM_COL32(187, 201, 202, 255), inviteCode);
	
	// Copy Icon
	drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(badgePos.x + inviteSize.x + 20.0f, badgePos.y + 4.0f), IM_COL32(187, 201, 202, 255), ICON_FA_COPY);
}

void ProjectLayer::OnRenderContent()
{
	// Render the content for the currently active tab
	switch (m_ActiveTab)
	{
		case ProjectTab::Tasks: m_TasksView.OnRender(); break;
		case ProjectTab::Milestones: m_MilestonesView.OnRender(); break;
		case ProjectTab::Chart: m_ChartView.OnRender(); break;
		case ProjectTab::Workload: m_WorkloadView.OnRender(); break;
		case ProjectTab::Member: m_MemberView.OnRender(); break;
	}
}