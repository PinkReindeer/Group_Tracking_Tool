#include "imgui.h"

#include "ProjectLayer.h"

void ProjectLayer::OnUpdate(float ts)
{
}

void ProjectLayer::OnRenderTopNavBarExtensions()
{
	// Render the tabs in the top navigation bar
	if (ImGui::BeginTabBar("ProjectTabs"))
	{
		if (ImGui::BeginTabItem("Tasks"))
		{
			m_ActiveTab = ProjectTab::Tasks;
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Milestones"))
		{
			m_ActiveTab = ProjectTab::Milestones;
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Chart"))
		{
			m_ActiveTab = ProjectTab::Chart;
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Workload"))
		{
			m_ActiveTab = ProjectTab::Workload;
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Member"))
		{
			m_ActiveTab = ProjectTab::Member;
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
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