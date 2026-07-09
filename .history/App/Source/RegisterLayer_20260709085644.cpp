#include <glad/gl.h>

#include "RegisterLayer.h"

#include "imgui.h"

#include "Platform/Application.h"
#include "Service/AuthService.h"

#include <cstring>
#include <string>
#include "IconsFontAwesome6.h"

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
    // Background
    glClearColor(30.0f / 255.0f, 30.0f / 255.0f, 36.0f / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    // Register window lớn hơn Login vì có thêm Confirm Password
    ImVec2 containerSize(520.0f, 600.0f);
    ImVec2 containerPos(viewport->WorkPos.x + (viewport->WorkSize.x - containerSize.x) * 0.5f, viewport->WorkPos.y + (viewport->WorkSize.y - containerSize.y) * 0.5f);

    ImGui::SetNextWindowPos(containerPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(containerSize, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);

    ImGuiWindowFlags mainFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));

    if (ImGui::Begin("RegisterContainer", nullptr, mainFlags))
    {
        ImVec4 cyanColor(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f);

        ImVec4 greyText(0.7f, 0.7f, 0.7f, 1.0f);


        ImGui::SetWindowFontScale(1.8f);

        ImGui::PushStyleColor(ImGuiCol_Text, cyanColor);
        ImGui::Text("Group Tracking Tool");
        ImGui::PopStyleColor();

        ImGui::SameLine();

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

        // REGISTER BOX
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(43.0f / 255.0f, 43.0f / 255.0f, 54.0f / 255.0f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.2f, 0.2f, 0.25f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

        if (ImGui::BeginChild("RegisterBox", ImVec2(containerSize.x, .0f), true, ImGuiWindowFlags_NoScrollbar))
        {
			// Custom Title Bar for Child Window
            ImVec2 childSize = ImGui::GetWindowSize();
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 pMin = ImGui::GetWindowPos();
            ImVec2 pMax(pMin.x + childSize.x, pMin.y + 45.0f);

            // TITLE BAR
            drawList->AddRectFilled(pMin, pMax, IM_COL32(52, 52, 66, 255), 8.0f, ImDrawFlags_RoundCornersTop);
            drawList->AddLine(ImVec2(pMin.x, pMax.y), ImVec2(pMax.x, pMax.y), IM_COL32(30, 30, 36, 255), 2.0f);

			//TITLE TEXT
            ImGui::SetCursorPos(ImVec2(20.0f, 12.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,1,1,1));
            ImGui::Text("Register");
            ImGui::PopStyleColor();

			// Title Dots
            float dotRadius = 5.0f;
            drawList->AddCircleFilled(ImVec2(pMax.x - 40.0f, pMin.y + 22.0f), dotRadius, IM_COL32(70,70,85,255));
            drawList->AddCircleFilled(ImVec2(pMax.x - 20.0f, pMin.y + 22.0f), dotRadius, IM_COL32(70,70,85,255));

            // INPUTS FIELD
            ImGui::SetCursorPosY(65.0f);

            float paddingX = 30.0f;
            float inputWidth = childSize.x - paddingX * 2.0f;

            // USERNAME
            ImGui::SetCursorPosX(paddingX);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f,0.8f,0.8f,1));
            ImGui::Text(ICON_FA_AT "  USERNAME");
            ImGui::PopStyleColor();

            ImGui::Spacing();

			//USERNAME INPUT
            ImGui::SetCursorPosX(paddingX);
            ImGui::SetNextItemWidth(inputWidth);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(28.0f/255.0f, 28.0f/255.0f, 36.0f/255.0f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f,12.0f));
            ImGui::InputText("##userName", m_UserName,sizeof(m_UserName));

            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Spacing();

            // PASSWORD LABEL
            ImGui::SetCursorPosX(paddingX);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f,0.8f,0.8f,1));
            ImGui::Text(ICON_FA_LOCK "  PASSWORD");
            ImGui::PopStyleColor();

            ImGui::Spacing();

            ImVec2 passMin = ImGui::GetCursorScreenPos();
            passMin.x += paddingX;
            ImVec2 passMax(passMin.x + inputWidth, passMin.y + ImGui::GetFrameHeight());
            drawList->AddRectFilled(passMin, passMax, IM_COL32(28,28,36,255), 4.0f);

            float eyeWidth = 40.0f;
            float passwordWidth = inputWidth - eyeWidth;

			//PASSWORD INPUT
            ImGui::SetCursorPosX(paddingX);
            ImGui::SetNextItemWidth(passwordWidth);
            ImGuiInputTextFlags passwordFlags = m_ShowPassword ? 0 : ImGuiInputTextFlags_Password;
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0,0,0,0));
            ImGui::InputText("##password", m_Password, sizeof(m_Password), passwordFlags);
            ImGui::PopStyleColor();

            // EYE BUTTON
            ImGui::SameLine(0,0);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1,1,1,0.05f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1,1,1,0.10f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f,0.6f,0.6f,1));

            const char* eyeIcon = m_ShowPassword ? ICON_FA_EYE_SLASH "##eyePassword" : ICON_FA_EYE "##eyePassword";

            if (ImGui::Button(eyeIcon, ImVec2(eyeWidth, ImGui::GetFrameHeight())))
            {
                m_ShowPassword = !m_ShowPassword;
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }

            ImGui::PopStyleColor(4);

            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor();

            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Spacing();

            // CONFIRM PASSWORD
            ImGui::SetCursorPosX(paddingX);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f,0.8f,0.8f,1));
            ImGui::Text(ICON_FA_LOCK "  PASSWORD CONFIRMATION");
            ImGui::PopStyleColor();

            ImGui::Spacing();

            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 12.0f));

            ImVec2 confirmMin = ImGui::GetCursorScreenPos();
            confirmMin.x += paddingX;
            ImVec2 confirmMax(confirmMin.x + inputWidth, confirmMin.y + ImGui::GetFrameHeight());
            drawList->AddRectFilled(confirmMin, confirmMax, IM_COL32(28,28,36,255), 4.0f);
            ImGui::SetCursorPosX(paddingX);
            ImGui::SetNextItemWidth(passwordWidth);
           
            

            ImGuiInputTextFlags confirmFlags = m_ShowConfirmPassword ? 0 : ImGuiInputTextFlags_Password;
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0,0,0,0));
            ImGui::InputText("##confirmPassword", m_ConfirmPassword, sizeof(m_ConfirmPassword), confirmFlags);
            ImGui::PopStyleColor();

            // CONFIRM EYE

            ImGui::SameLine(0,0);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1,1,1,0.05f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1,1,1,0.10f));

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f,0.6f,0.6f,1));

            const char* confirmEyeIcon = m_ShowConfirmPassword ? ICON_FA_EYE_SLASH "##eyeConfirm" : ICON_FA_EYE "##eyeConfirm";

            if(ImGui::Button(confirmEyeIcon, ImVec2(eyeWidth, ImGui::GetFrameHeight())))
            {
                m_ShowConfirmPassword =
                !m_ShowConfirmPassword;
            }

            if(ImGui::IsItemHovered())
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }

            ImGui::PopStyleColor(4);

            // Restore Style
            ImGui::PopStyleVar(2);

            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Spacing();

            // REGISTER BUTTON

            ImGui::SetCursorPosX(paddingX);
            ImGui::PushStyleColor(ImGuiCol_Button, cyanColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 193.0f/255.0f, 201.0f/255.0f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 153.0f/255.0f, 161.0f/255.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(30.0f/255.0f, 30.0f/255.0f, 36.0f/255.0f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

            if(ImGui::Button( ICON_FA_USER_PLUS "  Register", ImVec2(inputWidth,45.0f)))
            {
                m_NotifationMessage.clear();

                bool success = TrackingTool::AuthService::Register(m_UserName, m_Password, m_ConfirmPassword, m_NotifationMessage);

                if(success)
                {
                    std::memset(m_UserName, 0, sizeof(m_UserName));

                    std::memset(m_Password, 0, sizeof(m_Password));

                    std::memset(m_ConfirmPassword, 0, sizeof(m_ConfirmPassword));

                    ImGui::OpenPopup("Registration Success");
                }
                else
                {
                    std::memset(m_Password, 0, sizeof(m_Password));

                    std::memset(m_ConfirmPassword, 0, sizeof(m_ConfirmPassword));
                }
            }

            if(ImGui::IsItemHovered())
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }

            ImGui::PopStyleVar();

            ImGui::PopStyleColor(4);

            // NOTIFICATION

            if(!m_NotifationMessage.empty())
            {
                ImGui::Spacing();
                ImGui::PushTextWrapPos(paddingX + inputWidth);
                ImGui::SetCursorPosX(paddingX);
                ImGui::TextColored(ImVec4(1.0f, 0.25f, 0.25f, 1.0f), "%s", m_NotifationMessage.c_str());
                ImGui::PopTextWrapPos();
            }

            ImGui::Spacing();
            ImGui::Spacing();

            // LOGIN LINK

            const char* text1 = "Already have an account? ";

            const char* text2 = "Login";

            float textWidth = ImGui::CalcTextSize(text1).x + ImGui::CalcTextSize(text2).x;

            ImGui::SetCursorPosX((childSize.x - textWidth) * 0.5f);
            ImGui::PushStyleColor(ImGuiCol_Text, greyText);
            ImGui::Text("%s", text1);
            ImGui::PopStyleColor();
            ImGui::SameLine(0,0);
            ImGui::PushStyleColor(ImGuiCol_Text, cyanColor);

            if(ImGui::Selectable(text2, false, 0, ImGui::CalcTextSize(text2)))
            {
                TrackingTool::Application::Get().PopLayer();
                // hoặc PushLayer<LoginLayer>()
            }

            if(ImGui::IsItemHovered())
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
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();

    // SUCCESS POPUP

    if (ImGui::BeginPopupModal("Registration Success", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.9f, 0.4f, 1.0f));
        ImGui::Text("%s", m_NotifationMessage.c_str());
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text("Go to Login page?");

        ImGui::Spacing();

        float buttonWidth = 90.0f;

        if (ImGui::Button("Yes", ImVec2(buttonWidth, 35)))
        {
            ImGui::CloseCurrentPopup();

            //--------------------------------------------------
            // TODO:
            // chuyển sang LoginLayer
            //--------------------------------------------------

            TrackingTool::Application::Get().PopLayer();

            // Nếu project của bạn không dùng PopLayer()
            // thì đổi thành:
            //
            // TrackingTool::Application::Get()
            //      .PushLayer<LoginLayer>();
        }

        ImGui::SameLine();

        if (ImGui::Button("Stay", ImVec2(buttonWidth, 35)))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}