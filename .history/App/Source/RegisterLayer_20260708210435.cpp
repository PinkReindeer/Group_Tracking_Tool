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

		ImGui::Text("Password Confirmation");
		ImGui::SetNextItemWidth(inputWidth);
		ImGui::InputText("##confirm", m_ConfirmPassword, sizeof(m_ConfirmPassword), ImGuiInputTextFlags_Password);
		ImGui::Spacing();

		ImGui::Spacing();
		ImGui::Spacing();

		if (ImGui::Button("Register", ImVec2(inputWidth, 0)))
		{
			m_NotifationMessage.clear();

			bool success = TrackingTool::AuthService::Register(m_UserName, m_Password, m_ConfirmPassword, m_NotifationMessage);

			if (success)
			{
				std::memset(m_UserName, 0, sizeof(m_UserName));
				std::memset(m_Password, 0, sizeof(m_Password));
				std::memset(m_ConfirmPassword, 0, sizeof(m_ConfirmPassword));

				ImGui::OpenPopup("Registration Success");
			}
			else
			{
				// Xóa password để người dùng nhập lại
				std::memset(m_Password, 0, sizeof(m_Password));
				std::memset(m_ConfirmPassword, 0, sizeof(m_ConfirmPassword));
			}

		}

		if (!m_NotifationMessage.empty())
		{
			ImGui::Spacing();

			ImGui::TextColored(ImVec4(1.0f, 0.25f, 0.25f, 1.0f), "%s", m_NotifationMessage.c_str());
		}
	}

	if (ImGui::BeginPopupModal("Registration Success", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextColored(ImVec4(0,1,0,1), "%s", m_NotifationMessage.c_str());

		ImGui::Spacing();

		ImGui::Text("Go to Login page?");

		if (ImGui::Button("Yes"))
		{
			// TODO:
			// Move to LoginLayer

			ImGui::OpenPopup("Registration Success->LoGin");
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("Stay"))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
	ImGui::End();
}


void LoginLayer::OnRender()
{
	// Main screen background #1E1E24
	glClearColor(30.0f / 255.0f, 30.0f / 255.0f, 36.0f / 255.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	// Overall container size
	ImVec2 containerSize(520.0f, 500.0f);
	ImVec2 containerPos(viewport->WorkPos.x + (viewport->WorkSize.x - containerSize.x) * 0.5f, viewport->WorkPos.y + (viewport->WorkSize.y - containerSize.y) * 0.5f);

	ImGui::SetNextWindowPos(containerPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(containerSize, ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.0f); // Transparent main window

	ImGuiWindowFlags mainFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
	
	if (ImGui::Begin("RegisterContainer", nullptr, mainFlags))
	{
		ImVec4 cyanColor = ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f); // #00ADB5
		ImVec4 greyText = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
		
		// Header Row: Title and Shield Icon
		ImGui::SetWindowFontScale(1.8f);
		ImGui::PushStyleColor(ImGuiCol_Text, cyanColor);
		ImGui::Text("Group Tracking Tool");
		ImGui::PopStyleColor();
		
		ImGui::SameLine();
		
		// Align Shield Icon to the right
		const char* shieldIcon = ICON_FA_USER_SHIELD;
		float shieldWidth = ImGui::CalcTextSize(shieldIcon).x;
		ImGui::SetCursorPosX(containerSize.x - shieldWidth);
		ImGui::PushStyleColor(ImGuiCol_Text, cyanColor);
		ImGui::Text("%s", shieldIcon);
		ImGui::PopStyleColor();

		// Subtitle
		ImGui::SetWindowFontScale(1.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, greyText);
		ImGui::Text("Version 0.1.0 - Beta");
		ImGui::PopStyleColor();
		
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();

		// Login Box Child Window
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(43.0f / 255.0f, 43.0f / 255.0f, 54.0f / 255.0f, 1.0f)); // #2B2B36
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.2f, 0.2f, 0.25f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
		
		if (ImGui::BeginChild("LoginBox", ImVec2(containerSize.x, 380.0f), true, ImGuiWindowFlags_NoScrollbar))
		{
			// Custom Title Bar for Child Window
			ImVec2 childSize = ImGui::GetWindowSize();
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImVec2 pMin = ImGui::GetWindowPos();
			ImVec2 pMax = ImVec2(pMin.x + childSize.x, pMin.y + 45.0f);
			
			// Draw title bar background #343442
			drawList->AddRectFilled(pMin, pMax, IM_COL32(52, 52, 66, 255), 8.0f, ImDrawFlags_RoundCornersTop);
			// Draw a subtle line under the title bar
			drawList->AddLine(ImVec2(pMin.x, pMax.y), ImVec2(pMax.x, pMax.y), IM_COL32(30, 30, 36, 255), 2.0f);

			// Title Text
			ImGui::SetCursorPos(ImVec2(20.0f, 12.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
			ImGui::Text("Register");
			ImGui::PopStyleColor();

			// Title Dots
			float dotRadius = 5.0f;
			ImVec2 dot1(pMax.x - 40.0f, pMin.y + 22.0f);
			ImVec2 dot2(pMax.x - 20.0f, pMin.y + 22.0f);
			drawList->AddCircleFilled(dot1, dotRadius, IM_COL32(70, 70, 85, 255));
			drawList->AddCircleFilled(dot2, dotRadius, IM_COL32(70, 70, 85, 255));

			ImGui::SetCursorPosY(65.0f);
//sửa
			// Input Fields configuration
			float paddingX = 30.0f;
			float inputWidth = childSize.x - paddingX * 2.0f;

			// Username Label
			ImGui::SetCursorPosX(paddingX);
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
			ImGui::Text(ICON_FA_AT "  USERNAME");
			ImGui::PopStyleColor();

			ImGui::Spacing();

			// Username Input
			ImGui::SetCursorPosX(paddingX);
			ImGui::SetNextItemWidth(inputWidth);
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(28.0f / 255.0f, 28.0f / 255.0f, 36.0f / 255.0f, 1.0f)); // #1C1C24
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 12.0f));
			ImGui::InputText("##userName", m_UserName, sizeof(m_UserName));

			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();

			// Password Label
			ImGui::SetCursorPosX(paddingX);
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
			ImGui::Text(ICON_FA_LOCK "  PASSWORD");
			ImGui::PopStyleColor();

			ImGui::Spacing();

			// Custom background for the entire unified password field
			ImVec2 passMin = ImGui::GetCursorScreenPos();
			passMin.x += paddingX; // account for SetCursorPosX
			ImVec2 passMax = ImVec2(passMin.x + inputWidth, passMin.y + ImGui::GetFrameHeight());
			ImGui::GetWindowDrawList()->AddRectFilled(passMin, passMax, IM_COL32(28, 28, 36, 255), 4.0f);

			float eyeButtonWidth = 40.0f;
			float passwordInputWidth = inputWidth - eyeButtonWidth;

			// Password Input
			ImGui::SetCursorPosX(paddingX);
			ImGui::SetNextItemWidth(passwordInputWidth);
			ImGuiInputTextFlags passwordFlags = m_ShowPassword ? 0 : ImGuiInputTextFlags_Password;
			
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0)); // Transparent FrameBg
			ImGui::InputText("##password", m_Password, sizeof(m_Password), passwordFlags);
			ImGui::PopStyleColor();

			
			// Confirm password
			ImGui::SetCursorPosX(paddingX);
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
			ImGui::Text(ICON_FA_AT "  PASSWORD CONFIRMATION");
			ImGui::PopStyleColor();

			ImGui::Spacing();

			// Confirm Password Input
			ImGui::SetCursorPosX(paddingX);
			ImGui::SetNextItemWidth(passwordInputWidth);
			ImGuiInputTextFlags passwordFlags = m_ShowPassword ? 0 : ImGuiInputTextFlags_Password;
			
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0)); // Transparent FrameBg
			ImGui::InputText("##passwordconfirmation", m_Password, sizeof(m_Password), passwordFlags);
			ImGui::PopStyleColor();


			// Eye Button
			ImGui::SameLine(0, 0); 
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0)); 
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.05f)); 
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.1f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));

			const char* eyeIcon = m_ShowPassword ? ICON_FA_EYE_SLASH "##eyebtn" : ICON_FA_EYE "##eyebtn";
			if (ImGui::Button(eyeIcon, ImVec2(eyeButtonWidth, ImGui::GetFrameHeight())))
			{
				m_ShowPassword = !m_ShowPassword;
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			}

			ImGui::PopStyleColor(4);

			ImGui::PopStyleVar(2); // FrameRounding, FramePadding
			ImGui::PopStyleColor(); // FrameBg

			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();

			// Login Button
			ImGui::SetCursorPosX(paddingX);
			ImGui::PushStyleColor(ImGuiCol_Button, cyanColor); 
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 193.0f / 255.0f, 201.0f / 255.0f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 153.0f / 255.0f, 161.0f / 255.0f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(30.0f / 255.0f, 30.0f / 255.0f, 36.0f / 255.0f, 1.0f)); // Dark text for button
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

			if (ImGui::Button(ICON_FA_RIGHT_TO_BRACKET "  Login", ImVec2(inputWidth, 45.0f)))
			{
				// TODO: Implement login logic
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			}
			
			ImGui::PopStyleVar();
			ImGui::PopStyleColor(4);

			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();

			// Sign up text
			const char* text1 = "Don't have an account? ";
			const char* text2 = "Sign up";
			float textWidth = ImGui::CalcTextSize(text1).x + ImGui::CalcTextSize(text2).x;
			ImGui::SetCursorPosX((childSize.x - textWidth) * 0.5f);

			ImGui::PushStyleColor(ImGuiCol_Text, greyText);
			ImGui::Text("%s", text1);
			ImGui::PopStyleColor();
			
			ImGui::SameLine(0, 0);

			ImGui::PushStyleColor(ImGuiCol_Text, cyanColor);
			if (ImGui::Selectable(text2, false, 0, ImGui::CalcTextSize(text2)))
			{
				TrackingTool::Application::Get().PushLayer<RegisterLayer>();
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			}
			ImGui::PopStyleColor();
		}
		ImGui::EndChild();
		
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(2);
	}
	ImGui::End();

	ImGui::PopStyleVar();
	ImGui::PopStyleColor(2);
}