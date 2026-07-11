#pragma once

#include <string>
#include "imgui.h"

class LoginView
{
public:
	LoginView() = default;
	~LoginView() = default;

	void OnUpdate(float ts);
	bool Render(const ImVec2& containerSize, bool& outWantToSwitch);

private:
	char m_UserName[256] = "";
	char m_Password[256] = "";
	
	std::string m_NotificationMessage;

	bool m_ShowPassword = false;
};
