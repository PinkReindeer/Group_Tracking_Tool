#include "DashboardLayer.h"

#include "imgui.h"

DashboardLayer::DashboardLayer()
{
}

DashboardLayer::~DashboardLayer()
{
}

void DashboardLayer::OnUpdate(float ts)
{
}

void DashboardLayer::OnRender()
{
	// Main screen background #1E1E24
	glClearColor(30.0f / 255.0f, 30.0f / 255.0f, 36.0f / 255.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	ImGui::SetNextWindowPos(viewport->WorkPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(viewport->WorkSize, ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.0f); // Transparent main window

	ImGuiWindowFlags mainFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
	
	if (ImGui::Begin("DashboardContainer", nullptr, mainFlags))
	{
		ImGui::SetWindowFontScale(1.8f);
		ImVec4 cyanColor = ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f); // #00ADB5
		ImGui::PushStyleColor(ImGuiCol_Text, cyanColor);
		ImGui::Text("Welcome to Dashboard!");
		ImGui::PopStyleColor();
	}
	ImGui::End();
}
