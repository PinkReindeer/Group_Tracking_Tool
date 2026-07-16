#include "TasksView.h"

#include "imgui.h"
#include "IconsFontAwesome6.h"

#include "Platform/Application.h"
#include "Service/AuthService.h"
#include "Service/ProjectService.h"
#include "Utils/TimeUtils.h"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>

namespace
{
	const char* const kPriorities[] = { "low", "medium", "high", "urgent" };
	const char* const kPriorityLabels[] = { "Low", "Medium", "High", "Urgent" };

	// Case-insensitive ASCII compare against a literal — no heap allocation.
	bool EqualsIgnoreCase(const std::string& value, const char* literal)
	{
		size_t i = 0;
		for (; i < value.size() && literal[i] != '\0'; ++i)
		{
			const unsigned char a = static_cast<unsigned char>(value[i]);
			const unsigned char b = static_cast<unsigned char>(literal[i]);
			if (std::tolower(a) != std::tolower(b))
				return false;
		}
		return i == value.size() && literal[i] == '\0';
	}

	const char* ToBadgeLabel(const std::string& status)
	{
		if (status == "backlog")
			return "Backlog";
		if (status == "pending")
			return "Pending";
		if (status == "in progress")
			return "In Progress";
		if (status == "under review")
			return "Review";
		if (status == "done")
			return "Done";
		return "Unknown";
	}

	const char* PriorityDisplayLabel(const std::string& priority)
	{
		if (EqualsIgnoreCase(priority, "low"))
			return "Low";
		if (EqualsIgnoreCase(priority, "medium"))
			return "Medium";
		if (EqualsIgnoreCase(priority, "high"))
			return "High";
		if (EqualsIgnoreCase(priority, "urgent"))
			return "Urgent";
		return "—";
	}

	// Solid accent used for left priority bar and filled priority chip.
	ImVec4 PriorityAccentColor(const std::string& priority)
	{
		if (EqualsIgnoreCase(priority, "low"))
			return ImVec4(120.0f / 255.0f, 144.0f / 255.0f, 156.0f / 255.0f, 1.0f); // blue-gray
		if (EqualsIgnoreCase(priority, "medium"))
			return ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f);           // cyan
		if (EqualsIgnoreCase(priority, "high"))
			return ImVec4(255.0f / 255.0f, 167.0f / 255.0f, 38.0f / 255.0f, 1.0f); // amber
		if (EqualsIgnoreCase(priority, "urgent"))
			return ImVec4(244.0f / 255.0f, 67.0f / 255.0f, 54.0f / 255.0f, 1.0f);  // red
		return ImVec4(120.0f / 255.0f, 144.0f / 255.0f, 156.0f / 255.0f, 1.0f);
	}

	void PriorityBadgeColors(const std::string& priority, ImVec4& outBg, ImVec4& outText)
	{
		const ImVec4 accent = PriorityAccentColor(priority);
		outText = accent;
		outBg = ImVec4(accent.x, accent.y, accent.z, 0.18f);
	}

	void StatusBadgeColors(const std::string& status, ImVec4& outBg, ImVec4& outText,
		const ImVec4& grayText, const ImVec4& cyanColor)
	{
		outBg = ImVec4(60.0f / 255.0f, 65.0f / 255.0f, 65.0f / 255.0f, 1.0f);
		outText = grayText;

		if (status == "pending")
		{
			outBg = ImVec4(60.0f / 255.0f, 65.0f / 255.0f, 65.0f / 255.0f, 1.0f);
			outText = grayText;
		}
		else if (status == "in progress")
		{
			outBg = ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 0.25f);
			outText = cyanColor;
		}
		else if (status == "under review")
		{
			outBg = ImVec4(255.0f / 255.0f, 193.0f / 255.0f, 7.0f / 255.0f, 0.2f);
			outText = ImVec4(255.0f / 255.0f, 213.0f / 255.0f, 79.0f / 255.0f, 1.0f);
		}
		else if (status == "done")
		{
			outBg = ImVec4(0.0f, 100.0f / 255.0f, 50.0f / 255.0f, 0.5f);
			outText = ImVec4(0.0f, 255.0f / 255.0f, 100.0f / 255.0f, 1.0f);
		}
	}

	float BadgeWidthForLabel(const char* badgeLabel, float minWidth = 56.0f)
	{
		const ImVec2 textSize = ImGui::CalcTextSize(badgeLabel);
		const float padded = textSize.x * 0.85f + 18.0f;
		return padded < minWidth ? minWidth : padded;
	}

	// Overdue when deadline is before today and the task is not already done.
	// todayMmDdYyyy should be GetTodayMmDdYyyy() (no heap allocation).
	bool IsTaskOverdue(const TrackingTool::TaskInfo& task, const char* todayMmDdYyyy)
	{
		if (task.Deadline.empty() || task.Status == "done")
			return false;
		if (!todayMmDdYyyy || !TrackingTool::Utils::IsValidMmDdYyyy(task.Deadline.c_str()))
			return false;
		return TrackingTool::Utils::CompareMmDdYyyy(task.Deadline.c_str(), todayMmDdYyyy) < 0;
	}

	int PriorityIndexFromString(const std::string& priority)
	{
		for (int i = 0; i < 4; ++i)
		{
			if (EqualsIgnoreCase(priority, kPriorities[i]))
				return i;
		}
		return 1; // medium default
	}

	void CopyToBuffer(char* dest, size_t destSize, const std::string& src)
	{
		if (destSize == 0)
			return;
#if defined(_WIN32)
		strncpy_s(dest, destSize, src.c_str(), _TRUNCATE);
#else
		std::snprintf(dest, destSize, "%s", src.c_str());
#endif
	}

	// Copy src into dest, flattening newlines to spaces. No heap allocation.
	size_t CopyFlattenedLine(char* dest, size_t destSize, const char* src)
	{
		if (destSize == 0)
			return 0;
		if (!src)
		{
			dest[0] = '\0';
			return 0;
		}

		size_t n = 0;
		for (const char* p = src; *p != '\0' && n + 1 < destSize; ++p)
		{
			const char c = *p;
			dest[n++] = (c == '\n' || c == '\r') ? ' ' : c;
		}
		dest[n] = '\0';
		return n;
	}

	// Append src to dest (null-terminated). Returns new length. Truncates if full.
	size_t AppendCStr(char* dest, size_t destSize, size_t pos, const char* src)
	{
		if (destSize == 0 || !src)
			return pos;
		while (*src != '\0' && pos + 1 < destSize)
			dest[pos++] = *src++;
		dest[pos] = '\0';
		return pos;
	}

	// In-place ellipsis so ImGui::CalcTextSize(buf) fits in maxWidth. Stack-only.
	void EllipsizeToWidth(char* buf, size_t bufSize, float maxWidth)
	{
		if (!buf || bufSize < 4 || maxWidth <= 0.0f)
			return;

		if (ImGui::CalcTextSize(buf).x <= maxWidth)
			return;

		const float ellipsisW = ImGui::CalcTextSize("...").x;
		if (maxWidth <= ellipsisW)
		{
			buf[0] = '.';
			buf[1] = '.';
			buf[2] = '.';
			buf[3] = '\0';
			return;
		}

		// Binary search largest prefix whose width + "..." still fits.
		const size_t len = std::strlen(buf);
		size_t lo = 0;
		size_t hi = len;
		size_t best = 0;
		while (lo <= hi)
		{
			const size_t mid = lo + (hi - lo) / 2;
			const char saved = buf[mid];
			buf[mid] = '\0';
			const float w = ImGui::CalcTextSize(buf).x;
			buf[mid] = saved;

			if (w + ellipsisW <= maxWidth)
			{
				best = mid;
				lo = mid + 1;
			}
			else
			{
				if (mid == 0)
					break;
				hi = mid - 1;
			}
		}

		if (best + 4 > bufSize)
			best = bufSize - 4;
		buf[best] = '.';
		buf[best + 1] = '.';
		buf[best + 2] = '.';
		buf[best + 3] = '\0';
	}

	// Flatten + ellipsize into a stack buffer for single-line card labels.
	void FormatEllipsizedLine(char* dest, size_t destSize, const char* src, float maxWidth)
	{
		CopyFlattenedLine(dest, destSize, src);
		EllipsizeToWidth(dest, destSize, maxWidth);
	}

	// Shared field chrome used by create + edit modals.
	void PushFormFieldStyles()
	{
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(15.0f / 255.0f, 16.0f / 255.0f, 16.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(35.0f / 255.0f, 38.0f / 255.0f, 38.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(45.0f / 255.0f, 48.0f / 255.0f, 48.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 0.35f));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 0.65f));
		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(20.0f / 255.0f, 22.0f / 255.0f, 22.0f / 255.0f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 10.0f));
	}

	void PopFormFieldStyles()
	{
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(7);
	}
}

void TasksView::ResetCreateForm()
{
	memset(m_FormTaskName, 0, sizeof(m_FormTaskName));
	memset(m_FormDescription, 0, sizeof(m_FormDescription));
	memset(m_FormEstimatedHours, 0, sizeof(m_FormEstimatedHours));
	memset(m_FormDeadline, 0, sizeof(m_FormDeadline));
	m_SelectedMilestoneIndex = m_FormMilestones.empty() ? -1 : 0;
	m_SelectedMemberIndex = 0;
	m_SelectedPriorityIndex = 1; // medium
	m_SelectedDependencyIds.clear();
	m_EditingTaskId = 0;
	m_EditingStatus.clear();
}

void TasksView::ResetSubmitForm()
{
	memset(m_SubmitExecutionNotes, 0, sizeof(m_SubmitExecutionNotes));
	memset(m_SubmitFilePath, 0, sizeof(m_SubmitFilePath));
	memset(m_SubmitCodeSnippet, 0, sizeof(m_SubmitCodeSnippet));
	m_SubmitTaskId = 0;
	m_SubmitTaskName.clear();
}

void TasksView::ResetReviewForm()
{
	memset(m_ReviewComment, 0, sizeof(m_ReviewComment));
	m_ReviewTaskId = 0;
	m_ReviewTaskName.clear();
	m_ReviewAssigneeName.clear();
	m_ReviewDeliverableLoaded = false;
	m_ReviewDeliverableFound = false;
	m_ReviewDeliverable = TrackingTool::DeliverableInfo{};
}

void TasksView::OpenReviewForTask(int projectId, const TrackingTool::TaskInfo& task)
{
	ResetReviewForm();
	m_ReviewTaskId = task.Id;
	m_ReviewTaskName = task.Name;
	m_ReviewAssigneeName = task.AssignedMemberName;

	std::string message;
	TrackingTool::DeliverableInfo deliverable;
	bool found = false;
	if (TrackingTool::ProjectService::GetLatestDeliverable(projectId, task.Id, deliverable, found, message))
	{
		m_ReviewDeliverable = std::move(deliverable);
		m_ReviewDeliverableFound = found;
		m_ReviewDeliverableLoaded = true;
	}
	else
	{
		m_ReviewDeliverableLoaded = false;
		m_ReviewDeliverableFound = false;
		TrackingTool::Application::Get().PushNotification(message, NotificationType::Error);
	}

	m_OpenReviewModal = true;
}

void TasksView::FillFormFromTask(const TrackingTool::TaskInfo& task)
{
	CopyToBuffer(m_FormTaskName, sizeof(m_FormTaskName), task.Name);
	CopyToBuffer(m_FormDescription, sizeof(m_FormDescription), task.Description);
	if (task.EstimatedHours > 0.0f)
	{
		char hoursBuf[32];
		std::snprintf(hoursBuf, sizeof(hoursBuf), "%.1f", static_cast<double>(task.EstimatedHours));
		// Trim trailing .0
		std::string hoursStr = hoursBuf;
		if (hoursStr.size() >= 2 && hoursStr.ends_with(".0"))
			hoursStr.resize(hoursStr.size() - 2);
		CopyToBuffer(m_FormEstimatedHours, sizeof(m_FormEstimatedHours), hoursStr);
	}
	else
	{
		memset(m_FormEstimatedHours, 0, sizeof(m_FormEstimatedHours));
	}
	CopyToBuffer(m_FormDeadline, sizeof(m_FormDeadline), task.Deadline);

	m_SelectedMilestoneIndex = -1;
	for (int i = 0; i < static_cast<int>(m_FormMilestones.size()); ++i)
	{
		if (m_FormMilestones[static_cast<size_t>(i)].Id == task.MilestoneId)
		{
			m_SelectedMilestoneIndex = i;
			break;
		}
	}

	m_SelectedMemberIndex = 0;
	if (task.AssignedMembershipId > 0)
	{
		for (int i = 0; i < static_cast<int>(m_FormMembers.size()); ++i)
		{
			if (m_FormMembers[static_cast<size_t>(i)].MembershipId == task.AssignedMembershipId)
			{
				m_SelectedMemberIndex = i + 1;
				break;
			}
		}
	}

	m_SelectedPriorityIndex = PriorityIndexFromString(task.Priority);
	m_EditingTaskId = task.Id;
	m_EditingStatus = task.Status;

	m_SelectedDependencyIds.clear();
	for (const TrackingTool::TaskDependencyInfo& dep : task.Dependencies)
	{
		if (dep.DependsOnTaskId > 0)
			m_SelectedDependencyIds.insert(dep.DependsOnTaskId);
	}
}

bool TasksView::ParseFormHours(float& outHours) const
{
	outHours = 0.0f;
	if (m_FormEstimatedHours[0] == '\0')
		return true;

	char* end = nullptr;
	outHours = std::strtof(m_FormEstimatedHours, &end);
	if (end == m_FormEstimatedHours)
	{
		TrackingTool::Application::Get().PushNotification("Estimated hours must be a number.", NotificationType::Error);
		return false;
	}
	return true;
}

int TasksView::ResolveSelectedMembershipId() const
{
	if (m_SelectedMemberIndex > 0
		&& m_SelectedMemberIndex <= static_cast<int>(m_FormMembers.size()))
	{
		return m_FormMembers[static_cast<size_t>(m_SelectedMemberIndex - 1)].MembershipId;
	}
	return 0;
}

int TasksView::ResolveSelectedMilestoneId() const
{
	if (m_SelectedMilestoneIndex >= 0
		&& m_SelectedMilestoneIndex < static_cast<int>(m_FormMilestones.size()))
	{
		return m_FormMilestones[static_cast<size_t>(m_SelectedMilestoneIndex)].Id;
	}
	return 0;
}

std::vector<int> TasksView::ResolveSelectedDependencyIds() const
{
	std::vector<int> ids;
	ids.reserve(m_SelectedDependencyIds.size());
	for (int id : m_SelectedDependencyIds)
	{
		if (id > 0 && id != m_EditingTaskId)
			ids.push_back(id);
	}
	return ids;
}

void TasksView::RenderDependencyPicker(float fieldWidth)
{
	const ImVec4 grayText = ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f);
	const ImVec4 whiteText = ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f);

	ImGui::PushStyleColor(ImGuiCol_Text, grayText);
	ImGui::Text("DEPENDS ON (must be Done first)");
	ImGui::PopStyleColor();

	// Count selectable candidates (exclude the task being edited).
	int selectableCount = 0;
	for (const TrackingTool::TaskInfo& t : m_Tasks)
	{
		if (t.Id > 0 && t.Id != m_EditingTaskId)
			++selectableCount;
	}

	if (selectableCount == 0)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::TextWrapped("No other tasks yet — create more tasks to add prerequisites.");
		ImGui::PopStyleColor();
		return;
	}

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(15.0f / 255.0f, 16.0f / 255.0f, 16.0f / 255.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(60.0f / 255.0f, 73.0f / 255.0f, 74.0f / 255.0f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));

	const float listHeight = selectableCount > 4 ? 120.0f : static_cast<float>(selectableCount) * 28.0f + 16.0f;
	if (ImGui::BeginChild("##TaskDependencyPicker", ImVec2(fieldWidth, listHeight), true))
	{
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		for (const TrackingTool::TaskInfo& t : m_Tasks)
		{
			if (t.Id <= 0 || t.Id == m_EditingTaskId)
				continue;

			bool checked = m_SelectedDependencyIds.count(t.Id) > 0;
			char label[256];
			const char* statusLabel = ToBadgeLabel(t.Status);
			std::snprintf(label, sizeof(label), "%s  [%s]",
				t.Name.empty() ? "(unnamed)" : t.Name.c_str(), statusLabel);

			ImGui::PushID(t.Id);
			if (ImGui::Checkbox(label, &checked))
			{
				if (checked)
					m_SelectedDependencyIds.insert(t.Id);
				else
					m_SelectedDependencyIds.erase(t.Id);
			}
			ImGui::PopID();
		}
		ImGui::PopStyleColor();
	}
	ImGui::EndChild();

	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(2);

	if (!m_SelectedDependencyIds.empty())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::Text("%zu prerequisite(s) selected", m_SelectedDependencyIds.size());
		ImGui::PopStyleColor();
	}
}

void TasksView::EnsureTasksLoaded(int projectId, bool forceRefresh)
{
	if (projectId <= 0)
	{
		m_Tasks.clear();
		m_LoadedProjectId = 0;
		m_HasLoaded = false;
		return;
	}

	if (!forceRefresh && m_HasLoaded && m_LoadedProjectId == projectId)
		return;

	// Drop collapse state when switching projects so we don't leak ids.
	if (m_LoadedProjectId != projectId)
		m_CollapsedMilestoneIds.clear();

	std::string message;
	std::vector<TrackingTool::TaskInfo> tasks;
	if (TrackingTool::ProjectService::GetProjectTasks(projectId, tasks, message, forceRefresh))
	{
		m_Tasks = std::move(tasks);
		m_LoadedProjectId = projectId;
		m_HasLoaded = true;
	}
	else
	{
		if (m_LoadedProjectId != projectId)
		{
			m_Tasks.clear();
			m_LoadedProjectId = projectId;
			m_HasLoaded = false;
		}
		TrackingTool::Application::Get().PushNotification(message, NotificationType::Error);
	}
}

void TasksView::EnsureFormLookupsLoaded(int projectId)
{
	if (projectId <= 0)
	{
		m_FormMilestones.clear();
		m_FormMembers.clear();
		m_FormLookupsProjectId = 0;
		m_HasFormLookups = false;
		return;
	}

	if (m_HasFormLookups && m_FormLookupsProjectId == projectId)
		return;

	std::string message;
	std::vector<TrackingTool::MilestoneInfo> milestones;
	if (!TrackingTool::ProjectService::GetProjectMilestones(projectId, milestones, message, false))
	{
		TrackingTool::Application::Get().PushNotification(message, NotificationType::Error);
	}

	std::vector<TrackingTool::MemberInfo> members;
	if (!TrackingTool::Database::GetProjectMembers(projectId, members))
	{
		TrackingTool::Application::Get().PushNotification(
			"Failed to load project members for task assignment.", NotificationType::Error);
	}

	m_FormMilestones = std::move(milestones);
	m_FormMembers = std::move(members);
	m_FormLookupsProjectId = projectId;
	m_HasFormLookups = true;

	if (m_SelectedMilestoneIndex < 0 || m_SelectedMilestoneIndex >= static_cast<int>(m_FormMilestones.size()))
		m_SelectedMilestoneIndex = m_FormMilestones.empty() ? -1 : 0;
	if (m_SelectedMemberIndex < 0 || m_SelectedMemberIndex > static_cast<int>(m_FormMembers.size()))
		m_SelectedMemberIndex = 0;
}

void TasksView::RenderCreateTaskModal(int projectId)
{
	const ImVec4 cyanColor = ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f);
	const ImVec4 grayText = ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f);
	const ImVec4 whiteText = ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f);
	const ImVec4 borderColor = ImVec4(60.0f / 255.0f, 73.0f / 255.0f, 74.0f / 255.0f, 1.0f);
	const ImVec4 boxBgColor = ImVec4(26.0f / 255.0f, 28.0f / 255.0f, 28.0f / 255.0f, 1.0f);

	const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	ImGui::PushStyleColor(ImGuiCol_PopupBg, boxBgColor);
	ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 24.0f));

	if (ImGui::BeginPopupModal("Create Task", nullptr,
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
	{
		if (ImGui::IsWindowAppearing())
		{
			m_HasFormLookups = false;
			EnsureFormLookupsLoaded(projectId);
			if (m_SelectedMilestoneIndex < 0 && !m_FormMilestones.empty())
				m_SelectedMilestoneIndex = 0;
		}

		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetWindowFontScale(1.2f);
		ImGui::Text(ICON_FA_PLUS " Create Task");
		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 8.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 380.0f);
		ImGui::TextWrapped("Unassigned tasks start as Backlog; assigned tasks start as Pending. Deadline uses MM-DD-YYYY.");
		ImGui::PopTextWrapPos();
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 15.0f));

		PushFormFieldStyles();
		const float fieldWidth = 380.0f;

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::Text("TASK NAME");
		ImGui::PopStyleColor();
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetNextItemWidth(fieldWidth);
		if (ImGui::IsWindowAppearing())
			ImGui::SetKeyboardFocusHere();
		ImGui::InputText("##CreateTaskName", m_FormTaskName, IM_ARRAYSIZE(m_FormTaskName));
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 10.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::Text("MILESTONE");
		ImGui::PopStyleColor();
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetNextItemWidth(fieldWidth);
		{
			const char* milestonePreview = m_FormMilestones.empty()
				? "(No milestones — create one first)"
				: (m_SelectedMilestoneIndex >= 0 && m_SelectedMilestoneIndex < static_cast<int>(m_FormMilestones.size())
					? m_FormMilestones[static_cast<size_t>(m_SelectedMilestoneIndex)].Name.c_str()
					: "Select milestone");
			if (ImGui::BeginCombo("##CreateTaskMilestone", milestonePreview))
			{
				for (int i = 0; i < static_cast<int>(m_FormMilestones.size()); ++i)
				{
					const bool selected = (m_SelectedMilestoneIndex == i);
					if (ImGui::Selectable(m_FormMilestones[static_cast<size_t>(i)].Name.c_str(), selected))
						m_SelectedMilestoneIndex = i;
					if (selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		}
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 10.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::Text("ASSIGN MEMBER");
		ImGui::PopStyleColor();
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetNextItemWidth(fieldWidth);
		{
			const char* memberPreview = "Unassigned";
			if (m_SelectedMemberIndex > 0 && m_SelectedMemberIndex <= static_cast<int>(m_FormMembers.size()))
				memberPreview = m_FormMembers[static_cast<size_t>(m_SelectedMemberIndex - 1)].Name.c_str();

			if (ImGui::BeginCombo("##CreateTaskMember", memberPreview))
			{
				if (ImGui::Selectable("Unassigned", m_SelectedMemberIndex == 0))
					m_SelectedMemberIndex = 0;
				if (m_SelectedMemberIndex == 0)
					ImGui::SetItemDefaultFocus();

				for (int i = 0; i < static_cast<int>(m_FormMembers.size()); ++i)
				{
					const int comboIndex = i + 1;
					const bool selected = (m_SelectedMemberIndex == comboIndex);
					if (ImGui::Selectable(m_FormMembers[static_cast<size_t>(i)].Name.c_str(), selected))
						m_SelectedMemberIndex = comboIndex;
					if (selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		}
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 10.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::Text("DESCRIPTION");
		ImGui::PopStyleColor();
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::InputTextMultiline("##CreateTaskDescription", m_FormDescription, IM_ARRAYSIZE(m_FormDescription), ImVec2(fieldWidth, 80.0f));
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 10.0f));

		const float halfWidth = (fieldWidth - 12.0f) * 0.5f;
		const float startX = ImGui::GetCursorPosX();

		// -- Row 1: Labels --
		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::Text("ESTIMATED HOURS");
		ImGui::SameLine();
		ImGui::SetCursorPosX(startX + halfWidth + 12.0f);
		ImGui::Text("PRIORITY");
		ImGui::PopStyleColor();

		// -- Row 2: Inputs --
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);

		// First input
		ImGui::SetNextItemWidth(halfWidth);
		ImGui::InputTextWithHint("##CreateTaskHours", "e.g. 4", m_FormEstimatedHours, IM_ARRAYSIZE(m_FormEstimatedHours));

		ImGui::SameLine();
		ImGui::SetCursorPosX(startX + halfWidth + 12.0f);

		// Second input
		ImGui::SetNextItemWidth(halfWidth);
		if (ImGui::BeginCombo("##CreateTaskPriority", kPriorityLabels[m_SelectedPriorityIndex]))
		{
			for (int i = 0; i < 4; ++i)
			{
				const bool selected = (m_SelectedPriorityIndex == i);
				if (ImGui::Selectable(kPriorityLabels[i], selected))
					m_SelectedPriorityIndex = i;
				if (selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 10.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::Text("DEADLINE (MM-DD-YYYY)");
		ImGui::PopStyleColor();
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetNextItemWidth(fieldWidth);
		ImGui::InputTextWithHint("##CreateTaskDeadline", "optional, e.g. 07-30-2026", m_FormDeadline, IM_ARRAYSIZE(m_FormDeadline));
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 10.0f));
		RenderDependencyPicker(fieldWidth);

		PopFormFieldStyles();

		ImGui::Dummy(ImVec2(0.0f, 20.0f));

		const float modalBtnWidth = (fieldWidth - 10.0f) / 2.0f;

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		if (ImGui::Button("Cancel##CreateTask", ImVec2(modalBtnWidth, 36.0f)))
		{
			ResetCreateForm();
			ImGui::CloseCurrentPopup();
		}
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(2);

		ImGui::SameLine(0, 10.0f);

		ImGui::PushStyleColor(ImGuiCol_Button, cyanColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 193.0f / 255.0f, 201.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 153.0f / 255.0f, 161.0f / 255.0f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(18.0f / 255.0f, 20.0f / 255.0f, 20.0f / 255.0f, 1.0f));
		if (ImGui::Button("Create##CreateTask", ImVec2(modalBtnWidth, 36.0f)))
		{
			const int milestoneId = ResolveSelectedMilestoneId();
			if (milestoneId <= 0)
			{
				TrackingTool::Application::Get().PushNotification(
					"Create a milestone first, then assign the task to it.", NotificationType::Error);
			}
			else
			{
				float hours = 0.0f;
				if (ParseFormHours(hours) && hours >= 0.0f)
				{
					std::string message;
					const std::vector<int> dependsOn = ResolveSelectedDependencyIds();
					if (TrackingTool::ProjectService::CreateTask(
						projectId, milestoneId, ResolveSelectedMembershipId(),
						m_FormTaskName, m_FormDescription, hours,
						kPriorities[m_SelectedPriorityIndex], m_FormDeadline, dependsOn, message))
					{
						TrackingTool::Application::Get().PushNotification(message, NotificationType::Info);
						EnsureTasksLoaded(projectId, true);
						ResetCreateForm();
						ImGui::CloseCurrentPopup();
					}
					else
					{
						TrackingTool::Application::Get().PushNotification(message, NotificationType::Error);
					}
				}
			}
		}
		ImGui::SetItemDefaultFocus();
		ImGui::PopStyleColor(4);
		ImGui::PopStyleVar();

		ImGui::EndPopup();
	}

	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(2);
}

void TasksView::RenderEditTaskModal(int projectId)
{
	const ImVec4 cyanColor = ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f);
	const ImVec4 grayText = ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f);
	const ImVec4 whiteText = ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f);
	const ImVec4 borderColor = ImVec4(60.0f / 255.0f, 73.0f / 255.0f, 74.0f / 255.0f, 1.0f);
	const ImVec4 boxBgColor = ImVec4(26.0f / 255.0f, 28.0f / 255.0f, 28.0f / 255.0f, 1.0f);

	if (m_OpenEditModal)
	{
		ImGui::OpenPopup("Edit Task");
		m_OpenEditModal = false;
	}

	const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	ImGui::PushStyleColor(ImGuiCol_PopupBg, boxBgColor);
	ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 24.0f));

	if (ImGui::BeginPopupModal("Edit Task", nullptr,
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
	{
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetWindowFontScale(1.2f);
		ImGui::Text(ICON_FA_PEN_TO_SQUARE " Edit Task");
		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 8.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 380.0f);
		ImGui::TextWrapped("Changing assignment updates Backlog/Pending. In Progress, Review, and Done stay as-is.");
		ImGui::PopTextWrapPos();
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 15.0f));

		PushFormFieldStyles();
		const float fieldWidth = 380.0f;

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::Text("TASK NAME");
		ImGui::PopStyleColor();
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetNextItemWidth(fieldWidth);
		if (ImGui::IsWindowAppearing())
			ImGui::SetKeyboardFocusHere();
		ImGui::InputText("##EditTaskName", m_FormTaskName, IM_ARRAYSIZE(m_FormTaskName));
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 10.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::Text("MILESTONE");
		ImGui::PopStyleColor();
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetNextItemWidth(fieldWidth);
		{
			const char* milestonePreview = m_FormMilestones.empty()
				? "(No milestones)"
				: (m_SelectedMilestoneIndex >= 0 && m_SelectedMilestoneIndex < static_cast<int>(m_FormMilestones.size())
					? m_FormMilestones[static_cast<size_t>(m_SelectedMilestoneIndex)].Name.c_str()
					: "Select milestone");
			if (ImGui::BeginCombo("##EditTaskMilestone", milestonePreview))
			{
				for (int i = 0; i < static_cast<int>(m_FormMilestones.size()); ++i)
				{
					const bool selected = (m_SelectedMilestoneIndex == i);
					if (ImGui::Selectable(m_FormMilestones[static_cast<size_t>(i)].Name.c_str(), selected))
						m_SelectedMilestoneIndex = i;
					if (selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		}
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 10.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::Text("ASSIGN MEMBER");
		ImGui::PopStyleColor();
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetNextItemWidth(fieldWidth);
		{
			const char* memberPreview = "Unassigned";
			if (m_SelectedMemberIndex > 0 && m_SelectedMemberIndex <= static_cast<int>(m_FormMembers.size()))
				memberPreview = m_FormMembers[static_cast<size_t>(m_SelectedMemberIndex - 1)].Name.c_str();

			if (ImGui::BeginCombo("##EditTaskMember", memberPreview))
			{
				if (ImGui::Selectable("Unassigned", m_SelectedMemberIndex == 0))
					m_SelectedMemberIndex = 0;
				if (m_SelectedMemberIndex == 0)
					ImGui::SetItemDefaultFocus();

				for (int i = 0; i < static_cast<int>(m_FormMembers.size()); ++i)
				{
					const int comboIndex = i + 1;
					const bool selected = (m_SelectedMemberIndex == comboIndex);
					if (ImGui::Selectable(m_FormMembers[static_cast<size_t>(i)].Name.c_str(), selected))
						m_SelectedMemberIndex = comboIndex;
					if (selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		}
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 10.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::Text("DESCRIPTION");
		ImGui::PopStyleColor();
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::InputTextMultiline("##EditTaskDescription", m_FormDescription, IM_ARRAYSIZE(m_FormDescription), ImVec2(fieldWidth, 80.0f));
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 10.0f));

		const float halfWidth = (fieldWidth - 12.0f) * 0.5f;
		const float startX = ImGui::GetCursorPosX();

		// -- Row 1: Labels --
		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::Text("ESTIMATED HOURS");
		ImGui::SameLine();
		ImGui::SetCursorPosX(startX + halfWidth + 12.0f);
		ImGui::Text("PRIORITY");
		ImGui::PopStyleColor();

		// -- Row 2: Inputs --
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);

		// First input
		ImGui::SetNextItemWidth(halfWidth);
		ImGui::InputTextWithHint("##CreateTaskHours", "e.g. 4", m_FormEstimatedHours, IM_ARRAYSIZE(m_FormEstimatedHours));

		ImGui::SameLine();
		ImGui::SetCursorPosX(startX + halfWidth + 12.0f);

		// Second input
		ImGui::SetNextItemWidth(halfWidth);
		if (ImGui::BeginCombo("##CreateTaskPriority", kPriorityLabels[m_SelectedPriorityIndex]))
		{
			for (int i = 0; i < 4; ++i)
			{
				const bool selected = (m_SelectedPriorityIndex == i);
				if (ImGui::Selectable(kPriorityLabels[i], selected))
					m_SelectedPriorityIndex = i;
				if (selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 10.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::Text("DEADLINE (MM-DD-YYYY)");
		ImGui::PopStyleColor();
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetNextItemWidth(fieldWidth);
		ImGui::InputTextWithHint("##EditTaskDeadline", "optional, e.g. 07-30-2026", m_FormDeadline, IM_ARRAYSIZE(m_FormDeadline));
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 10.0f));
		RenderDependencyPicker(fieldWidth);

		PopFormFieldStyles();

		ImGui::Dummy(ImVec2(0.0f, 20.0f));

		const float modalBtnWidth = (fieldWidth - 10.0f) / 2.0f;

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		if (ImGui::Button("Cancel##EditTask", ImVec2(modalBtnWidth, 36.0f)))
		{
			ResetCreateForm();
			ImGui::CloseCurrentPopup();
		}
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(2);

		ImGui::SameLine(0, 10.0f);

		ImGui::PushStyleColor(ImGuiCol_Button, cyanColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 193.0f / 255.0f, 201.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 153.0f / 255.0f, 161.0f / 255.0f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(18.0f / 255.0f, 20.0f / 255.0f, 20.0f / 255.0f, 1.0f));
		if (ImGui::Button("Save##EditTask", ImVec2(modalBtnWidth, 36.0f)))
		{
			const int milestoneId = ResolveSelectedMilestoneId();
			if (m_EditingTaskId <= 0)
			{
				TrackingTool::Application::Get().PushNotification("No task selected.", NotificationType::Error);
			}
			else if (milestoneId <= 0)
			{
				TrackingTool::Application::Get().PushNotification(
					"Select a milestone for this task.", NotificationType::Error);
			}
			else
			{
				float hours = 0.0f;
				if (ParseFormHours(hours) && hours >= 0.0f)
				{
					std::string message;
					const std::vector<int> dependsOn = ResolveSelectedDependencyIds();
					if (TrackingTool::ProjectService::UpdateTask(
						projectId, m_EditingTaskId, milestoneId, ResolveSelectedMembershipId(),
						m_FormTaskName, m_FormDescription, hours,
						kPriorities[m_SelectedPriorityIndex], m_FormDeadline, m_EditingStatus,
						dependsOn, message))
					{
						TrackingTool::Application::Get().PushNotification(message, NotificationType::Info);
						EnsureTasksLoaded(projectId, true);
						ResetCreateForm();
						ImGui::CloseCurrentPopup();
					}
					else
					{
						TrackingTool::Application::Get().PushNotification(message, NotificationType::Error);
					}
				}
			}
		}
		ImGui::SetItemDefaultFocus();
		ImGui::PopStyleColor(4);
		ImGui::PopStyleVar();

		ImGui::EndPopup();
	}

	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(2);
}

void TasksView::RenderDeleteTaskModal(int projectId)
{
	const ImVec4 grayText = ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f);
	const ImVec4 whiteText = ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f);
	const ImVec4 borderColor = ImVec4(60.0f / 255.0f, 73.0f / 255.0f, 74.0f / 255.0f, 1.0f);
	const ImVec4 boxBgColor = ImVec4(26.0f / 255.0f, 28.0f / 255.0f, 28.0f / 255.0f, 1.0f);
	const ImVec4 redColor = ImVec4(238.0f / 255.0f, 56.0f / 255.0f, 56.0f / 255.0f, 1.0f);

	if (m_OpenDeleteModal)
	{
		ImGui::OpenPopup("Delete Task");
		m_OpenDeleteModal = false;
	}

	const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	ImGui::PushStyleColor(ImGuiCol_PopupBg, boxBgColor);
	ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 24.0f));

	if (ImGui::BeginPopupModal("Delete Task", nullptr,
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
	{
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetWindowFontScale(1.2f);
		ImGui::Text(ICON_FA_TRASH " Delete Task");
		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 8.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 350.0f);
		ImGui::TextWrapped(
			"Are you sure you want to delete \"%s\"? This cannot be undone.",
			m_PendingDeleteTaskName.empty() ? "this task" : m_PendingDeleteTaskName.c_str());
		ImGui::PopTextWrapPos();
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 20.0f));

		const float modalBtnWidth = (350.0f - 10.0f) / 2.0f;

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		if (ImGui::Button("Cancel##DeleteTask", ImVec2(modalBtnWidth, 36.0f)))
		{
			m_PendingDeleteTaskId = 0;
			m_PendingDeleteTaskName.clear();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(2);

		ImGui::SameLine(0, 10.0f);

		ImGui::PushStyleColor(ImGuiCol_Button, redColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(255.0f / 255.0f, 76.0f / 255.0f, 76.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(200.0f / 255.0f, 40.0f / 255.0f, 40.0f / 255.0f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		if (ImGui::Button("Delete##DeleteTask", ImVec2(modalBtnWidth, 36.0f)))
		{
			std::string message;
			if (TrackingTool::ProjectService::DeleteTask(projectId, m_PendingDeleteTaskId, message))
			{
				TrackingTool::Application::Get().PushNotification(message, NotificationType::Info);
				EnsureTasksLoaded(projectId, true);
				m_PendingDeleteTaskId = 0;
				m_PendingDeleteTaskName.clear();
				ImGui::CloseCurrentPopup();
			}
			else
			{
				TrackingTool::Application::Get().PushNotification(message, NotificationType::Error);
			}
		}
		ImGui::PopStyleColor(4);
		ImGui::PopStyleVar();

		ImGui::EndPopup();
	}

	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(2);
}

void TasksView::RenderSubmitTaskModal(int projectId)
{
	const ImVec4 cyanColor = ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f);
	const ImVec4 grayText = ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f);
	const ImVec4 whiteText = ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f);
	const ImVec4 borderColor = ImVec4(60.0f / 255.0f, 73.0f / 255.0f, 74.0f / 255.0f, 1.0f);
	const ImVec4 boxBgColor = ImVec4(26.0f / 255.0f, 28.0f / 255.0f, 28.0f / 255.0f, 1.0f);

	if (m_OpenSubmitModal)
	{
		ImGui::OpenPopup("Submit Task");
		m_OpenSubmitModal = false;
	}

	const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	ImGui::PushStyleColor(ImGuiCol_PopupBg, boxBgColor);
	ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 24.0f));

	if (ImGui::BeginPopupModal("Submit Task", nullptr,
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
	{
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetWindowFontScale(1.2f);
		ImGui::Text(ICON_FA_PAPER_PLANE " Submit Task");
		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 6.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 420.0f);
		if (!m_SubmitTaskName.empty())
			ImGui::TextWrapped("Deliverable for \"%s\". All fields are required. Submission time is recorded automatically.",
				m_SubmitTaskName.c_str());
		else
			ImGui::TextWrapped("All fields are required. Submission time is recorded automatically.");
		ImGui::PopTextWrapPos();
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 14.0f));

		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(15.0f / 255.0f, 16.0f / 255.0f, 16.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(35.0f / 255.0f, 38.0f / 255.0f, 38.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(45.0f / 255.0f, 48.0f / 255.0f, 48.0f / 255.0f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 10.0f));

		const float fieldWidth = 420.0f;

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::Text("EXECUTION NOTES");
		ImGui::PopStyleColor();
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		if (ImGui::IsWindowAppearing())
			ImGui::SetKeyboardFocusHere();
		ImGui::InputTextMultiline("##SubmitNotes", m_SubmitExecutionNotes, IM_ARRAYSIZE(m_SubmitExecutionNotes),
			ImVec2(fieldWidth, 90.0f));
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 10.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::Text("FILE PATHS");
		ImGui::PopStyleColor();
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetNextItemWidth(fieldWidth);
		ImGui::InputTextWithHint("##SubmitPaths", "e.g. App/Source/Project/Views/TasksView.cpp",
			m_SubmitFilePath, IM_ARRAYSIZE(m_SubmitFilePath));
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 10.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::Text("CODE SNIPPET");
		ImGui::PopStyleColor();
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::InputTextMultiline("##SubmitSnippet", m_SubmitCodeSnippet, IM_ARRAYSIZE(m_SubmitCodeSnippet),
			ImVec2(fieldWidth, 140.0f));
		ImGui::PopStyleColor();

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(3);

		ImGui::Dummy(ImVec2(0.0f, 18.0f));

		const float modalBtnWidth = (fieldWidth - 10.0f) / 2.0f;

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		if (ImGui::Button("Cancel##SubmitTask", ImVec2(modalBtnWidth, 36.0f)))
		{
			ResetSubmitForm();
			ImGui::CloseCurrentPopup();
		}
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(2);

		ImGui::SameLine(0, 10.0f);

		ImGui::PushStyleColor(ImGuiCol_Button, cyanColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 193.0f / 255.0f, 201.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 153.0f / 255.0f, 161.0f / 255.0f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(18.0f / 255.0f, 20.0f / 255.0f, 20.0f / 255.0f, 1.0f));
		if (ImGui::Button(ICON_FA_PAPER_PLANE " Submit##SubmitTask", ImVec2(modalBtnWidth, 36.0f)))
		{
			if (m_SubmitTaskId <= 0)
			{
				TrackingTool::Application::Get().PushNotification("No task selected.", NotificationType::Error);
			}
			else
			{
				std::string message;
				if (TrackingTool::ProjectService::SubmitTask(
					projectId, m_SubmitTaskId,
					m_SubmitExecutionNotes, m_SubmitFilePath, m_SubmitCodeSnippet, message))
				{
					TrackingTool::Application::Get().PushNotification(message, NotificationType::Info);
					EnsureTasksLoaded(projectId, true);
					ResetSubmitForm();
					ImGui::CloseCurrentPopup();
				}
				else
				{
					TrackingTool::Application::Get().PushNotification(message, NotificationType::Error);
				}
			}
		}
		ImGui::SetItemDefaultFocus();
		ImGui::PopStyleColor(4);
		ImGui::PopStyleVar();

		ImGui::EndPopup();
	}

	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(2);
}

void TasksView::RenderReviewTaskModal(int projectId)
{
	const ImVec4 cyanColor = ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f);
	const ImVec4 grayText = ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f);
	const ImVec4 whiteText = ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f);
	const ImVec4 mutedText = ImVec4(140.0f / 255.0f, 150.0f / 255.0f, 152.0f / 255.0f, 1.0f);
	const ImVec4 borderColor = ImVec4(60.0f / 255.0f, 73.0f / 255.0f, 74.0f / 255.0f, 1.0f);
	const ImVec4 boxBgColor = ImVec4(26.0f / 255.0f, 28.0f / 255.0f, 28.0f / 255.0f, 1.0f);
	const ImVec4 greenColor = ImVec4(76.0f / 255.0f, 175.0f / 255.0f, 80.0f / 255.0f, 1.0f);
	const ImVec4 redColor = ImVec4(238.0f / 255.0f, 56.0f / 255.0f, 56.0f / 255.0f, 1.0f);
	const ImVec4 fieldBg = ImVec4(15.0f / 255.0f, 16.0f / 255.0f, 16.0f / 255.0f, 1.0f);
	const ImVec4 cardBg = ImVec4(20.0f / 255.0f, 22.0f / 255.0f, 22.0f / 255.0f, 1.0f);
	const ImVec4 codeBg = ImVec4(12.0f / 255.0f, 13.0f / 255.0f, 13.0f / 255.0f, 1.0f);
	const ImVec4 codeText = ImVec4(200.0f / 255.0f, 220.0f / 255.0f, 200.0f / 255.0f, 1.0f);

	if (m_OpenReviewModal)
	{
		ImGui::OpenPopup("Review Task");
		m_OpenReviewModal = false;
	}

	const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	// Cap height so long submissions scroll inside the modal instead of overflowing the screen.
	const float viewportH = ImGui::GetMainViewport()->Size.y;
	ImGui::SetNextWindowSizeConstraints(ImVec2(520.0f, 0.0f), ImVec2(640.0f, viewportH * 0.88f));

	ImGui::PushStyleColor(ImGuiCol_PopupBg, boxBgColor);
	ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 22.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 6.0f));

	if (ImGui::BeginPopupModal("Review Task", nullptr,
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
	{
		const float fieldWidth = 520.0f;
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		// --- Header ---
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetWindowFontScale(1.15f);
		ImGui::Text(ICON_FA_CLIPBOARD_CHECK "  Review Submission");
		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 4.0f));

		// Task title
		if (!m_ReviewTaskName.empty())
		{
			ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
			ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + fieldWidth);
			ImGui::TextWrapped("%s", m_ReviewTaskName.c_str());
			ImGui::PopTextWrapPos();
			ImGui::PopStyleColor();
		}

		// Meta row: assignee · submitted time
		{
			ImGui::PushStyleColor(ImGuiCol_Text, mutedText);
			if (!m_ReviewAssigneeName.empty())
			{
				ImGui::Text("%s  %s", ICON_FA_USER, m_ReviewAssigneeName.c_str());
				if (m_ReviewDeliverableFound && !m_ReviewDeliverable.SubmissionTime.empty())
					ImGui::SameLine(0.0f, 16.0f);
			}
			if (m_ReviewDeliverableFound && !m_ReviewDeliverable.SubmissionTime.empty())
				ImGui::Text("%s  %s", ICON_FA_CLOCK, m_ReviewDeliverable.SubmissionTime.c_str());
			ImGui::PopStyleColor();
		}

		ImGui::Dummy(ImVec2(0.0f, 6.0f));
		{
			ImVec2 p = ImGui::GetCursorScreenPos();
			drawList->AddLine(p, ImVec2(p.x + fieldWidth, p.y), ImGui::GetColorU32(borderColor), 1.0f);
			ImGui::Dummy(ImVec2(fieldWidth, 1.0f));
		}
		ImGui::Dummy(ImVec2(0.0f, 10.0f));

		// Section label with cyan accent icon.
		auto DrawSectionLabel = [&](const char* icon, const char* label) {
			ImGui::PushStyleColor(ImGuiCol_Text, cyanColor);
			ImGui::Text("%s  %s", icon, label);
			ImGui::PopStyleColor();
			ImGui::Dummy(ImVec2(0.0f, 3.0f));
		};

		// Soft-wrapped text panel (notes / file path).
		auto DrawWrappedPanel = [&](const char* id, const std::string& text, float height) {
			const bool empty = text.empty();
			ImGui::PushStyleColor(ImGuiCol_ChildBg, fieldBg);
			ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
			ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));
			if (ImGui::BeginChild(id, ImVec2(fieldWidth, height), ImGuiChildFlags_Borders))
			{
				ImGui::PushStyleColor(ImGuiCol_Text, empty ? mutedText : whiteText);
				ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + fieldWidth - 28.0f);
				ImGui::TextUnformatted(empty ? "(none)" : text.c_str());
				ImGui::PopTextWrapPos();
				ImGui::PopStyleColor();
			}
			ImGui::EndChild();
			ImGui::PopStyleVar(2);
			ImGui::PopStyleColor(2);
		};

		// Code panel: line-by-line so long lines scroll horizontally instead of wrapping badly.
		auto DrawCodePanel = [&](const char* id, const std::string& text, float height) {
			const bool empty = text.empty();
			ImGui::PushStyleColor(ImGuiCol_ChildBg, codeBg);
			ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
			ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 2.0f));
			if (ImGui::BeginChild(id, ImVec2(fieldWidth, height),
				ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar))
			{
				ImGui::PushStyleColor(ImGuiCol_Text, empty ? mutedText : codeText);
				if (empty)
				{
					ImGui::TextUnformatted("(none)");
				}
				else
				{
					const char* begin = text.c_str();
					const char* end = begin + text.size();
					const char* lineStart = begin;
					int lineNo = 0;
					for (const char* p = begin; p <= end; ++p)
					{
						if (p == end || *p == '\n')
						{
							ImGui::PushID(lineNo++);
							if (p > lineStart)
								ImGui::TextUnformatted(lineStart, p);
							else
								ImGui::TextUnformatted(" ");
							ImGui::PopID();
							lineStart = p + 1;
						}
					}
				}
				ImGui::PopStyleColor();
			}
			ImGui::EndChild();
			ImGui::PopStyleVar(3);
			ImGui::PopStyleColor(2);
		};

		// --- Submission body ---
		if (m_ReviewDeliverableLoaded && m_ReviewDeliverableFound)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, cyanColor);
			ImGui::Text(ICON_FA_BOX_ARCHIVE "  Member submission");
			ImGui::PopStyleColor();
			ImGui::Dummy(ImVec2(0.0f, 8.0f));

			DrawSectionLabel(ICON_FA_NOTE_STICKY, "Execution notes");
			DrawWrappedPanel("##ReviewNotesRO", m_ReviewDeliverable.ExecutionNotes, 96.0f);

			ImGui::Dummy(ImVec2(0.0f, 10.0f));

			DrawSectionLabel(ICON_FA_FOLDER_OPEN, "File path");
			DrawWrappedPanel("##ReviewPathsRO", m_ReviewDeliverable.FilePath, 56.0f);

			ImGui::Dummy(ImVec2(0.0f, 10.0f));

			DrawSectionLabel(ICON_FA_CODE, "Code snippet");
			DrawCodePanel("##ReviewSnippetRO", m_ReviewDeliverable.CodeSnippet, 160.0f);
		}
		else if (m_ReviewDeliverableLoaded && !m_ReviewDeliverableFound)
		{
			ImGui::PushStyleColor(ImGuiCol_ChildBg, cardBg);
			ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
			ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 14.0f));
			if (ImGui::BeginChild("##ReviewNoDeliverable", ImVec2(fieldWidth, 56.0f),
				ImGuiChildFlags_Borders))
			{
				ImGui::PushStyleColor(ImGuiCol_Text, mutedText);
				ImGui::TextWrapped("%s  No deliverable was found for this task. You can still leave a review comment and reject so the member can submit.",
					ICON_FA_CIRCLE_INFO);
				ImGui::PopStyleColor();
			}
			ImGui::EndChild();
			ImGui::PopStyleVar(2);
			ImGui::PopStyleColor(2);
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Text, mutedText);
			ImGui::TextUnformatted("Loading submission…");
			ImGui::PopStyleColor();
		}

		ImGui::Dummy(ImVec2(0.0f, 14.0f));
		{
			ImVec2 p = ImGui::GetCursorScreenPos();
			drawList->AddLine(p, ImVec2(p.x + fieldWidth, p.y), ImGui::GetColorU32(borderColor), 1.0f);
			ImGui::Dummy(ImVec2(fieldWidth, 1.0f));
		}
		ImGui::Dummy(ImVec2(0.0f, 12.0f));

		// --- Leader feedback ---
		ImGui::PushStyleColor(ImGuiCol_Text, cyanColor);
		ImGui::Text(ICON_FA_COMMENT_DOTS "  Your review");
		ImGui::PopStyleColor();
		ImGui::Dummy(ImVec2(0.0f, 2.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, mutedText);
		ImGui::TextWrapped("Required. Approve marks the task Done; Reject returns it to In Progress for resubmit.");
		ImGui::PopStyleColor();
		ImGui::Dummy(ImVec2(0.0f, 6.0f));

		ImGui::PushStyleColor(ImGuiCol_FrameBg, fieldBg);
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(35.0f / 255.0f, 38.0f / 255.0f, 38.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(45.0f / 255.0f, 48.0f / 255.0f, 48.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 10.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

		if (ImGui::IsWindowAppearing())
			ImGui::SetKeyboardFocusHere();
		ImGui::InputTextMultiline("##ReviewComment", m_ReviewComment, IM_ARRAYSIZE(m_ReviewComment),
			ImVec2(fieldWidth, 96.0f));

		ImGui::PopStyleVar(3);
		ImGui::PopStyleColor(5);

		ImGui::Dummy(ImVec2(0.0f, 18.0f));

		const float gap = 10.0f;
		const float btnW = (fieldWidth - gap * 2.0f) / 3.0f;

		// Cancel
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.06f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.1f));
		ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		if (ImGui::Button("Cancel##ReviewTask", ImVec2(btnW, 38.0f)))
		{
			ResetReviewForm();
			ImGui::CloseCurrentPopup();
		}
		ImGui::PopStyleColor(5);
		ImGui::PopStyleVar(2);

		ImGui::SameLine(0, gap);

		// Reject
		ImGui::PushStyleColor(ImGuiCol_Button, redColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(255.0f / 255.0f, 76.0f / 255.0f, 76.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(200.0f / 255.0f, 40.0f / 255.0f, 40.0f / 255.0f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		if (ImGui::Button(ICON_FA_XMARK "  Reject##ReviewTask", ImVec2(btnW, 38.0f)))
		{
			if (m_ReviewTaskId <= 0)
			{
				TrackingTool::Application::Get().PushNotification("No task selected.", NotificationType::Error);
			}
			else
			{
				std::string message;
				if (TrackingTool::ProjectService::ReviewTask(
					projectId, m_ReviewTaskId, false, m_ReviewComment, message))
				{
					TrackingTool::Application::Get().PushNotification(message, NotificationType::Info);
					EnsureTasksLoaded(projectId, true);
					ResetReviewForm();
					ImGui::CloseCurrentPopup();
				}
				else
				{
					TrackingTool::Application::Get().PushNotification(message, NotificationType::Error);
				}
			}
		}
		ImGui::PopStyleColor(4);
		ImGui::PopStyleVar();

		ImGui::SameLine(0, gap);

		// Approve
		ImGui::PushStyleColor(ImGuiCol_Button, greenColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(96.0f / 255.0f, 195.0f / 255.0f, 100.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(56.0f / 255.0f, 142.0f / 255.0f, 60.0f / 255.0f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		if (ImGui::Button(ICON_FA_CHECK "  Approve##ReviewTask", ImVec2(btnW, 38.0f)))
		{
			if (m_ReviewTaskId <= 0)
			{
				TrackingTool::Application::Get().PushNotification("No task selected.", NotificationType::Error);
			}
			else
			{
				std::string message;
				if (TrackingTool::ProjectService::ReviewTask(
					projectId, m_ReviewTaskId, true, m_ReviewComment, message))
				{
					TrackingTool::Application::Get().PushNotification(message, NotificationType::Info);
					EnsureTasksLoaded(projectId, true);
					ResetReviewForm();
					ImGui::CloseCurrentPopup();
				}
				else
				{
					TrackingTool::Application::Get().PushNotification(message, NotificationType::Error);
				}
			}
		}
		ImGui::PopStyleColor(4);
		ImGui::PopStyleVar();

		ImGui::EndPopup();
	}

	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor(2);
}

void TasksView::OnRender(const char* projectName, const char* createdDate, int projectId, bool isLeader)
{
	EnsureTasksLoaded(projectId, false);

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	float totalWidth = ImGui::GetContentRegionAvail().x;

	ImVec4 cyanColor = ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f);
	ImVec4 grayText = ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f);
	ImVec4 whiteText = ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f);
	ImVec4 borderColor = ImVec4(60.0f / 255.0f, 73.0f / 255.0f, 74.0f / 255.0f, 1.0f);

	const char* displayName = (projectName && projectName[0] != '\0') ? projectName : "Project";
	const char* displayDate = (createdDate && createdDate[0] != '\0') ? createdDate : "—";

	// Header
	ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
	ImGui::SetWindowFontScale(1.1f);
	ImGui::Text("%s", displayName);
	ImGui::SetWindowFontScale(1.0f);
	ImGui::PopStyleColor();

	ImGui::SameLine(0.0f, 24.0f);

	ImGui::PushStyleColor(ImGuiCol_Text, grayText);
	ImGui::Text("%s %s", ICON_FA_CALENDAR, displayDate);
	ImGui::PopStyleColor();

	if (isLeader)
	{
		const float btnWidth = 120.0f;
		ImGui::SameLine(totalWidth - btnWidth);

		ImGui::PushStyleColor(ImGuiCol_Button, cyanColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 193.0f / 255.0f, 201.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 153.0f / 255.0f, 161.0f / 255.0f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(18.0f / 255.0f, 20.0f / 255.0f, 20.0f / 255.0f, 1.0f));
		if (ImGui::Button(ICON_FA_PLUS " New Task", ImVec2(btnWidth, 32.0f)))
		{
			m_HasFormLookups = false;
			EnsureFormLookupsLoaded(projectId);
			ResetCreateForm();
			ImGui::OpenPopup("Create Task");
		}
		ImGui::PopStyleColor(4);
		ImGui::PopStyleVar();
	}

	RenderCreateTaskModal(projectId);
	RenderEditTaskModal(projectId);
	RenderDeleteTaskModal(projectId);
	RenderSubmitTaskModal(projectId);
	RenderReviewTaskModal(projectId);

	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	ImVec2 cursor = ImGui::GetCursorScreenPos();
	drawList->AddLine(cursor, ImVec2(cursor.x + totalWidth, cursor.y), ImGui::GetColorU32(borderColor), 1.0f);
	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	const ImVec4 overdueColor = ImVec4(244.0f / 255.0f, 67.0f / 255.0f, 54.0f / 255.0f, 1.0f);
	const ImVec4 overdueBg = ImVec4(244.0f / 255.0f, 67.0f / 255.0f, 54.0f / 255.0f, 0.16f);
	const ImVec4 cardBg = ImVec4(22.0f / 255.0f, 24.0f / 255.0f, 24.0f / 255.0f, 1.0f);
	const ImVec4 cardBgOverdue = ImVec4(40.0f / 255.0f, 22.0f / 255.0f, 22.0f / 255.0f, 1.0f);
	const ImVec4 mutedBorder = ImVec4(40.0f / 255.0f, 43.0f / 255.0f, 43.0f / 255.0f, 1.0f);

	auto DrawFilledBadge = [&](const char* label, ImVec4 bgColor, ImVec4 textColor, float width, float height = 20.0f) {
		const ImVec2 pos = ImGui::GetCursorScreenPos();
		drawList->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), ImGui::GetColorU32(bgColor), 4.0f);

		const ImVec2 textSize = ImGui::CalcTextSize(label);
		const float fontSize = ImGui::GetFontSize() * 0.78f;
		const float textW = textSize.x * 0.78f;
		drawList->AddText(ImGui::GetFont(), fontSize,
			ImVec2(pos.x + (width - textW) * 0.5f, pos.y + (height - fontSize) * 0.5f),
			ImGui::GetColorU32(textColor), label);

		ImGui::Dummy(ImVec2(width, height));
	};

	auto DrawRowSeparator = [&]() {
		ImGui::Dummy(ImVec2(0.0f, 6.0f));
		ImVec2 p = ImGui::GetCursorScreenPos();
		drawList->AddLine(p, ImVec2(p.x + totalWidth, p.y), ImGui::GetColorU32(mutedBorder), 1.0f);
		ImGui::Dummy(ImVec2(0.0f, 6.0f));
	};

	if (m_Tasks.empty())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		if (isLeader)
			ImGui::TextUnformatted("No tasks yet. Create one with NEW TASK (a milestone is required).");
		else
			ImGui::TextUnformatted("No tasks yet. Ask a project leader to create one.");
		ImGui::PopStyleColor();
		return;
	}

	// m_Tasks is ordered by milestone — count group size by scanning forward.
	auto CountTasksInMilestone = [&](int milestoneId, size_t fromIndex) -> size_t {
		size_t count = 0;
		for (size_t i = fromIndex; i < m_Tasks.size(); ++i)
		{
			if (m_Tasks[i].MilestoneId != milestoneId)
				break;
			++count;
		}
		return count;
	};

	int lastMilestoneId = -1;
	bool groupCollapsed = false;
	// Day-cached C string — no allocation on the render path.
	const char* todayForOverdue = TrackingTool::Utils::GetTodayMmDdYyyy();

	for (size_t taskIndex = 0; taskIndex < m_Tasks.size(); ++taskIndex)
	{
		const TrackingTool::TaskInfo& task = m_Tasks[taskIndex];

		if (task.MilestoneId != lastMilestoneId)
		{
			lastMilestoneId = task.MilestoneId;
			groupCollapsed = m_CollapsedMilestoneIds.count(task.MilestoneId) > 0;
			const size_t groupTaskCount = CountTasksInMilestone(task.MilestoneId, taskIndex);

			// Count overdue in this group for a subtle header hint.
			size_t overdueInGroup = 0;
			for (size_t i = taskIndex; i < m_Tasks.size() && m_Tasks[i].MilestoneId == task.MilestoneId; ++i)
			{
				if (IsTaskOverdue(m_Tasks[i], todayForOverdue))
					++overdueInGroup;
			}

			const char* chevron = groupCollapsed ? ICON_FA_CHEVRON_RIGHT : ICON_FA_CHEVRON_DOWN;
			const char* msName = task.MilestoneName.empty() ? "Milestone" : task.MilestoneName.c_str();

			char headerLabel[360];
			if (overdueInGroup > 0)
			{
				std::snprintf(headerLabel, sizeof(headerLabel), "%s  %s  (%zu)  ·  %zu overdue",
					chevron, msName, groupTaskCount, overdueInGroup);
			}
			else
			{
				std::snprintf(headerLabel, sizeof(headerLabel), "%s  %s  (%zu)",
					chevron, msName, groupTaskCount);
			}

			ImGui::PushID(task.MilestoneId);
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 0.12f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 0.2f));
			ImGui::PushStyleColor(ImGuiCol_Text, overdueInGroup > 0 ? overdueColor : cyanColor);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));

			const ImVec2 labelSize = ImGui::CalcTextSize(headerLabel);
			if (ImGui::Button(headerLabel, ImVec2(labelSize.x + 16.0f, 0.0f)))
			{
				if (groupCollapsed)
					m_CollapsedMilestoneIds.erase(task.MilestoneId);
				else
					m_CollapsedMilestoneIds.insert(task.MilestoneId);
				groupCollapsed = !groupCollapsed;
			}
			if (ImGui::IsItemHovered())
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

			ImGui::PopStyleVar(2);
			ImGui::PopStyleColor(4);
			ImGui::PopID();

			DrawRowSeparator();
		}

		if (groupCollapsed)
			continue;

		const bool overdue = IsTaskOverdue(task, todayForOverdue);
		const bool isBlocked = !task.DependenciesSatisfied && task.Status != "done";
		const char* statusLabel = ToBadgeLabel(task.Status);
		ImVec4 statusBg, statusText;
		StatusBadgeColors(task.Status, statusBg, statusText, grayText, cyanColor);
		const float statusWidth = BadgeWidthForLabel(statusLabel, 70.0f);

		const char* priorityLabel = PriorityDisplayLabel(task.Priority);
		ImVec4 priorityBg, priorityText;
		PriorityBadgeColors(task.Priority, priorityBg, priorityText);
		const ImVec4 priorityAccent = PriorityAccentColor(task.Priority);
		const float priorityWidth = BadgeWidthForLabel(priorityLabel, 58.0f);

		const float overdueWidth = overdue ? BadgeWidthForLabel("Overdue", 68.0f) : 0.0f;
		const float blockedWidth = isBlocked ? BadgeWidthForLabel("Blocked", 68.0f) : 0.0f;
		const ImVec4 blockedColor = ImVec4(255.0f / 255.0f, 152.0f / 255.0f, 0.0f / 255.0f, 1.0f);
		const ImVec4 blockedBg = ImVec4(255.0f / 255.0f, 152.0f / 255.0f, 0.0f / 255.0f, 0.18f);
		const float actionsWidth = isLeader ? 64.0f : 0.0f;
		const float rightPad = 8.0f;
		const float badgeGap = 6.0f;

		const std::string& currentUser = TrackingTool::AuthService::GetLoggedInUser();
		const bool isAssignee = !currentUser.empty()
			&& !task.AssignedMemberName.empty()
			&& task.AssignedMemberName == currentUser;
		// Accept only when pending AND all prerequisite tasks are Done.
		const bool canAccept = isAssignee && task.Status == "pending" && task.DependenciesSatisfied;
		const bool canSubmit = isAssignee && task.Status == "in progress" && task.DependenciesSatisfied;
		const bool canReview = isLeader && task.Status == "under review";
		const bool showBlockedHint = isAssignee && task.Status == "pending" && !task.DependenciesSatisfied;
		const bool showMemberAction = canAccept || canSubmit || showBlockedHint;
		const bool showReviewAction = canReview;
		const bool showCardAction = showMemberAction || showReviewAction;
		const float memberActionH = showCardAction ? 32.0f : 0.0f;
		const bool hasReviewComment = !task.ReviewComment.empty();
		const bool hasDependencies = !task.Dependencies.empty();

		// Card layout (all positions in screen space from cardMin).
		const float indent = 24.0f;
		const float cardWidth = totalWidth - indent;
		const float cardPadX = 12.0f;
		const float cardPadY = 10.0f;
		const float accentBarW = 4.0f;

		const bool hasDescription = !task.Description.empty();
		const float lineH = ImGui::GetTextLineHeight();
		const float cardHeight = cardPadY * 2.0f + lineH + 6.0f + lineH
			+ (hasDependencies ? (lineH + 4.0f) : 0.0f)
			+ (hasDescription ? (lineH + 4.0f) : 0.0f)
			+ (hasReviewComment ? (lineH + 4.0f) : 0.0f)
			+ (showCardAction ? (8.0f + memberActionH) : 0.0f);

		ImGui::PushID(task.Id);

		// Indent card from the left of the content region.
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent);
		const ImVec2 cardMin = ImGui::GetCursorScreenPos();
		const ImVec2 cardMax = ImVec2(cardMin.x + cardWidth, cardMin.y + cardHeight);

		const float contentLeftX = cardMin.x + accentBarW + cardPadX;
		const float contentRightX = cardMin.x + cardWidth - cardPadX;

		// Background card
		drawList->AddRectFilled(cardMin, cardMax,
			ImGui::GetColorU32(overdue ? cardBgOverdue : cardBg), 6.0f);
		drawList->AddRect(cardMin, cardMax,
			ImGui::GetColorU32(overdue ? ImVec4(overdueColor.x, overdueColor.y, overdueColor.z, 0.35f) : mutedBorder),
			6.0f, 0, 1.0f);

		// Left accent bar = priority color (red when overdue)
		drawList->AddRectFilled(
			ImVec2(cardMin.x, cardMin.y),
			ImVec2(cardMin.x + accentBarW, cardMax.y),
			ImGui::GetColorU32(overdue ? overdueColor : priorityAccent),
			6.0f, ImDrawFlags_RoundCornersLeft);

		// --- Title row ---
		const float titleY = cardMin.y + cardPadY;
		ImGui::SetCursorScreenPos(ImVec2(contentLeftX, titleY));

		// Priority chip
		DrawFilledBadge(priorityLabel, priorityBg, priorityText, priorityWidth, 18.0f);

		// Right-side stack: [status] [blocked?] [overdue?] [actions]
		float cursorRight = contentRightX;
		if (isLeader)
			cursorRight -= actionsWidth;

		if (overdue)
		{
			cursorRight -= overdueWidth;
			ImGui::SetCursorScreenPos(ImVec2(cursorRight, titleY + 1.0f));
			DrawFilledBadge("Overdue", overdueBg, overdueColor, overdueWidth, 18.0f);
			cursorRight -= badgeGap;
		}

		if (isBlocked)
		{
			cursorRight -= blockedWidth;
			ImGui::SetCursorScreenPos(ImVec2(cursorRight, titleY + 1.0f));
			DrawFilledBadge("Blocked", blockedBg, blockedColor, blockedWidth, 18.0f);
			cursorRight -= badgeGap;
		}

		cursorRight -= statusWidth;
		ImGui::SetCursorScreenPos(ImVec2(cursorRight, titleY + 1.0f));
		DrawFilledBadge(statusLabel, statusBg, statusText, statusWidth, 18.0f);

		// Task name between priority chip and right badges (single line, ellipsis).
		{
			const float nameLeft = contentLeftX + priorityWidth + 10.0f;
			const float nameRight = cursorRight - badgeGap;
			const float nameMaxW = nameRight - nameLeft;

			char namePreview[256];
			FormatEllipsizedLine(namePreview, sizeof(namePreview), task.Name.c_str(),
				nameMaxW > 20.0f ? nameMaxW : 20.0f);

			ImGui::SetCursorScreenPos(ImVec2(nameLeft, titleY));
			ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
			ImGui::TextUnformatted(namePreview);
			ImGui::PopStyleColor();
		}

		if (isLeader)
		{
			ImGui::SetCursorScreenPos(ImVec2(contentRightX - actionsWidth + 4.0f, titleY));

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 0.18f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 0.3f));
			ImGui::PushStyleColor(ImGuiCol_Text, grayText);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 2.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);

			if (ImGui::SmallButton(ICON_FA_PEN_TO_SQUARE "##edit"))
			{
				m_HasFormLookups = false;
				EnsureFormLookupsLoaded(projectId);
				FillFormFromTask(task);
				m_OpenEditModal = true;
			}
			if (ImGui::IsItemHovered())
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

			ImGui::SameLine(0.0f, 6.0f);

			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(238.0f / 255.0f, 56.0f / 255.0f, 56.0f / 255.0f, 0.2f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(238.0f / 255.0f, 56.0f / 255.0f, 56.0f / 255.0f, 0.35f));
			if (ImGui::SmallButton(ICON_FA_TRASH "##delete"))
			{
				m_PendingDeleteTaskId = task.Id;
				m_PendingDeleteTaskName = task.Name;
				m_OpenDeleteModal = true;
			}
			if (ImGui::IsItemHovered())
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			ImGui::PopStyleColor(2);

			ImGui::PopStyleVar(2);
			ImGui::PopStyleColor(4);
		}

		// --- Meta row: assignee (left) · hours + deadline (right-aligned inside card) ---
		const float metaY = cardMin.y + cardPadY + lineH + 6.0f;

		char hoursBuf[32] = "";
		char deadlineBuf[48] = "";
		if (task.EstimatedHours > 0.0f)
			std::snprintf(hoursBuf, sizeof(hoursBuf), "%s %.1fh", ICON_FA_CLOCK, static_cast<double>(task.EstimatedHours));
		if (!task.Deadline.empty())
		{
			std::snprintf(deadlineBuf, sizeof(deadlineBuf), "%s %s",
				overdue ? ICON_FA_CALENDAR_XMARK : ICON_FA_CALENDAR,
				task.Deadline.c_str());
		}

		const float hoursW = hoursBuf[0] ? ImGui::CalcTextSize(hoursBuf).x : 0.0f;
		const float deadlineW = deadlineBuf[0] ? ImGui::CalcTextSize(deadlineBuf).x : 0.0f;
		const float metaGap = 14.0f;

		// Right-align hours + deadline within the card so they never spill off-screen.
		float metaRightCursor = contentRightX;
		if (deadlineW > 0.0f)
		{
			metaRightCursor -= deadlineW;
			ImGui::SetCursorScreenPos(ImVec2(metaRightCursor, metaY));
			ImGui::PushStyleColor(ImGuiCol_Text, overdue ? overdueColor : grayText);
			ImGui::TextUnformatted(deadlineBuf);
			ImGui::PopStyleColor();
			metaRightCursor -= metaGap;
		}
		if (hoursW > 0.0f)
		{
			metaRightCursor -= hoursW;
			// Keep hours left of the deadline block and still inside the card.
			if (metaRightCursor < contentLeftX)
				metaRightCursor = contentLeftX;
			ImGui::SetCursorScreenPos(ImVec2(metaRightCursor, metaY));
			ImGui::PushStyleColor(ImGuiCol_Text, grayText);
			ImGui::TextUnformatted(hoursBuf);
			ImGui::PopStyleColor();
			metaRightCursor -= metaGap;
		}

		// Assignee on the left; clip so it doesn't run into hours/deadline.
		{
			char assigneeBuf[160];
			if (!task.AssignedMemberName.empty())
				std::snprintf(assigneeBuf, sizeof(assigneeBuf), "%s %s", ICON_FA_USER, task.AssignedMemberName.c_str());
			else
				std::snprintf(assigneeBuf, sizeof(assigneeBuf), "%s Unassigned", ICON_FA_USER);

			const float assigneeMaxW = metaRightCursor - contentLeftX;
			if (assigneeMaxW > 24.0f)
				EllipsizeToWidth(assigneeBuf, sizeof(assigneeBuf), assigneeMaxW);

			ImGui::SetCursorScreenPos(ImVec2(contentLeftX, metaY));
			ImGui::PushStyleColor(ImGuiCol_Text, grayText);
			ImGui::TextUnformatted(assigneeBuf);
			ImGui::PopStyleColor();
		}

		// Optional dependencies line, then description (truncated).
		float afterMetaY = metaY + lineH;
		if (hasDependencies)
		{
			const float depY = metaY + lineH + 4.0f;
			afterMetaY = depY + lineH;
			ImGui::SetCursorScreenPos(ImVec2(contentLeftX, depY));

			char depPreview[384];
			size_t depLen = 0;
			depPreview[0] = '\0';
			depLen = AppendCStr(depPreview, sizeof(depPreview), depLen, ICON_FA_LINK);
			depLen = AppendCStr(depPreview, sizeof(depPreview), depLen, " Depends on: ");
			for (size_t di = 0; di < task.Dependencies.size(); ++di)
			{
				if (di > 0)
					depLen = AppendCStr(depPreview, sizeof(depPreview), depLen, ", ");
				const TrackingTool::TaskDependencyInfo& dep = task.Dependencies[di];
				depLen = AppendCStr(depPreview, sizeof(depPreview), depLen,
					dep.DependsOnTaskName.empty() ? "(unnamed)" : dep.DependsOnTaskName.c_str());
				if (!dep.IsDone)
					depLen = AppendCStr(depPreview, sizeof(depPreview), depLen, " (open)");
			}
			// Flatten any newlines from task names, then ellipsize to card width.
			for (size_t i = 0; i < depLen; ++i)
			{
				if (depPreview[i] == '\n' || depPreview[i] == '\r')
					depPreview[i] = ' ';
			}
			const float depMaxW = contentRightX - contentLeftX;
			EllipsizeToWidth(depPreview, sizeof(depPreview), depMaxW);

			ImGui::PushStyleColor(ImGuiCol_Text, isBlocked ? blockedColor
				: ImVec4(grayText.x, grayText.y, grayText.z, 0.9f));
			ImGui::TextUnformatted(depPreview);
			ImGui::PopStyleColor();
		}

		if (hasDescription)
		{
			const float descY = afterMetaY + 4.0f;
			afterMetaY = descY + lineH;
			ImGui::SetCursorScreenPos(ImVec2(contentLeftX, descY));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(grayText.x, grayText.y, grayText.z, 0.85f));
			char descPreview[320];
			FormatEllipsizedLine(descPreview, sizeof(descPreview), task.Description.c_str(),
				contentRightX - contentLeftX);
			ImGui::TextUnformatted(descPreview);
			ImGui::PopStyleColor();
		}

		// Show saved review feedback (especially useful after reject)
		if (hasReviewComment)
		{
			const float reviewY = afterMetaY + 4.0f;
			afterMetaY = reviewY + lineH;
			ImGui::SetCursorScreenPos(ImVec2(contentLeftX, reviewY));

			const ImVec4 reviewTint = (task.Status == "done")
				? ImVec4(76.0f / 255.0f, 175.0f / 255.0f, 80.0f / 255.0f, 1.0f)
				: ImVec4(255.0f / 255.0f, 183.0f / 255.0f, 77.0f / 255.0f, 1.0f);

			char reviewPreview[320];
			size_t reviewLen = 0;
			reviewPreview[0] = '\0';
			reviewLen = AppendCStr(reviewPreview, sizeof(reviewPreview), reviewLen, ICON_FA_COMMENT);
			reviewLen = AppendCStr(reviewPreview, sizeof(reviewPreview), reviewLen, " ");
			reviewLen = AppendCStr(reviewPreview, sizeof(reviewPreview), reviewLen, task.ReviewComment.c_str());
			for (size_t i = 0; i < reviewLen; ++i)
			{
				if (reviewPreview[i] == '\n' || reviewPreview[i] == '\r')
					reviewPreview[i] = ' ';
			}
			EllipsizeToWidth(reviewPreview, sizeof(reviewPreview), contentRightX - contentLeftX);

			ImGui::PushStyleColor(ImGuiCol_Text, reviewTint);
			ImGui::TextUnformatted(reviewPreview);
			ImGui::PopStyleColor();
		}

		// Actions: Accept / Submit (assignee) or Review (leader)
		if (showCardAction)
		{
			const float actionY = afterMetaY + 8.0f;
			ImGui::SetCursorScreenPos(ImVec2(contentLeftX, actionY));

			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 6.0f));

			if (canAccept)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, cyanColor);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 193.0f / 255.0f, 201.0f / 255.0f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 153.0f / 255.0f, 161.0f / 255.0f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(18.0f / 255.0f, 20.0f / 255.0f, 20.0f / 255.0f, 1.0f));
				if (ImGui::Button(ICON_FA_CIRCLE_CHECK " Accept Task##accept", ImVec2(140.0f, memberActionH)))
				{
					std::string message;
					if (TrackingTool::ProjectService::AcceptTask(projectId, task.Id, message))
					{
						TrackingTool::Application::Get().PushNotification(message, NotificationType::Info);
						EnsureTasksLoaded(projectId, true);
					}
					else
					{
						TrackingTool::Application::Get().PushNotification(message, NotificationType::Error);
					}
				}
				if (ImGui::IsItemHovered())
					ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
				ImGui::PopStyleColor(4);
			}
			else if (showBlockedHint)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(255.0f / 255.0f, 152.0f / 255.0f, 0.0f / 255.0f, 0.25f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(255.0f / 255.0f, 152.0f / 255.0f, 0.0f / 255.0f, 0.35f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(255.0f / 255.0f, 152.0f / 255.0f, 0.0f / 255.0f, 0.35f));
				ImGui::PushStyleColor(ImGuiCol_Text, blockedColor);
				ImGui::BeginDisabled();
				ImGui::Button(ICON_FA_LOCK " Waiting on prerequisites##blocked", ImVec2(220.0f, memberActionH));
				ImGui::EndDisabled();
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
				{
					ImGui::BeginTooltip();
					ImGui::TextUnformatted("Complete these tasks first:");
					for (const TrackingTool::TaskDependencyInfo& dep : task.Dependencies)
					{
						if (!dep.IsDone)
							ImGui::BulletText("%s", dep.DependsOnTaskName.c_str());
					}
					ImGui::EndTooltip();
				}
				ImGui::PopStyleColor(4);
			}
			else if (canSubmit)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, cyanColor);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 193.0f / 255.0f, 201.0f / 255.0f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 153.0f / 255.0f, 161.0f / 255.0f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(18.0f / 255.0f, 20.0f / 255.0f, 20.0f / 255.0f, 1.0f));
				if (ImGui::Button(ICON_FA_PAPER_PLANE " Submit Work##submit", ImVec2(140.0f, memberActionH)))
				{
					ResetSubmitForm();
					m_SubmitTaskId = task.Id;
					m_SubmitTaskName = task.Name;
					m_OpenSubmitModal = true;
				}
				if (ImGui::IsItemHovered())
					ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
				ImGui::PopStyleColor(4);
			}
			else if (canReview)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(255.0f / 255.0f, 193.0f / 255.0f, 7.0f / 255.0f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(255.0f / 255.0f, 213.0f / 255.0f, 79.0f / 255.0f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(245.0f / 255.0f, 160.0f / 255.0f, 0.0f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(18.0f / 255.0f, 20.0f / 255.0f, 20.0f / 255.0f, 1.0f));
				if (ImGui::Button(ICON_FA_CLIPBOARD_CHECK " Review##review", ImVec2(140.0f, memberActionH)))
				{
					OpenReviewForTask(projectId, task);
				}
				if (ImGui::IsItemHovered())
					ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
				ImGui::PopStyleColor(4);
			}

			ImGui::PopStyleVar(2);
		}

		// Advance layout past the card (window-local cursor).
		ImGui::SetCursorScreenPos(ImVec2(cardMin.x - indent, cardMax.y + 8.0f));
		ImGui::Dummy(ImVec2(0.0f, 0.0f));

		ImGui::PopID();
	}
}
