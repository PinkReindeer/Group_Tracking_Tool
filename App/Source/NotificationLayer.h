#pragma once

#include <string>
#include <deque>

#include <glad/gl.h>

#include "Platform/Layer.h"

enum class NotificationType
{
	Info,
	Warning,
	Error
};

class NotificationLayer : public TrackingTool::Layer
{
public:
	NotificationLayer() = default;
	virtual ~NotificationLayer() = default;

	virtual void OnUpdate(float ts) override;
	virtual void OnRender() override;

	void ShowNotification(const std::string& message, NotificationType type);

private:
	struct Notification
	{
		std::string Message;
		NotificationType Type = NotificationType::Info;
		float Duration = 2.0f;
	};

	float m_NotifyTimer = 0.0f;
	float m_NotifyDuration = 0.0f;

private:
	std::string m_Message;
	NotificationType m_CurrentType = NotificationType::Info;
	std::deque<Notification> m_Queue;

	void StartNextNotification();
};
