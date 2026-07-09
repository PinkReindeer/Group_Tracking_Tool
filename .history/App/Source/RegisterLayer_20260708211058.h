#pragma once

#include "Platform/Layer.h"
#include <string>

class RegisterLayer : public TrackingTool::Layer
{
public:
	RegisterLayer();
	virtual ~RegisterLayer();

	virtual void OnUpdate(float ts) override;
	virtual void OnRender() override;

private:
	char m_UserName[128] = {};
	char m_Password[128] = {};
	char m_ConfirmPassword[128] = {};
	bool m_ShowPassword = false;
	
	std::string m_NotifationMessage;
};