#include "imgui.h"
#include "IconsFontAwesome6.h"

#include "NotificationLayer.h"

void NotificationLayer::OnUpdate(float ts)
{
    if (m_NotifyTimer > 0.0f)
    {
        m_NotifyTimer -= ts;
        if (m_NotifyTimer < 0.0f)
            m_NotifyTimer = 0.0f;
    }

    // when current notification ended, start next if available
    if (m_NotifyTimer <= 0.0f && !m_Queue.empty())
    {
        StartNextNotification();
    }
}

void NotificationLayer::OnRender()
{
    if (m_NotifyTimer > 0.0f)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        float startY = viewport->WorkPos.y - 100.0f;
        float targetY = viewport->WorkPos.y + 30.0f;
        
        float progress = 1.0f;
        float animDuration = 0.3f; // 0.3s for slide animation
        
        // Slide in
        if (m_NotifyDuration - m_NotifyTimer < animDuration)
        {
            progress = (m_NotifyDuration - m_NotifyTimer) / animDuration;
            // Easing function (easeOutQuad)
            progress = 1.0f - (1.0f - progress) * (1.0f - progress);
        }
        // Slide out
        else if (m_NotifyTimer < animDuration)
        {
            progress = m_NotifyTimer / animDuration;
            // Easing function (easeInQuad)
            progress = progress * progress;
        }
        
        float currentY = startY + (targetY - startY) * progress;

        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x * 0.5f, currentY), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
        ImGui::SetNextWindowBgAlpha(0.9f);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |  ImGuiWindowFlags_AlwaysAutoResize | 
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

        ImU32 bgColor = IM_COL32(0, 0, 0, 0);
        const char* icon = nullptr;

        switch (m_CurrentType)
        {
        case NotificationType::Warning:
            bgColor = IM_COL32(200, 150, 0, 255);
            icon = ICON_FA_TRIANGLE_EXCLAMATION;
            break;
        case NotificationType::Error:
            bgColor = IM_COL32(180, 50, 50, 255);
            icon = ICON_FA_CIRCLE_XMARK;
            break;
        case NotificationType::Info:
        default:
            bgColor = IM_COL32(50, 150, 50, 255);
            icon = ICON_FA_CIRCLE_INFO;
            break;
        }

        ImU32 textColor = IM_COL32(255, 255, 255, 255);
        ImU32 borderColor = IM_COL32(255, 255, 255, 50);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, bgColor);
        ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
        ImGui::PushStyleColor(ImGuiCol_Text, textColor);
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 12.0f));

        if (ImGui::Begin("Notification", nullptr, flags))
        {
            ImGui::Text("%s  %s", icon, m_Message.c_str());
        }
        ImGui::End();

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(3);
    }
}

void NotificationLayer::StartNextNotification()
{
    if (m_Queue.empty())
        return;

    auto next = m_Queue.front();
    m_Queue.pop_front();

    m_Message = next.Message;
    m_CurrentType = next.Type;
    m_NotifyTimer = next.Duration;
    m_NotifyDuration = next.Duration;
}

void NotificationLayer::ShowNotification(const std::string& message , NotificationType type)
{
    m_Queue.push_back({ message, type, 3.0f });
}