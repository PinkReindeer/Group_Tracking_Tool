#pragma once

#include <string>
#include <vector>

#include "Database/Database.h"

class WorkloadView
{
public:
	WorkloadView() = default;
	~WorkloadView() = default;

	// projectId loads members + tasks and builds per-member workload from estimated hours.
	void OnRender(const char* projectName, const char* createdDate, int projectId);

private:
	// Reloads only when project or task cache generation changes (no per-frame heap work).
	void EnsureDataLoaded(int projectId);
	// Aggregates task estimated hours / status counts per member.
	void RebuildWorkload();

	struct MemberWorkload
	{
		int MembershipId = 0;
		std::string Name;
		std::string Role;

		int PendingCount = 0;
		int InProgressCount = 0; // "in progress" + "under review"
		int DoneCount = 0;

		float PendingHours = 0.0f;
		float InProgressHours = 0.0f;
		float DoneHours = 0.0f;

		// Convenience totals derived in RebuildWorkload.
		int ActiveTaskCount = 0;   // pending + in progress (+ under review)
		int TotalTaskCount = 0;    // pending + in progress + done
		float RemainingHours = 0.0f; // pending + in progress hours (open load)
		float TotalHours = 0.0f;     // all assigned estimated hours
		float Progress = 0.0f;       // done hours / total hours (0..1), count fallback if no hours

		// "BALANCED" | "HEAVY" | "LIGHT" | "IDLE"
		const char* BalanceLabel = "IDLE";
	};

	std::vector<TrackingTool::MemberInfo> m_Members;
	std::vector<TrackingTool::TaskInfo> m_Tasks;
	// Reused across loads so EnsureDataLoaded never heap-allocates per frame.
	std::vector<TrackingTool::MemberInfo> m_MemberScratch;
	std::vector<TrackingTool::TaskInfo> m_TaskScratch;
	std::string m_LoadMessage;

	std::vector<MemberWorkload> m_MemberWorkloads;

	// Project-wide summary cards (task counts).
	int m_PendingTasks = 0;
	int m_InProgressTasks = 0;
	int m_CompletedTasks = 0;

	// Average remaining hours across members who have any assigned work (for balance badges).
	float m_AverageRemainingHours = 0.0f;

	int m_LoadedProjectId = 0;
	int m_LoadedTasksCacheGeneration = -1;
	bool m_HasLoaded = false;
	bool m_NotifiedLoadError = false;
	bool m_WorkloadDirty = true;
};
