#pragma once

#include <glad/gl.h>
#include "Platform/Layer.h"

class LoginLayer : public TrackingTool::Layer
{
public:
	LoginLayer() = default;
	virtual ~LoginLayer() = default;

	virtual void OnUpdate(float ts) override;
	virtual void OnRender() override;

private:
	char m_UserName[256] = "";
	char m_Password[256] = "";
	
	std::string m_NotificationMessage;

	bool m_ShowPassword = false;
};
