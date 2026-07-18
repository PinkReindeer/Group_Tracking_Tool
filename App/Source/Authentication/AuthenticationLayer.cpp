#include "AuthenticationLayer.h"
#include "Dashboard/DashboardLayer.h"

#include <glad/gl.h>
#include "imgui.h"
#include "IconsFontAwesome6.h"

void AuthenticationLayer::OnUpdate(float ts)
{
	if (m_CurrentState == AuthState::Login)
		m_LoginView.OnUpdate(ts);
	else
		m_RegisterView.OnUpdate(ts);
}

void AuthenticationLayer::OnRender()
{
	// Main screen background #1E1E24
	glClearColor(30.0f / 255.0f, 30.0f / 255.0f, 36.0f / 255.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	// Overall container size depends on state
	ImVec2 containerSize = (m_CurrentState == AuthState::Login) ? ImVec2(420.0f, 392.0f) : ImVec2(420.0f, 468.0f);
	ImVec2 containerPos(viewport->WorkPos.x + (viewport->WorkSize.x - containerSize.x) * 0.5f, viewport->WorkPos.y + (viewport->WorkSize.y - containerSize.y) * 0.5f);

	ImGui::SetNextWindowPos(containerPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(containerSize, ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.0f); // Transparent main window

	ImGuiWindowFlags mainFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollWithMouse |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
	
	if (ImGui::Begin("AuthContainer", nullptr, mainFlags))
	{
		ImVec4 cyanColor = ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f); // #00ADB5
		ImVec4 subtitleText = ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f);
		
		// Header Row: Title and Shield Icon
		ImGui::SetWindowFontScale(1.2f);
		ImGui::PushStyleColor(ImGuiCol_Text, cyanColor);
		ImGui::Text("Group Tracking Tool");
		ImGui::PopStyleColor();
		
		ImGui::SameLine();
		
		// Align Shield Icon to the right
		const char* shieldIcon = ICON_FA_USER_SHIELD;
		float shieldWidth = ImGui::CalcTextSize(shieldIcon).x;
		ImGui::SetCursorPosX(containerSize.x - shieldWidth - 8.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, cyanColor);
		ImGui::Text("%s", shieldIcon);
		ImGui::PopStyleColor();

		// Subtitle
		ImGui::SetWindowFontScale(1.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, subtitleText);
		ImGui::Text("Version 0.1.0 - Beta");
		ImGui::PopStyleColor();
		
		ImGui::Dummy(ImVec2(0.0f, 16.0f));

		bool wantToSwitch = false;

		if (m_CurrentState == AuthState::Login)
		{
			bool loginSuccess = m_LoginView.Render(containerSize, wantToSwitch);
			if (loginSuccess)
			{
				TransitionTo<DashboardLayer>();
			}
			else if (wantToSwitch)
			{
				m_CurrentState = AuthState::Register;
			}
		}
		else if (m_CurrentState == AuthState::Register)
		{
			bool registerSuccess = m_RegisterView.Render(containerSize, wantToSwitch);
			if (registerSuccess)
			{
				// Registration usually routes back to Login
				m_CurrentState = AuthState::Login;
			}
			else if (wantToSwitch)
			{
				m_CurrentState = AuthState::Login;
			}
		}
	}
	ImGui::End();

	ImGui::PopStyleVar();
	ImGui::PopStyleColor(2);
}
