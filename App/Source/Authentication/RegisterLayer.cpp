#include <glad/gl.h>

#include "imgui.h"
#include "IconsFontAwesome6.h"

#include <cstring>
#include <string>

#include "LoginLayer.h"
#include "Platform/Application.h"
#include "Service/AuthService.h"
#include "RegisterLayer.h"

void RegisterLayer::OnUpdate(float ts)
{
    
}

void RegisterLayer::OnRender()
{
    // Background
    glClearColor(30.0f / 255.0f, 30.0f / 255.0f, 36.0f / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImVec2 containerSize(420.0f, 468.0f);
    ImVec2 containerPos(viewport->WorkPos.x + (viewport->WorkSize.x - containerSize.x) * 0.5f, viewport->WorkPos.y + (viewport->WorkSize.y - containerSize.y) * 0.5f);

    ImGui::SetNextWindowPos(containerPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(containerSize, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);

    ImGuiWindowFlags mainFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));

    if (ImGui::Begin("RegisterContainer", nullptr, mainFlags))
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

        // Register box
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(37.0f / 255.0f, 37.0f / 255.0f, 45.0f / 255.0f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(57.0f / 255.0f, 62.0f / 255.0f, 70.0f / 255.0f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

        if (ImGui::BeginChild("RegisterBox", ImVec2(containerSize.x, 404.0f), true, ImGuiWindowFlags_NoScrollbar))
        {
			// Custom Title Bar for Child Window
            ImVec2 childSize = ImGui::GetWindowSize();
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 pMin = ImGui::GetWindowPos();
            ImVec2 pMax = ImVec2(pMin.x + childSize.x, pMin.y + 32.0f);

            // Draw title bar background
            drawList->AddRectFilled(pMin, pMax, IM_COL32(51, 53, 53, 255), 4.0f, ImDrawFlags_RoundCornersTop);
            // Draw a subtle line under the title bar
            drawList->AddLine(ImVec2(pMin.x, pMax.y), ImVec2(pMax.x, pMax.y), IM_COL32(60, 73, 74, 255), 1.0f);

            // Title Text
            ImGui::SetCursorPos(ImVec2(16.0f, 8.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f));
            ImGui::Text("Register");
            ImGui::PopStyleColor();

			// Title Dots
            float dotRadius = 4.0f;
            ImVec2 dot2(pMax.x - 20.0f, pMin.y + 16.0f);
            ImVec2 dot1(pMax.x - 36.0f, pMin.y + 16.0f);
            drawList->AddCircleFilled(dot1, dotRadius, IM_COL32(70, 70, 85, 255));
            drawList->AddCircleFilled(dot2, dotRadius, IM_COL32(70, 70, 85, 255));

            ImGui::SetCursorPosY(50.0f);

            // Input Fields configuration
            float paddingX = 24.0f;
            float inputWidth = childSize.x - paddingX * 2.0f;

            // Username Label
            ImGui::SetCursorPosX(paddingX);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f));
            ImGui::Text(ICON_FA_AT "  USERNAME");
            ImGui::PopStyleColor();

            ImGui::Dummy(ImVec2(0.0f, 6.0f));

            // Username Input
            ImGui::SetCursorPosX(paddingX);
            ImGui::SetNextItemWidth(inputWidth);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(30.0f / 255.0f, 30.0f / 255.0f, 36.0f / 255.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(57.0f / 255.0f, 62.0f / 255.0f, 70.0f / 255.0f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 8.0f));
            ImGui::InputText("##userName", m_UserName, sizeof(m_UserName));
            ImGui::PopStyleVar(3);
            ImGui::PopStyleColor(2);

            ImGui::Dummy(ImVec2(0.0f, 10.0f));

            // Password Label
            ImGui::SetCursorPosX(paddingX);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f));
            ImGui::Text(ICON_FA_LOCK "  PASSWORD");
            ImGui::PopStyleColor();

            ImGui::Dummy(ImVec2(0.0f, 6.0f));

            // Custom background for the entire unified password field
            ImVec2 passMin = ImGui::GetCursorScreenPos();
            passMin.x += paddingX;
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 8.0f));
            ImVec2 passMax = ImVec2(passMin.x + inputWidth, passMin.y + ImGui::GetFrameHeight());
            ImGui::GetWindowDrawList()->AddRectFilled(passMin, passMax, IM_COL32(30, 30, 36, 255), 2.0f);
            ImGui::GetWindowDrawList()->AddRect(passMin, passMax, IM_COL32(57, 62, 70, 255), 2.0f, 0, 1.0f);

            float eyeButtonWidth = 40.0f;
            float passwordInputWidth = inputWidth - eyeButtonWidth;

            // Password Input
            ImGui::SetCursorPosX(paddingX);
            ImGui::SetNextItemWidth(passwordInputWidth);
            ImGuiInputTextFlags passwordFlags = m_ShowPassword ? 0 : ImGuiInputTextFlags_Password;

            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
            ImGui::InputText("##password", m_Password, sizeof(m_Password), passwordFlags);
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
                m_ShowConfirmPassword = m_ShowPassword;
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }

            ImGui::PopStyleColor(4);
            ImGui::PopStyleVar();

            ImGui::Dummy(ImVec2(0.0f, 10.0f));

            // Confirm password
            ImGui::SetCursorPosX(paddingX);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f));
            ImGui::Text(ICON_FA_LOCK "  PASSWORD CONFIRMATION");
            ImGui::PopStyleColor();

            ImGui::Dummy(ImVec2(0.0f, 6.0f));

            ImVec2 confirmMin = ImGui::GetCursorScreenPos();
            confirmMin.x += paddingX;
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 8.0f));
            ImVec2 confirmMax = ImVec2(confirmMin.x + inputWidth, confirmMin.y + ImGui::GetFrameHeight());
            ImGui::GetWindowDrawList()->AddRectFilled(confirmMin, confirmMax, IM_COL32(30, 30, 36, 255), 2.0f);
            ImGui::GetWindowDrawList()->AddRect(confirmMin, confirmMax, IM_COL32(57, 62, 70, 255), 2.0f, 0, 1.0f);
           
            ImGui::SetCursorPosX(paddingX);
            ImGui::SetNextItemWidth(passwordInputWidth);
            ImGuiInputTextFlags confirmFlags = m_ShowConfirmPassword ? 0 : ImGuiInputTextFlags_Password;

            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
            ImGui::InputText("##confirmPassword", m_ConfirmPassword, sizeof(m_ConfirmPassword), confirmFlags);
            ImGui::PopStyleColor();

            // Confirm Eye Button
            ImGui::SameLine(0, 0);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.05f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.1f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));

            const char* confirmEyeIcon = m_ShowConfirmPassword ? ICON_FA_EYE_SLASH "##eyeConfirm" : ICON_FA_EYE "##eyeConfirm";
            if (ImGui::Button(confirmEyeIcon, ImVec2(eyeButtonWidth, ImGui::GetFrameHeight())))
            {
                m_ShowConfirmPassword = !m_ShowConfirmPassword;
                m_ShowPassword = m_ShowConfirmPassword;
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }

            ImGui::PopStyleColor(4);
            ImGui::PopStyleVar();

            ImGui::Dummy(ImVec2(0.0f, 20.0f));

            // Register Button
            ImGui::SetCursorPosX(paddingX);
            ImGui::PushStyleColor(ImGuiCol_Button, cyanColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 193.0f / 255.0f, 201.0f / 255.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 153.0f / 255.0f, 161.0f / 255.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(30.0f / 255.0f, 30.0f / 255.0f, 36.0f / 255.0f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);

            bool doRegister = false;

            if (ImGui::Button(ICON_FA_USER_PLUS "  Register", ImVec2(inputWidth, 40.0f)))
            {
                doRegister = true;
            }

            if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
            {
                doRegister = true;
            }

            if (doRegister) 
            {
                if (TrackingTool::AuthService::Register(m_UserName, m_Password, m_ConfirmPassword, m_NotificationMessage))
                {
                    TransitionTo<LoginLayer>();
                    TrackingTool::Application::Get().PushNotification(m_NotificationMessage, NotificationType::Info);
                }
                else
                {
                    TrackingTool::Application::Get().PushNotification(m_NotificationMessage, NotificationType::Error);
                }
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }

            ImGui::PopStyleVar();
            ImGui::PopStyleColor(4);

            ImGui::Dummy(ImVec2(0.0f, 8.0f));

            // Register text
            const char* text1 = "Already have an account? ";
            const char* text2 = "Login";
            float textWidth = ImGui::CalcTextSize(text1).x + ImGui::CalcTextSize(text2).x;
            ImGui::SetCursorPosX((childSize.x - textWidth) * 0.5f);

            ImVec4 footerText = ImVec4(134.0f / 255.0f, 147.0f / 255.0f, 148.0f / 255.0f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, footerText);
            ImGui::Text("%s", text1);
            ImGui::PopStyleColor();

            ImGui::SameLine(0,0);

            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Text, cyanColor);
            if (ImGui::Selectable(text2, false, 0, ImGui::CalcTextSize(text2)))
            {
                TransitionTo<LoginLayer>();
            }
            if(ImGui::IsItemHovered())
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }
            ImGui::PopStyleColor(3);
		}
        ImGui::EndChild();

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
    }

    ImGui::End();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}