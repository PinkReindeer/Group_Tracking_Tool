#pragma once

#include <string>
#include <vector>

#include "Database/Database.h"

class MilestonesView
{
public:
	MilestonesView() = default;
	~MilestonesView() = default;

	// projectId is required to load/create milestones. isLeader gates the create button.
	void OnRender(const char* projectName, const char* createdDate, int projectId, bool isLeader);

private:
	void EnsureMilestonesLoaded(int projectId, bool forceRefresh = false);
	void RenderCreateMilestoneModal(int projectId);

	std::vector<TrackingTool::MilestoneInfo> m_Milestones;
	int m_LoadedProjectId = 0;
	bool m_HasLoaded = false;

	char m_NewMilestoneName[128] = "";
	char m_NewStartDate[16] = "";
	char m_NewEndDate[16] = "";
};
