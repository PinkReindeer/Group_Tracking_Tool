#pragma once

#include <string>
#include <vector>

#include "Database/Database.h"

class MilestonesView
{
public:
	MilestonesView() = default;
	~MilestonesView() = default;

	// projectId is required to load/create milestones. isLeader gates create/edit/delete.
	void OnRender(const char* projectName, const char* createdDate, int projectId, bool isLeader);

private:
	void EnsureMilestonesLoaded(int projectId, bool forceRefresh = false);
	void OpenEditForMilestone(const TrackingTool::MilestoneInfo& milestone);
	void RenderCreateMilestoneModal(int projectId);
	void RenderEditMilestoneModal(int projectId);
	void RenderDeleteMilestoneModal(int projectId);

	std::vector<TrackingTool::MilestoneInfo> m_Milestones;
	// Reused across loads so EnsureMilestonesLoaded never heap-allocates per frame.
	std::vector<TrackingTool::MilestoneInfo> m_LoadScratch;
	std::string m_LoadMessage;
	int m_LoadedProjectId = 0;
	int m_LoadedCacheGeneration = -1;
	bool m_HasLoaded = false;

	char m_NewMilestoneName[128] = "";
	char m_NewStartDate[16] = "";
	char m_NewEndDate[16] = "";

	// Edit form (shared buffers with create layout style)
	int m_EditMilestoneId = 0;
	char m_EditMilestoneName[128] = "";
	char m_EditStartDate[16] = "";
	char m_EditEndDate[16] = "";
	bool m_OpenEditModal = false;

	// Delete confirmation
	int m_PendingDeleteMilestoneId = 0;
	std::string m_PendingDeleteMilestoneName;
	bool m_OpenDeleteModal = false;
};
