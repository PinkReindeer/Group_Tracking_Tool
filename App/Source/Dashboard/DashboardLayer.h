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

	// Counts the logged-in user's tasks across m_Projects (pending / in progress / overdue).
	// Call after m_Projects is up to date. forceRefresh bypasses the tasks session cache.
	void RefreshTaskStats(bool forceRefresh = false);

	void RenderEditProjectModal();
	void RenderDeleteProjectModal();

	std::vector<TrackingTool::ProjectInfo> m_Projects;
	int m_PendingTaskCount = 0;
	int m_InProgressTaskCount = 0;
	int m_OverdueTaskCount = 0;

	bool m_ShowCreateProjectModal = false;
	char m_NewProjectName[128] = "";
	char m_NewProjectDescription[512] = "";
	char m_JoinProjectCode[32] = "";

	// Project card overflow menu (three-dot) → Edit / Delete
	bool m_OpenEditProject = false;
	bool m_OpenDeleteProject = false;
	int m_ActionProjectId = 0;
	std::string m_ActionProjectName;
	char m_EditProjectName[128] = "";
	char m_EditProjectDescription[512] = "";
};
