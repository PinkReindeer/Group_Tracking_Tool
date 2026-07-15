#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "Database/Database.h"

class TasksView
{
public:
	TasksView() = default;
	~TasksView() = default;

	// projectId is required to load/create tasks. isLeader gates create/edit/delete/review.
	// Assigned members can accept pending tasks and submit in-progress work.
	void OnRender(const char* projectName, const char* createdDate, int projectId, bool isLeader);

private:
	void EnsureTasksLoaded(int projectId, bool forceRefresh = false);
	void EnsureFormLookupsLoaded(int projectId);
	void RenderCreateTaskModal(int projectId);
	void RenderEditTaskModal(int projectId);
	void RenderDeleteTaskModal(int projectId);
	void RenderSubmitTaskModal(int projectId);
	void RenderReviewTaskModal(int projectId);
	void ResetCreateForm();
	void ResetSubmitForm();
	void ResetReviewForm();
	void OpenReviewForTask(int projectId, const TrackingTool::TaskInfo& task);
	void FillFormFromTask(const TrackingTool::TaskInfo& task);
	bool ParseFormHours(float& outHours) const;
	int ResolveSelectedMembershipId() const;
	int ResolveSelectedMilestoneId() const;
	// Collects checked prerequisite task ids from the create/edit form.
	std::vector<int> ResolveSelectedDependencyIds() const;
	// Checkbox list of other project tasks (excludes m_EditingTaskId when editing).
	void RenderDependencyPicker(float fieldWidth);

	std::vector<TrackingTool::TaskInfo> m_Tasks;
	int m_LoadedProjectId = 0;
	bool m_HasLoaded = false;

	// Lookups for create/edit forms (milestones + members of this project).
	std::vector<TrackingTool::MilestoneInfo> m_FormMilestones;
	std::vector<TrackingTool::MemberInfo> m_FormMembers;
	int m_FormLookupsProjectId = 0;
	bool m_HasFormLookups = false;

	// Shared form buffers (create + edit).
	char m_FormTaskName[128] = "";
	char m_FormDescription[512] = "";
	char m_FormEstimatedHours[32] = "";
	char m_FormDeadline[16] = "";

	// Index into m_FormMilestones (-1 = none selected).
	int m_SelectedMilestoneIndex = -1;
	// 0 = Unassigned; otherwise index into m_FormMembers + 1.
	int m_SelectedMemberIndex = 0;
	// 0=low, 1=medium, 2=high, 3=urgent
	int m_SelectedPriorityIndex = 1;
	// Prerequisite task ids selected in create/edit (taskdependency.dependsontaskid).
	std::unordered_set<int> m_SelectedDependencyIds;

	// Edit state
	int m_EditingTaskId = 0;
	std::string m_EditingStatus;
	bool m_OpenEditModal = false;

	// Delete confirmation state
	int m_PendingDeleteTaskId = 0;
	std::string m_PendingDeleteTaskName;
	bool m_OpenDeleteModal = false;

	// Submit deliverable state (assigned member)
	int m_SubmitTaskId = 0;
	std::string m_SubmitTaskName;
	bool m_OpenSubmitModal = false;
	char m_SubmitExecutionNotes[1024] = "";
	char m_SubmitFilePath[512] = "";
	char m_SubmitCodeSnippet[4096] = "";

	// Review state (team lead)
	int m_ReviewTaskId = 0;
	std::string m_ReviewTaskName;
	std::string m_ReviewAssigneeName;
	bool m_OpenReviewModal = false;
	bool m_ReviewDeliverableLoaded = false;
	bool m_ReviewDeliverableFound = false;
	TrackingTool::DeliverableInfo m_ReviewDeliverable;
	char m_ReviewComment[1024] = "";

	// Milestone groups that are currently collapsed (missing = expanded).
	std::unordered_set<int> m_CollapsedMilestoneIds;
};
