#pragma once

#include <glad/gl.h>
#include <string>
#include <vector>

#include "AppLayoutLayer.h"
#include "Database/Database.h"

class DashboardLayer : public AppLayoutLayer
{
public:
	DashboardLayer();
	virtual ~DashboardLayer() = default;

	virtual void OnUpdate(float ts) override;

protected:
	virtual void OnRenderContent() override;

	virtual const char* GetActiveSidebarMenu() const override { return "Dashboard"; }
	virtual const char* GetTopNavBarTitle() const override { return "Personal Dashboard"; }

private:
	// forceRefresh: bypass ProjectService session cache and hit the DB.
	// showNotification: toast success/error (used by Live Refresh).
	void RefreshProjects(bool forceRefresh = false, bool showNotification = false);

	std::vector<TrackingTool::ProjectInfo> m_Projects;
	bool m_ShowCreateProjectModal = false;
	char m_NewProjectName[128] = "";
	char m_NewProjectDescription[512] = "";
	char m_JoinProjectCode[32] = "";
};
