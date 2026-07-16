#include "ChartView.h"

#include "imgui.h"
#include "IconsFontAwesome6.h"

#include "Platform/Application.h"
#include "Service/ProjectService.h"
#include "Utils/TimeUtils.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace
{
	constexpr float kSidebarWidth = 240.0f;
	constexpr float kMinDayWidth = 56.0f;
	constexpr float kRowHeight = 40.0f;
	constexpr int kMaxDayColumns = 90;
	constexpr int kDefaultWindowDays = 7;
	constexpr float kSidebarPadX = 12.0f;
	constexpr float kTaskIndentX = 28.0f;

	const ImVec4 kCyanColor = ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f); // #00ADB5
	const ImVec4 kGrayText = ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f); // #BBC9CA
	const ImVec4 kWhiteText = ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f); // #E2E2E2
	const ImVec4 kBorderColor = ImVec4(60.0f / 255.0f, 73.0f / 255.0f, 74.0f / 255.0f, 1.0f); // #3C494A
	const ImVec4 kGreenBar = ImVec4(0.0f, 255.0f / 255.0f, 100.0f / 255.0f, 0.4f);
	const ImVec4 kGreenText = ImVec4(0.0f, 255.0f / 255.0f, 100.0f / 255.0f, 1.0f);
	const ImVec4 kCyanBar = ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 0.35f);
	const ImVec4 kCyanText = ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f);
	const ImVec4 kRedBar = ImVec4(255.0f / 255.0f, 80.0f / 255.0f, 80.0f / 255.0f, 0.35f);
	const ImVec4 kRedText = ImVec4(255.0f / 255.0f, 120.0f / 255.0f, 120.0f / 255.0f, 1.0f);
	const ImVec4 kGrayBar = ImVec4(120.0f / 255.0f, 130.0f / 255.0f, 130.0f / 255.0f, 0.3f);

	bool IsValidDate(const char* s)
	{
		return TrackingTool::Utils::IsValidMmDdYyyy(s);
	}

	bool IsValidDate(const std::string& s)
	{
		return IsValidDate(s.c_str());
	}

	// Expands min/max range (fixed buffers, no heap).
	void ExpandRange(const char* date, char* rangeStart, char* rangeEnd, bool& hasRange)
	{
		if (!IsValidDate(date))
			return;

		if (!hasRange)
		{
			std::snprintf(rangeStart, 11, "%s", date);
			std::snprintf(rangeEnd, 11, "%s", date);
			hasRange = true;
			return;
		}

		if (TrackingTool::Utils::CompareMmDdYyyy(date, rangeStart) < 0)
			std::snprintf(rangeStart, 11, "%s", date);
		if (TrackingTool::Utils::CompareMmDdYyyy(date, rangeEnd) > 0)
			std::snprintf(rangeEnd, 11, "%s", date);
	}

	void ExpandRange(const std::string& date, char* rangeStart, char* rangeEnd, bool& hasRange)
	{
		ExpandRange(date.c_str(), rangeStart, rangeEnd, hasRange);
	}

	const char* TaskStatusLabel(const std::string& status)
	{
		if (status == "done")
			return "Done";
		if (status == "under review")
			return "Review";
		if (status == "in progress")
			return "In progress";
		if (status == "pending")
			return "Pending";
		if (status == "backlog")
			return "Backlog";
		return status.c_str();
	}

	void TaskBarColors(const std::string& status, bool overdue, ImVec4& barColor, ImVec4& textColor)
	{
		if (status == "done")
		{
			barColor = kGreenBar;
			textColor = kGreenText;
			return;
		}
		if (overdue)
		{
			barColor = kRedBar;
			textColor = kRedText;
			return;
		}
		if (status == "in progress" || status == "under review")
		{
			barColor = kCyanBar;
			textColor = kCyanText;
			return;
		}
		barColor = kGrayBar;
		textColor = kGrayText;
	}

	void DrawDiamond(ImDrawList* drawList, float tipX, float centerY, ImU32 color)
	{
		drawList->AddQuadFilled(
			ImVec2(tipX + 4.0f, centerY),
			ImVec2(tipX - 2.0f, centerY + 6.0f),
			ImVec2(tipX - 8.0f, centerY),
			ImVec2(tipX - 2.0f, centerY - 6.0f),
			color);
	}

	void DrawClippedBarText(ImDrawList* drawList, const ImVec2& barMin, const ImVec2& barMax,
		const char* text, ImU32 color, float padX = 8.0f)
	{
		if (!text || text[0] == '\0')
			return;

		const float maxW = barMax.x - barMin.x - padX * 2.0f;
		if (maxW < 8.0f)
			return;

		const ImVec2 textSize = ImGui::CalcTextSize(text);
		const float textY = barMin.y + (barMax.y - barMin.y - textSize.y) * 0.5f;
		const ImVec2 textPos(barMin.x + padX, textY);

		drawList->PushClipRect(barMin, barMax, true);
		drawList->AddText(textPos, color, text);
		drawList->PopClipRect();
	}

	float DateToX(const char* date, const char* rangeStart, int dayCount, float chartLeft, float dayWidth)
	{
		int idx = TrackingTool::Utils::DaysBetweenMmDdYyyy(rangeStart, date);
		if (idx < 0)
			idx = 0;
		if (idx > dayCount)
			idx = dayCount;
		return chartLeft + static_cast<float>(idx) * dayWidth;
	}

	// Left-aligned label in the sidebar (draw-list, no ImGui button centering).
	void DrawSidebarLabel(ImDrawList* drawList, float x, float rowTop, float maxWidth,
		const char* text, ImU32 color)
	{
		if (!text)
			return;
		const float textY = rowTop + (kRowHeight - ImGui::GetTextLineHeight()) * 0.5f;
		drawList->PushClipRect(ImVec2(x, rowTop), ImVec2(x + maxWidth, rowTop + kRowHeight), true);
		drawList->AddText(ImVec2(x, textY), color, text);
		drawList->PopClipRect();
	}

	bool TaskBelongsToMilestone(const TrackingTool::TaskInfo& task, int milestoneId)
	{
		return task.MilestoneId == milestoneId;
	}

	bool TaskIsOrphan(const TrackingTool::TaskInfo& task,
		const std::vector<TrackingTool::MilestoneInfo>& milestones)
	{
		for (const auto& m : milestones)
		{
			if (m.Id == task.MilestoneId)
				return false;
		}
		return true;
	}
}

void ChartView::EnsureDataLoaded(int projectId)
{
	if (projectId <= 0)
	{
		m_Milestones.clear();
		m_Tasks.clear();
		m_LoadedProjectId = 0;
		m_LoadedMilestonesCacheGeneration = -1;
		m_LoadedTasksCacheGeneration = -1;
		m_HasLoaded = false;
		m_NotifiedLoadError = false;
		m_TimelineDirty = true;
		m_CollapsedMilestoneIds.clear();
		m_DayCount = 0;
		m_DayColumns.clear();
		m_RangeStart[0] = '\0';
		m_RangeEnd[0] = '\0';
		return;
	}

	const int milestonesGen = TrackingTool::ProjectService::GetMilestonesCacheGeneration();
	const int tasksGen = TrackingTool::ProjectService::GetTasksCacheGeneration();

	// Hot path: same project + caches not invalidated → zero heap work.
	if (m_HasLoaded && m_LoadedProjectId == projectId
		&& m_LoadedMilestonesCacheGeneration == milestonesGen
		&& m_LoadedTasksCacheGeneration == tasksGen)
	{
		return;
	}

	if (m_LoadedProjectId != projectId)
	{
		m_CollapsedMilestoneIds.clear();
		m_NotifiedLoadError = false;
	}

	// Load into reusable scratch (capacity retained across reloads).
	// m_LoadMessage is reused so we do not allocate a second message string.
	const bool milestonesOk = TrackingTool::ProjectService::GetProjectMilestones(
		projectId, m_MilestoneScratch, m_LoadMessage, false);
	const bool tasksOk = milestonesOk
		&& TrackingTool::ProjectService::GetProjectTasks(
			projectId, m_TaskScratch, m_LoadMessage, false);

	if (milestonesOk && tasksOk)
	{
		m_Milestones.swap(m_MilestoneScratch);
		m_Tasks.swap(m_TaskScratch);
		m_MilestoneScratch.clear(); // keeps capacity
		m_TaskScratch.clear();
		m_LoadedProjectId = projectId;
		m_LoadedMilestonesCacheGeneration = TrackingTool::ProjectService::GetMilestonesCacheGeneration();
		m_LoadedTasksCacheGeneration = TrackingTool::ProjectService::GetTasksCacheGeneration();
		m_HasLoaded = true;
		m_NotifiedLoadError = false;
		m_TimelineDirty = true;
		return;
	}

	if (m_LoadedProjectId != projectId)
	{
		m_Milestones.clear();
		m_Tasks.clear();
		m_LoadedProjectId = projectId;
		m_LoadedMilestonesCacheGeneration = -1;
		m_LoadedTasksCacheGeneration = -1;
		m_HasLoaded = false;
		m_TimelineDirty = true;
	}

	if (!m_NotifiedLoadError)
	{
		TrackingTool::Application::Get().PushNotification(
			m_LoadMessage.empty() ? "Failed to load chart data." : m_LoadMessage,
			NotificationType::Error);
		m_NotifiedLoadError = true;
	}
}

void ChartView::RebuildTimeline()
{
	m_TimelineDirty = false;
	m_DayCount = 0;
	m_DayColumns.clear(); // capacity retained
	m_RangeStart[0] = '\0';
	m_RangeEnd[0] = '\0';

	const char* today = TrackingTool::Utils::GetTodayMmDdYyyy();
	char rangeStart[11] = {};
	char rangeEnd[11] = {};
	bool hasRange = false;

	// Task deadlines drive the chart range (milestones are headers only).
	for (const auto& t : m_Tasks)
		ExpandRange(t.Deadline, rangeStart, rangeEnd, hasRange);

	// Include parent milestone start when present so task bars from start→deadline fit.
	for (const auto& m : m_Milestones)
		ExpandRange(m.StartDate, rangeStart, rangeEnd, hasRange);

	ExpandRange(today, rangeStart, rangeEnd, hasRange);

	if (!hasRange)
	{
		char startBuf[11] = {};
		char endBuf[11] = {};
		if (!TrackingTool::Utils::AddDaysMmDdYyyy(today, -(kDefaultWindowDays / 2), startBuf, sizeof(startBuf))
			|| !TrackingTool::Utils::AddDaysMmDdYyyy(today, kDefaultWindowDays / 2, endBuf, sizeof(endBuf)))
		{
			return;
		}
		std::memcpy(rangeStart, startBuf, sizeof(rangeStart));
		std::memcpy(rangeEnd, endBuf, sizeof(rangeEnd));
	}
	else
	{
		char paddedStart[11] = {};
		char paddedEnd[11] = {};
		if (TrackingTool::Utils::AddDaysMmDdYyyy(rangeStart, -1, paddedStart, sizeof(paddedStart)))
			std::memcpy(rangeStart, paddedStart, sizeof(rangeStart));
		if (TrackingTool::Utils::AddDaysMmDdYyyy(rangeEnd, 1, paddedEnd, sizeof(paddedEnd)))
			std::memcpy(rangeEnd, paddedEnd, sizeof(rangeEnd));
	}

	int dayCount = TrackingTool::Utils::DaysBetweenMmDdYyyy(rangeStart, rangeEnd) + 1;
	if (dayCount < 1)
		dayCount = 1;
	if (dayCount > kMaxDayColumns)
	{
		char cappedEnd[11] = {};
		if (TrackingTool::Utils::AddDaysMmDdYyyy(rangeStart, kMaxDayColumns - 1, cappedEnd, sizeof(cappedEnd)))
			std::memcpy(rangeEnd, cappedEnd, sizeof(rangeEnd));
		dayCount = kMaxDayColumns;
	}

	std::memcpy(m_RangeStart, rangeStart, sizeof(m_RangeStart));
	std::memcpy(m_RangeEnd, rangeEnd, sizeof(m_RangeEnd));
	m_DayCount = dayCount;

	m_DayColumns.resize(static_cast<size_t>(dayCount));
	for (int i = 0; i < dayCount; ++i)
	{
		DayColumn& col = m_DayColumns[static_cast<size_t>(i)];
		col.Date[0] = '\0';
		col.Header[0] = '\0';
		if (!TrackingTool::Utils::AddDaysMmDdYyyy(m_RangeStart, i, col.Date, sizeof(col.Date)))
		{
			std::snprintf(col.Header, sizeof(col.Header), "?");
			continue;
		}
		if (!TrackingTool::Utils::FormatDayHeaderMmDdYyyy(col.Date, col.Header, sizeof(col.Header)))
			std::snprintf(col.Header, sizeof(col.Header), "%s", col.Date);
	}
}

void ChartView::OnRender(const char* projectName, const char* createdDate, int projectId)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const float totalWidth = ImGui::GetContentRegionAvail().x;

	const char* displayName = (projectName && projectName[0] != '\0') ? projectName : "Project";
	const char* displayDate = (createdDate && createdDate[0] != '\0') ? createdDate : "—";

	// --- Header Section ---
	ImGui::PushStyleColor(ImGuiCol_Text, kWhiteText);
	ImGui::SetWindowFontScale(1.1f);
	ImGui::Text("%s", displayName);
	ImGui::SetWindowFontScale(1.0f);
	ImGui::PopStyleColor();

	ImGui::SameLine(0.0f, 24.0f);

	ImGui::PushStyleColor(ImGuiCol_Text, kGrayText);
	ImGui::Text("%s %s", ICON_FA_CALENDAR, displayDate);
	ImGui::PopStyleColor();

	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	{
		const ImVec2 cursor = ImGui::GetCursorScreenPos();
		drawList->AddLine(cursor, ImVec2(cursor.x + totalWidth, cursor.y),
			ImGui::GetColorU32(kBorderColor), 1.0f);
	}
	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	EnsureDataLoaded(projectId);

	if (projectId <= 0)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, kGrayText);
		ImGui::TextUnformatted("Select a project from the Dashboard to open the chart.");
		ImGui::PopStyleColor();
		return;
	}

	if (!m_HasLoaded && m_Milestones.empty() && m_Tasks.empty())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, kGrayText);
		ImGui::TextUnformatted("Unable to load milestones and tasks for this project.");
		ImGui::PopStyleColor();
		return;
	}

	if (m_Milestones.empty() && m_Tasks.empty())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, kGrayText);
		ImGui::TextUnformatted("No milestones or tasks yet. Add them on the Milestones and Tasks tabs.");
		ImGui::PopStyleColor();
		return;
	}

	if (m_TimelineDirty)
		RebuildTimeline();

	if (m_DayCount <= 0 || m_DayColumns.empty())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, kGrayText);
		ImGui::TextUnformatted("Unable to build chart date range.");
		ImGui::PopStyleColor();
		return;
	}

	const char* today = TrackingTool::Utils::GetTodayMmDdYyyy();
	const int dayCount = m_DayCount;
	const char* rangeStart = m_RangeStart;
	const char* rangeEnd = m_RangeEnd;

	// Visible rows: milestone headers + tasks (no milestone bars).
	int visibleRows = 0;
	for (const auto& m : m_Milestones)
	{
		++visibleRows;
		if (m_CollapsedMilestoneIds.count(m.Id) != 0)
			continue;
		for (const auto& t : m_Tasks)
		{
			if (TaskBelongsToMilestone(t, m.Id))
				++visibleRows;
		}
	}
	{
		bool hasOrphans = false;
		int orphanCount = 0;
		for (const auto& t : m_Tasks)
		{
			if (TaskIsOrphan(t, m_Milestones))
			{
				hasOrphans = true;
				++orphanCount;
			}
		}
		if (hasOrphans)
		{
			++visibleRows; // "Other tasks" header
			if (m_CollapsedMilestoneIds.count(0) == 0)
				visibleRows += orphanCount;
		}
	}
	if (visibleRows < 1)
		visibleRows = 1;

	const float availWidth = ImGui::GetContentRegionAvail().x;
	const float availHeight = ImGui::GetContentRegionAvail().y;
	const float timelineWidth = static_cast<float>(dayCount) * kMinDayWidth;
	const float contentWidth = kSidebarWidth + timelineWidth;
	const float headerBlockHeight = 48.0f;
	const float bodyHeight = static_cast<float>(visibleRows) * kRowHeight + 4.0f;
	const float childHeight = (std::max)(120.0f, (std::min)(availHeight, headerBlockHeight + bodyHeight + 8.0f));

	ImGui::BeginChild("##ChartGanttScroll", ImVec2(availWidth, childHeight), false,
		ImGuiWindowFlags_HorizontalScrollbar);

	ImDrawList* childDraw = ImGui::GetWindowDrawList();
	const float dayWidth = kMinDayWidth;
	const ImVec2 origin = ImGui::GetCursorScreenPos();

	// --- Table header (left-aligned nomenclature title) ---
	ImGui::PushStyleColor(ImGuiCol_Text, kGrayText);
	ImGui::SetCursorScreenPos(ImVec2(origin.x + kSidebarPadX, origin.y + 8.0f));
	ImGui::TextUnformatted("TASK NOMENCLATURE");

	for (int i = 0; i < dayCount; ++i)
	{
		const DayColumn& col = m_DayColumns[static_cast<size_t>(i)];
		const bool isToday = col.Date[0] != '\0'
			&& TrackingTool::Utils::CompareMmDdYyyy(col.Date, today) == 0;

		const ImVec2 textSize = ImGui::CalcTextSize(col.Header);
		const float colCenterX = origin.x + kSidebarWidth + (static_cast<float>(i) + 0.5f) * dayWidth;
		const float textX = colCenterX - textSize.x * 0.5f;
		const float textY = origin.y + 4.0f;

		if (isToday)
			ImGui::PushStyleColor(ImGuiCol_Text, kCyanColor);
		ImGui::SetCursorScreenPos(ImVec2(textX, textY));
		ImGui::Text("%s", col.Header);
		if (isToday)
			ImGui::PopStyleColor();
	}
	ImGui::PopStyleColor();

	ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + headerBlockHeight));
	const ImVec2 gridTop = ImGui::GetCursorScreenPos();
	const float gridHeight = static_cast<float>(visibleRows) * kRowHeight;

	ImGui::Dummy(ImVec2(contentWidth, 0.0f));
	ImGui::SetCursorScreenPos(gridTop);

	childDraw->AddLine(gridTop, ImVec2(gridTop.x + contentWidth, gridTop.y),
		ImGui::GetColorU32(kBorderColor), 1.0f);

	for (int i = 0; i <= dayCount; ++i)
	{
		const float vx = gridTop.x + kSidebarWidth + static_cast<float>(i) * dayWidth;
		childDraw->AddLine(ImVec2(vx, gridTop.y), ImVec2(vx, gridTop.y + gridHeight),
			ImGui::GetColorU32(kBorderColor), 1.0f);
	}

	const int todayIdx = TrackingTool::Utils::DaysBetweenMmDdYyyy(rangeStart, today);
	if (todayIdx >= 0 && todayIdx < dayCount)
	{
		const float todayX = gridTop.x + kSidebarWidth + (static_cast<float>(todayIdx) + 0.5f) * dayWidth;
		childDraw->AddLine(ImVec2(todayX, gridTop.y), ImVec2(todayX, gridTop.y + gridHeight),
			ImGui::GetColorU32(kCyanColor), 2.0f);
		childDraw->AddCircleFilled(ImVec2(todayX, gridTop.y), 4.0f, ImGui::GetColorU32(kCyanColor));
	}

	const float chartLeft = gridTop.x + kSidebarWidth;
	int rowIndex = 0;

	auto DrawRowBaseline = [&](int row)
	{
		const float y = gridTop.y + static_cast<float>(row + 1) * kRowHeight;
		childDraw->AddLine(ImVec2(gridTop.x, y), ImVec2(gridTop.x + contentWidth, y),
			ImGui::GetColorU32(kBorderColor), 1.0f);
	};

	// Task bars only (milestones are hierarchy labels, not chart bars).
	auto DrawTaskGanttBar = [&](int row, const char* barStartDate, const char* barEndDate,
		const ImVec4& barColor, const ImVec4& textColor, const char* leftLabel, const char* rightLabel)
	{
		if (!barStartDate || !barEndDate || !IsValidDate(barStartDate) || !IsValidDate(barEndDate))
			return;

		const char* start = barStartDate;
		const char* end = barEndDate;
		if (TrackingTool::Utils::CompareMmDdYyyy(start, end) > 0)
		{
			start = barEndDate;
			end = barStartDate;
		}

		if (TrackingTool::Utils::CompareMmDdYyyy(end, rangeStart) < 0)
			return;
		if (TrackingTool::Utils::CompareMmDdYyyy(start, rangeEnd) > 0)
			return;

		const char* clipStart = start;
		const char* clipEnd = end;
		if (TrackingTool::Utils::CompareMmDdYyyy(start, rangeStart) < 0)
			clipStart = rangeStart;
		if (TrackingTool::Utils::CompareMmDdYyyy(end, rangeEnd) > 0)
			clipEnd = rangeEnd;

		float barStartX = DateToX(clipStart, rangeStart, dayCount, chartLeft, dayWidth);
		float barEndX = DateToX(clipEnd, rangeStart, dayCount, chartLeft, dayWidth) + dayWidth;

		barStartX += dayWidth * 0.08f;
		barEndX -= dayWidth * 0.08f;
		if (barEndX < barStartX + 8.0f)
			barEndX = barStartX + 8.0f;

		const float rowTop = gridTop.y + static_cast<float>(row) * kRowHeight;
		const ImVec2 barMin(barStartX, rowTop + 6.0f);
		const ImVec2 barMax(barEndX, rowTop + kRowHeight - 6.0f);

		childDraw->AddRectFilled(barMin, barMax, ImGui::GetColorU32(barColor), 4.0f);

		if (leftLabel && leftLabel[0] != '\0')
			DrawClippedBarText(childDraw, barMin, barMax, leftLabel, ImGui::GetColorU32(textColor), 8.0f);

		if (rightLabel && rightLabel[0] != '\0')
		{
			const ImVec2 rSize = ImGui::CalcTextSize(rightLabel);
			const float rx = barMax.x - rSize.x - 10.0f;
			const float ry = barMin.y + (barMax.y - barMin.y - rSize.y) * 0.5f;
			if (rx > barMin.x + 8.0f)
			{
				childDraw->PushClipRect(barMin, barMax, true);
				childDraw->AddText(ImVec2(rx, ry), ImGui::GetColorU32(textColor), rightLabel);
				childDraw->PopClipRect();
			}
		}

		DrawDiamond(childDraw, barMax.x, (barMin.y + barMax.y) * 0.5f, ImGui::GetColorU32(textColor));
	};

	// Invisible hit target + left-aligned label for collapsible milestone header.
	auto DrawMilestoneHeader = [&](int milestoneId, const char* name, int row, bool& outCollapsed)
	{
		const float rowTop = gridTop.y + static_cast<float>(row) * kRowHeight;
		outCollapsed = m_CollapsedMilestoneIds.count(milestoneId) != 0;
		const char* chevron = outCollapsed ? ICON_FA_CHEVRON_RIGHT : ICON_FA_CHEVRON_DOWN;

		char label[192];
		std::snprintf(label, sizeof(label), "%s  %s", chevron, name ? name : "");

		ImGui::PushID(milestoneId == 0 ? -1 : milestoneId);
		ImGui::SetCursorScreenPos(ImVec2(origin.x, rowTop));
		if (ImGui::InvisibleButton("##ms", ImVec2(kSidebarWidth, kRowHeight)))
		{
			if (outCollapsed)
				m_CollapsedMilestoneIds.erase(milestoneId);
			else
				m_CollapsedMilestoneIds.insert(milestoneId);
			outCollapsed = !outCollapsed;
		}
		if (ImGui::IsItemHovered())
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
		ImGui::PopID();

		DrawSidebarLabel(childDraw, origin.x + kSidebarPadX, rowTop,
			kSidebarWidth - kSidebarPadX - 4.0f, label, ImGui::GetColorU32(kCyanColor));
	};

	auto DrawTaskRow = [&](const TrackingTool::TaskInfo& task,
		const TrackingTool::MilestoneInfo* parentMilestone, int row)
	{
		const float rowTop = gridTop.y + static_cast<float>(row) * kRowHeight;

		// Left-aligned task name in sidebar.
		DrawSidebarLabel(childDraw, origin.x + kTaskIndentX, rowTop,
			kSidebarWidth - kTaskIndentX - 4.0f, task.Name.c_str(), ImGui::GetColorU32(kWhiteText));

		if (!IsValidDate(task.Deadline))
		{
			DrawRowBaseline(row);
			return;
		}

		const bool overdue = task.Status != "done"
			&& TrackingTool::Utils::CompareMmDdYyyy(task.Deadline.c_str(), today) < 0;

		ImVec4 barColor, textColor;
		TaskBarColors(task.Status, overdue, barColor, textColor);

		const char* barStart = task.Deadline.c_str();
		if (parentMilestone && IsValidDate(parentMilestone->StartDate)
			&& TrackingTool::Utils::CompareMmDdYyyy(parentMilestone->StartDate.c_str(), task.Deadline.c_str()) <= 0)
		{
			barStart = parentMilestone->StartDate.c_str();
		}

		char rightLabel[32];
		if (task.Status == "done")
			std::snprintf(rightLabel, sizeof(rightLabel), "100%%");
		else if (overdue)
			std::snprintf(rightLabel, sizeof(rightLabel), "Overdue");
		else
			std::snprintf(rightLabel, sizeof(rightLabel), "%s", TaskStatusLabel(task.Status));

		DrawTaskGanttBar(row, barStart, task.Deadline.c_str(),
			barColor, textColor, task.Name.c_str(), rightLabel);
		DrawRowBaseline(row);
	};

	// --- Rows: milestone headers (no bars) + task bars only ---
	for (const auto& milestone : m_Milestones)
	{
		bool collapsed = false;
		DrawMilestoneHeader(milestone.Id, milestone.Name.c_str(), rowIndex, collapsed);
		DrawRowBaseline(rowIndex);
		++rowIndex;

		if (collapsed)
			continue;

		for (const auto& task : m_Tasks)
		{
			if (!TaskBelongsToMilestone(task, milestone.Id))
				continue;
			DrawTaskRow(task, &milestone, rowIndex);
			++rowIndex;
		}
	}

	// Orphan tasks (no matching milestone) — no heap vector of pointers.
	{
		bool hasOrphans = false;
		for (const auto& t : m_Tasks)
		{
			if (TaskIsOrphan(t, m_Milestones))
			{
				hasOrphans = true;
				break;
			}
		}

		if (hasOrphans)
		{
			bool collapsed = false;
			DrawMilestoneHeader(0, "Other tasks", rowIndex, collapsed);
			DrawRowBaseline(rowIndex);
			++rowIndex;

			if (!collapsed)
			{
				for (const auto& task : m_Tasks)
				{
					if (!TaskIsOrphan(task, m_Milestones))
						continue;
					DrawTaskRow(task, nullptr, rowIndex);
					++rowIndex;
				}
			}
		}
	}

	ImGui::SetCursorScreenPos(ImVec2(origin.x, gridTop.y + gridHeight));
	ImGui::Dummy(ImVec2(contentWidth, 1.0f));

	ImGui::EndChild();
}
