#pragma once

#include "Platform/Layer.h"
#include "LoginView.h"
#include "RegisterView.h"

enum class AuthState { Login, Register };

class AuthenticationLayer : public TrackingTool::Layer
{
public:
	AuthenticationLayer() = default;
	virtual ~AuthenticationLayer() = default;

	virtual void OnUpdate(float ts) override;
	virtual void OnRender() override;

private:
	AuthState m_CurrentState = AuthState::Login;

	LoginView m_LoginView;
	RegisterView m_RegisterView;
};
