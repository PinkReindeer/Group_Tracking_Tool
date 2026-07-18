#pragma once

#include <string>
#include "imgui.h"

class RegisterView
{
public:
	RegisterView() = default;
	~RegisterView() = default;

	void OnUpdate(float ts);
	bool Render(const ImVec2& containerSize, bool& outWantToSwitch);

private:
	char m_UserName[256] = {};
	char m_Password[256] = {};
	char m_ConfirmPassword[256] = {};

	bool m_ShowPassword = false;
	bool m_ShowConfirmPassword = false;
	
	std::string m_NotificationMessage;
};
