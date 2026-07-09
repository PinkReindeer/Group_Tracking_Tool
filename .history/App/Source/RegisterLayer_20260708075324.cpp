#include <glad/gl.h>

#include "RegisterLayer.h"

#include "imgui.h"

#include "Platform/Application.h"
#include "Service/AuthService.h"

#include <cstring>
#include <string>

RegisterLayer::RegisterLayer()
{
}

RegisterLayer::~RegisterLayer()
{
}

void RegisterLayer::OnUpdate(float ts)
{
}

void RegisterLayer::OnRender()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// Centre the registration window on the screen.
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 windowSize(400.0f, 280.0f);
	ImVec2 windowPos(viewport->WorkPos.x + (viewport->WorkSize.x - windowSize.x) * 0.5f, viewport->WorkPos.y + (viewport->WorkSize.y - windowSize.y) * 0.5f);

	ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

	if (ImGui::Begin("Register", nullptr, flags))
	{
		ImGui::TextWrapped("Create a new account");
		ImGui::Separator();
		ImGui::Spacing();

		// Input fields — each fills the available width.
		float inputWidth = ImGui::GetContentRegionAvail().x;

		ImGui::Text("Username");
		ImGui::SetNextItemWidth(inputWidth);
		ImGui::InputText("##userName", m_UserName, sizeof(m_UserName));

		ImGui::Spacing();

		ImGui::Text("Password");
		ImGui::SetNextItemWidth(inputWidth);
		ImGui::InputText("##password", m_Password, sizeof(m_Password), ImGuiInputTextFlags_Password);

		ImGui::Spacing();
		ImGui::Spacing();

		bool can_register =  strlen(m_UserName) > 0 && strlen(m_Password) > 0;
		if (!canRegister)
		{
    		ImGui::BeginDisabled();
		}
		if(can_register)
		{
			if (ImGui::Button("Register", ImVec2(inputWidth, 0)))
			{
				bool success = TrackingTool::AuthService::Register(m_UserName, m_Password, m_NotifationMessage);

				if(success)
				{
					std::memset(m_UserName,0,sizeof(m_UserName));
					std::memset(m_Password, 0, sizeof(m_Password));
				}
				else
				{
					std::memset(m_Password,0,sizeof(m_Password));
				}
			}
		}
		if (!canRegister)
		{
    		ImGui::EndDisabled();
		}
		

	}
	if (!m_NotifationMessage.empty())
	{
		ImGui::Spacing();

		ImVec4 color =
			m_NotifationMessage == "Registration successful."
			? ImVec4(0,1,0,1)
			: ImVec4(1,0.3f,0.3f,1);

		ImGui::PushStyleColor(ImGuiCol_Text, color);
		ImGui::TextWrapped("%s", m_NotifationMessage.c_str());
		ImGui::PopStyleColor();
	}
	ImGui::End();
}