#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "Database/Database.h"

class ChartView
{
public:
	ChartView() = default;
	~ChartView() = default;

	// projectId loads milestones + tasks for the Gantt chart.
	void OnRender(const char* projectName, const char* createdDate, int projectId);

private:
	// Reloads only when project or service cache generation changes (no per-frame heap work).
	void EnsureDataLoaded(int projectId);
	// Rebuilds date range + day columns only after data reloads.
	void RebuildTimeline();

	std::vector<TrackingTool::MilestoneInfo> m_Milestones;
	std::vector<TrackingTool::TaskInfo> m_Tasks;
	// Reused across loads so EnsureDataLoaded never heap-allocates per frame.
	std::vector<TrackingTool::MilestoneInfo> m_MilestoneScratch;
	std::vector<TrackingTool::TaskInfo> m_TaskScratch;
	std::string m_LoadMessage;

	int m_LoadedProjectId = 0;
	int m_LoadedMilestonesCacheGeneration = -1;
	int m_LoadedTasksCacheGeneration = -1;
	bool m_HasLoaded = false;
	bool m_NotifiedLoadError = false;
	bool m_TimelineDirty = true;

	// Timeline (rebuilt only when m_TimelineDirty).
	char m_RangeStart[11] = "";
	char m_RangeEnd[11] = "";
	int m_DayCount = 0;

	struct DayColumn
	{
		char Date[11] = "";   // MM-DD-YYYY
		char Header[16] = ""; // "MON\n12 OCT"
	};
	std::vector<DayColumn> m_DayColumns; // capacity retained across rebuilds

	// Milestone groups that are currently collapsed (missing = expanded).
	std::unordered_set<int> m_CollapsedMilestoneIds;
};
