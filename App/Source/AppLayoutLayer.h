#pragma once

#include "Platform/Layer.h"

class AppLayoutLayer : public TrackingTool::Layer
{
public:
	AppLayoutLayer() = default;
	~AppLayoutLayer() = default;

	virtual void OnRender() override final;

protected:
	virtual void OnRenderContent() = 0;
	// Hook to allow derived layers (like ProjectLayer) to add specific UI elements (like tabs) to the top navigation bar.
	virtual void OnRenderTopNavBarExtensions() {}

	virtual const char* GetActiveSidebarMenu() const { return "Dashboard"; }
	virtual const char* GetTopNavBarTitle() const { return "Personal Dashboard"; }

private:
	void RenderSideNavBar();
	void RenderTopNavBar();
	void RenderLogoutConfirmModal();

	// Set when the user chooses Logout from the gear menu; opens the confirm modal next frame.
	bool m_OpenLogoutConfirm = false;
};