#pragma once

#include "Platform/Layer.h"
#include <string>

class RegisterLayer : public TrackingTool::Layer
{
public:
	RegisterLayer() = default;
	virtual ~RegisterLayer() = default;

	virtual void OnUpdate(float ts) override;
	virtual void OnRender() override;

private:
	char m_UserName[256] = {};
	char m_Password[256] = {};
	char m_ConfirmPassword[256] = {};

	bool m_ShowPassword = false;
	bool m_ShowConfirmPassword = false;
	
	std::string m_NotificationMessage;
};