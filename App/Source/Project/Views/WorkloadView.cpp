#include "WorkloadView.h"

#include "imgui.h"
#include "IconsFontAwesome6.h"

#include "Platform/Application.h"
#include "Service/ProjectService.h"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <cfloat>

namespace
{
	const ImVec4 kCyanColor = ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f); // #00ADB5
	const ImVec4 kGrayText = ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f); // #BBC9CA
	const ImVec4 kWhiteText = ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f); // #E2E2E2
	const ImVec4 kBorderColor = ImVec4(60.0f / 255.0f, 73.0f / 255.0f, 74.0f / 255.0f, 1.0f); // #3C494A
	const ImVec4 kDarkBg = ImVec4(26.0f / 255.0f, 28.0f / 255.0f, 28.0f / 255.0f, 1.0f); // #1A1C1C
	const ImVec4 kCardBorderColor = ImVec4(56.0f / 255.0f, 57.0f / 255.0f, 58.0f / 255.0f, 1.0f); // #38393A
	const ImVec4 kMemberCardBg = ImVec4(22.0f / 255.0f, 24.0f / 255.0f, 25.0f / 255.0f, 1.0f);
	const ImVec4 kMemberCardBorder = ImVec4(48.0f / 255.0f, 54.0f / 255.0f, 56.0f / 255.0f, 1.0f);
	const ImVec4 kChipBg = ImVec4(36.0f / 255.0f, 40.0f / 255.0f, 41.0f / 255.0f, 1.0f);
	const ImVec4 kGreenBadge = ImVec4(0.0f, 200.0f / 255.0f, 100.0f / 255.0f, 1.0f);
	const ImVec4 kYellowBadge = ImVec4(230.0f / 255.0f, 180.0f / 255.0f, 40.0f / 255.0f, 1.0f);
	const ImVec4 kRedBadge = ImVec4(238.0f / 255.0f, 80.0f / 255.0f, 80.0f / 255.0f, 1.0f);
	const ImVec4 kProgressBg = ImVec4(40.0f / 255.0f, 43.0f / 255.0f, 43.0f / 255.0f, 1.0f);
	const ImVec4 kPendingAccent = ImVec4(160.0f / 255.0f, 170.0f / 255.0f, 175.0f / 255.0f, 1.0f);
	const ImVec4 kInProgressAccent = ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f);
	const ImVec4 kDoneAccent = ImVec4(0.0f, 200.0f / 255.0f, 100.0f / 255.0f, 1.0f);

	// Distinct avatar palette so members are easy to tell apart.
	const ImVec4 kAvatarPalette[] = {
		ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f),   // cyan
		ImVec4(120.0f / 255.0f, 140.0f / 255.0f, 255.0f / 255.0f, 1.0f), // periwinkle
		ImVec4(255.0f / 255.0f, 140.0f / 255.0f, 90.0f / 255.0f, 1.0f),  // coral
		ImVec4(100.0f / 255.0f, 210.0f / 255.0f, 150.0f / 255.0f, 1.0f), // mint
		ImVec4(220.0f / 255.0f, 120.0f / 255.0f, 200.0f / 255.0f, 1.0f), // pink
		ImVec4(255.0f / 255.0f, 200.0f / 255.0f, 80.0f / 255.0f, 1.0f),  // gold
		ImVec4(140.0f / 255.0f, 200.0f / 255.0f, 255.0f / 255.0f, 1.0f), // sky
		ImVec4(180.0f / 255.0f, 130.0f / 255.0f, 255.0f / 255.0f, 1.0f), // violet
	};
	constexpr int kAvatarPaletteCount = static_cast<int>(sizeof(kAvatarPalette) / sizeof(kAvatarPalette[0]));

	bool StatusIsPending(const std::string& status)
	{
		return status == "pending" || status == "backlog";
	}

	bool StatusIsInProgress(const std::string& status)
	{
		// Under review still occupies the assignee until the leader closes it.
		return status == "in progress" || status == "under review";
	}

	bool StatusIsDone(const std::string& status)
	{
		return status == "done";
	}

	bool IsLeaderRole(const std::string& role)
	{
		static constexpr char kLeader[] = "leader";
		if (role.size() != sizeof(kLeader) - 1)
			return false;
		for (size_t i = 0; i < role.size(); ++i)
		{
			if (static_cast<char>(std::tolower(static_cast<unsigned char>(role[i]))) != kLeader[i])
				return false;
		}
		return true;
	}

	void CapitalizeRole(const std::string& role, char* out, size_t outSize)
	{
		if (!out || outSize == 0)
			return;
		if (role.empty())
		{
			std::snprintf(out, outSize, "Member");
			return;
		}
		std::snprintf(out, outSize, "%s", role.c_str());
		out[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(out[0])));
	}

	ImVec4 BalanceColor(const char* label)
	{
		if (std::strcmp(label, "HEAVY") == 0)
			return kRedBadge;
		if (std::strcmp(label, "LIGHT") == 0)
			return kYellowBadge;
		if (std::strcmp(label, "IDLE") == 0)
			return kGrayText;
		return kGreenBadge; // BALANCED
	}

	ImU32 WithAlpha(ImVec4 c, float a)
	{
		c.w = a;
		return ImGui::GetColorU32(c);
	}

	// Stable color from member name so the same person keeps the same avatar tint.
	ImVec4 AvatarColorForName(const std::string& name)
	{
		unsigned int hash = 2166136261u;
		for (unsigned char ch : name)
		{
			hash ^= ch;
			hash *= 16777619u;
		}
		return kAvatarPalette[static_cast<int>(hash % static_cast<unsigned int>(kAvatarPaletteCount))];
	}

	void InitialsFromName(const std::string& name, char* out, size_t outSize)
	{
		if (!out || outSize < 2)
			return;
		out[0] = '\0';
		if (name.empty())
		{
			std::snprintf(out, outSize, "?");
			return;
		}

		char first = '\0';
		char second = '\0';
		bool inWord = false;
		int wordCount = 0;
		for (unsigned char ch : name)
		{
			if (std::isalnum(ch))
			{
				if (!inWord)
				{
					const char upper = static_cast<char>(std::toupper(ch));
					if (wordCount == 0)
						first = upper;
					else if (wordCount == 1)
						second = upper;
					++wordCount;
					inWord = true;
				}
			}
			else
			{
				inWord = false;
			}
		}

		if (first == '\0')
		{
			std::snprintf(out, outSize, "?");
			return;
		}
		if (second == '\0' || outSize < 3)
			std::snprintf(out, outSize, "%c", first);
		else
			std::snprintf(out, outSize, "%c%c", first, second);
	}

	void FormatCount(char* buf, size_t bufSize, int value)
	{
		std::snprintf(buf, bufSize, "%d", value);
	}

	void FormatHours(char* buf, size_t bufSize, float hours)
	{
		if (hours <= 0.0f)
			std::snprintf(buf, bufSize, "0h");
		else if (hours < 10.0f)
			std::snprintf(buf, bufSize, "%.1fh", static_cast<double>(hours));
		else
			std::snprintf(buf, bufSize, "%.0fh", static_cast<double>(hours));
	}

	// Soft filled pill badge (balance / role).
	float DrawPillBadge(ImDrawList* drawList, ImVec2 pos, const char* label, ImVec4 color, float fontScale = 0.78f)
	{
		const float padX = 10.0f;
		const float padY = 4.0f;
		const float fontSize = ImGui::GetFontSize() * fontScale;
		const ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, label);
		const float w = textSize.x + padX * 2.0f;
		const float h = textSize.y + padY * 2.0f;
		const ImVec2 max(pos.x + w, pos.y + h);

		drawList->AddRectFilled(pos, max, WithAlpha(color, 0.16f), 10.0f);
		drawList->AddRect(pos, max, WithAlpha(color, 0.85f), 10.0f, 0, 1.2f);
		drawList->AddText(ImGui::GetFont(), fontSize,
			ImVec2(pos.x + padX, pos.y + padY), ImGui::GetColorU32(color), label);
		return w;
	}

	// Status chip: colored left strip + label + value.
	void DrawStatChip(ImDrawList* drawList, ImVec2 pos, float width, float height,
		const char* label, const char* value, ImVec4 accent)
	{
		const ImVec2 max(pos.x + width, pos.y + height);
		drawList->AddRectFilled(pos, max, ImGui::GetColorU32(kChipBg), 6.0f);
		drawList->AddRectFilled(pos, ImVec2(pos.x + 3.5f, max.y), ImGui::GetColorU32(accent), 6.0f, ImDrawFlags_RoundCornersLeft);
		drawList->AddRect(pos, max, WithAlpha(accent, 0.35f), 6.0f, 0, 1.0f);

		const float labelSize = ImGui::GetFontSize() * 0.72f;
		const float valueSize = ImGui::GetFontSize() * 1.05f;
		drawList->AddText(ImGui::GetFont(), labelSize,
			ImVec2(pos.x + 12.0f, pos.y + 8.0f), ImGui::GetColorU32(kGrayText), label);
		drawList->AddText(ImGui::GetFont(), valueSize,
			ImVec2(pos.x + 12.0f, pos.y + 26.0f), ImGui::GetColorU32(kWhiteText), value);
	}

	void DrawAvatar(ImDrawList* drawList, ImVec2 center, float radius, const char* initials, ImVec4 color)
	{
		drawList->AddCircleFilled(center, radius, WithAlpha(color, 0.22f), 32);
		drawList->AddCircle(center, radius, ImGui::GetColorU32(color), 32, 1.8f);

		const float fontSize = radius * 0.85f;
		const ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, initials);
		drawList->AddText(ImGui::GetFont(), fontSize,
			ImVec2(center.x - textSize.x * 0.5f, center.y - textSize.y * 0.5f),
			ImGui::GetColorU32(color), initials);
	}
}

void WorkloadView::EnsureDataLoaded(int projectId)
{
	if (projectId <= 0)
	{
		m_Members.clear();
		m_Tasks.clear();
		m_MemberWorkloads.clear();
		m_LoadedProjectId = 0;
		m_LoadedTasksCacheGeneration = -1;
		m_HasLoaded = false;
		m_NotifiedLoadError = false;
		m_WorkloadDirty = true;
		m_PendingTasks = 0;
		m_InProgressTasks = 0;
		m_CompletedTasks = 0;
		m_AverageRemainingHours = 0.0f;
		return;
	}

	const int tasksGen = TrackingTool::ProjectService::GetTasksCacheGeneration();

	// Hot path: same project + task cache not invalidated → zero heap work.
	if (m_HasLoaded && m_LoadedProjectId == projectId
		&& m_LoadedTasksCacheGeneration == tasksGen)
	{
		return;
	}

	if (m_LoadedProjectId != projectId)
		m_NotifiedLoadError = false;

	m_LoadMessage.clear();
	m_MemberScratch.clear();
	m_TaskScratch.clear();

	const bool membersOk = TrackingTool::Database::GetProjectMembers(projectId, m_MemberScratch);
	const bool tasksOk = membersOk
		&& TrackingTool::ProjectService::GetProjectTasks(
			projectId, m_TaskScratch, m_LoadMessage, false);

	if (membersOk && tasksOk)
	{
		m_Members.swap(m_MemberScratch);
		m_Tasks.swap(m_TaskScratch);
		m_MemberScratch.clear(); // keeps capacity
		m_TaskScratch.clear();
		m_LoadedProjectId = projectId;
		m_LoadedTasksCacheGeneration = TrackingTool::ProjectService::GetTasksCacheGeneration();
		m_HasLoaded = true;
		m_NotifiedLoadError = false;
		m_WorkloadDirty = true;
		return;
	}

	if (!membersOk)
		m_LoadMessage = "Failed to load project members for workload.";

	if (m_LoadedProjectId != projectId)
	{
		m_Members.clear();
		m_Tasks.clear();
		m_MemberWorkloads.clear();
		m_LoadedProjectId = projectId;
		m_LoadedTasksCacheGeneration = -1;
		m_HasLoaded = false;
		m_WorkloadDirty = true;
		m_PendingTasks = 0;
		m_InProgressTasks = 0;
		m_CompletedTasks = 0;
		m_AverageRemainingHours = 0.0f;
	}

	if (!m_NotifiedLoadError)
	{
		TrackingTool::Application::Get().PushNotification(
			m_LoadMessage.empty() ? "Failed to load workload data." : m_LoadMessage,
			NotificationType::Error);
		m_NotifiedLoadError = true;
	}
}

void WorkloadView::RebuildWorkload()
{
	m_WorkloadDirty = false;
	m_MemberWorkloads.clear();
	m_PendingTasks = 0;
	m_InProgressTasks = 0;
	m_CompletedTasks = 0;
	m_AverageRemainingHours = 0.0f;

	m_MemberWorkloads.reserve(m_Members.size());
	for (const auto& member : m_Members)
	{
		MemberWorkload row;
		row.MembershipId = member.MembershipId;
		row.Name = member.Name;
		row.Role = member.Role;
		m_MemberWorkloads.push_back(std::move(row));
	}

	// Project-wide cards count every task; per-member rows only count assigned work.
	for (const auto& task : m_Tasks)
	{
		const float hours = task.EstimatedHours > 0.0f ? task.EstimatedHours : 0.0f;

		if (StatusIsPending(task.Status))
			++m_PendingTasks;
		else if (StatusIsInProgress(task.Status))
			++m_InProgressTasks;
		else if (StatusIsDone(task.Status))
			++m_CompletedTasks;

		if (task.AssignedMembershipId <= 0)
			continue;

		MemberWorkload* row = nullptr;
		for (auto& candidate : m_MemberWorkloads)
		{
			if (candidate.MembershipId == task.AssignedMembershipId)
			{
				row = &candidate;
				break;
			}
		}
		if (!row)
			continue;

		if (StatusIsPending(task.Status))
		{
			++row->PendingCount;
			row->PendingHours += hours;
		}
		else if (StatusIsInProgress(task.Status))
		{
			++row->InProgressCount;
			row->InProgressHours += hours;
		}
		else if (StatusIsDone(task.Status))
		{
			++row->DoneCount;
			row->DoneHours += hours;
		}
	}

	// Derive totals / progress; collect remaining hours for the team average.
	float remainingSum = 0.0f;
	int membersWithWork = 0;

	for (auto& row : m_MemberWorkloads)
	{
		row.ActiveTaskCount = row.PendingCount + row.InProgressCount;
		row.TotalTaskCount = row.ActiveTaskCount + row.DoneCount;
		row.RemainingHours = row.PendingHours + row.InProgressHours;
		row.TotalHours = row.RemainingHours + row.DoneHours;

		if (row.TotalHours > 0.0f)
			row.Progress = row.DoneHours / row.TotalHours;
		else if (row.TotalTaskCount > 0)
			row.Progress = static_cast<float>(row.DoneCount) / static_cast<float>(row.TotalTaskCount);
		else
			row.Progress = 0.0f;

		if (row.Progress < 0.0f)
			row.Progress = 0.0f;
		if (row.Progress > 1.0f)
			row.Progress = 1.0f;

		if (row.TotalTaskCount > 0)
		{
			remainingSum += row.RemainingHours;
			++membersWithWork;
		}
	}

	if (membersWithWork > 0)
		m_AverageRemainingHours = remainingSum / static_cast<float>(membersWithWork);

	// Balance vs team average remaining estimated hours.
	// HEAVY: > 25% above average; LIGHT: > 25% below; IDLE: no assigned tasks.
	constexpr float kHeavyFactor = 1.25f;
	constexpr float kLightFactor = 0.75f;

	for (auto& row : m_MemberWorkloads)
	{
		if (row.TotalTaskCount == 0)
		{
			row.BalanceLabel = "IDLE";
			continue;
		}

		// Single person with work cannot be relatively unbalanced.
		if (membersWithWork <= 1 || m_AverageRemainingHours <= 0.0f)
		{
			row.BalanceLabel = "BALANCED";
			continue;
		}

		if (row.RemainingHours > m_AverageRemainingHours * kHeavyFactor)
			row.BalanceLabel = "HEAVY";
		else if (row.RemainingHours < m_AverageRemainingHours * kLightFactor)
			row.BalanceLabel = "LIGHT";
		else
			row.BalanceLabel = "BALANCED";
	}
}

void WorkloadView::OnRender(const char* projectName, const char* createdDate, int projectId)
{
	EnsureDataLoaded(projectId);
	if (m_WorkloadDirty)
		RebuildWorkload();

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const float totalWidth = ImGui::GetContentRegionAvail().x;

	const char* displayName = (projectName && projectName[0] != '\0') ? projectName : "Project";
	const char* displayDate = (createdDate && createdDate[0] != '\0') ? createdDate : "—";

	// --- Header ---
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

	const ImVec2 sep = ImGui::GetCursorScreenPos();
	drawList->AddLine(sep, ImVec2(sep.x + totalWidth, sep.y), ImGui::GetColorU32(kBorderColor), 1.0f);
	ImGui::Dummy(ImVec2(0.0f, 20.0f));

	// --- Summary cards (project-wide task counts) ---
	const float cardWidth = (totalWidth - 40.0f) / 3.0f;
	const float cardHeight = 100.0f;
	const float cardsStartX = ImGui::GetCursorPosX();
	const float cardsStartY = ImGui::GetCursorPosY();

	char pendingBuf[16];
	char inProgressBuf[16];
	char completedBuf[16];
	FormatCount(pendingBuf, sizeof(pendingBuf), m_PendingTasks);
	FormatCount(inProgressBuf, sizeof(inProgressBuf), m_InProgressTasks);
	FormatCount(completedBuf, sizeof(completedBuf), m_CompletedTasks);

	auto DrawCard = [&](int index, const char* title, const char* value, ImVec4 accentColor, ImVec4 borderCol) {
		const float x = cardsStartX + static_cast<float>(index) * (cardWidth + 20.0f);
		ImGui::SetCursorPos(ImVec2(x, cardsStartY));
		const ImVec2 pos = ImGui::GetCursorScreenPos();

		drawList->AddRectFilled(pos, ImVec2(pos.x + cardWidth, pos.y + cardHeight), ImGui::GetColorU32(kDarkBg), 8.0f);
		drawList->AddRect(pos, ImVec2(pos.x + cardWidth, pos.y + cardHeight), ImGui::GetColorU32(borderCol), 8.0f, 0, 2.0f);

		drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 0.9f,
			ImVec2(pos.x + 20.0f, pos.y + 20.0f), ImGui::GetColorU32(accentColor), title);

		ImGui::SetWindowFontScale(2.5f);
		drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(),
			ImVec2(pos.x + 20.0f, pos.y + 45.0f), ImGui::GetColorU32(kWhiteText), value);
		ImGui::SetWindowFontScale(1.0f);
	};

	DrawCard(0, "PENDING TASKS", pendingBuf, kGrayText, kCardBorderColor);
	DrawCard(1, "IN PROGRESS", inProgressBuf, kCyanColor, kCyanColor);
	DrawCard(2, "COMPLETED", completedBuf, kCyanColor, kCardBorderColor);

	// DrawCard leaves the cursor on the last card; reset X+Y so content is left-aligned.
	ImGui::SetCursorPos(ImVec2(cardsStartX, cardsStartY + cardHeight + 32.0f));

	// Section title
	ImGui::PushStyleColor(ImGuiCol_Text, kWhiteText);
	ImGui::SetWindowFontScale(1.0f);
	ImGui::Text("%s  Team Workload", ICON_FA_USERS);
	ImGui::PopStyleColor();
	ImGui::PushStyleColor(ImGuiCol_Text, kGrayText);
	ImGui::SetWindowFontScale(0.85f);
	ImGui::Text("Each card shows one member's estimated-hour load and task mix.");
	ImGui::SetWindowFontScale(1.0f);
	ImGui::PopStyleColor();
	ImGui::Dummy(ImVec2(0.0f, 12.0f));

	if (m_MemberWorkloads.empty())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, kGrayText);
		ImGui::TextUnformatted(m_HasLoaded
			? "No members in this project yet."
			: "Unable to load workload data.");
		ImGui::PopStyleColor();
		return;
	}

	// --- Per-member cards ---
	constexpr float kPad = 18.0f;
	constexpr float kAvatarR = 22.0f;
	constexpr float kHeaderH = 64.0f;
	constexpr float kChipH = 54.0f;
	constexpr float kChipGap = 10.0f;
	constexpr float kProgressH = 8.0f;
	constexpr float kFooterH = 28.0f;
	constexpr float kCardInnerGap = 14.0f;
	// header + chips + progress block + footer + paddings
	const float memberCardH = kPad + kHeaderH + kCardInnerGap + kChipH + kCardInnerGap
		+ 22.0f + kProgressH + 8.0f + kFooterH + kPad;

	for (size_t i = 0; i < m_MemberWorkloads.size(); ++i)
	{
		const MemberWorkload& row = m_MemberWorkloads[i];
		ImGui::PushID(static_cast<int>(row.MembershipId != 0 ? row.MembershipId : static_cast<int>(i + 1)));

		ImGui::SetCursorPosX(cardsStartX);
		const ImVec2 cardPos = ImGui::GetCursorScreenPos();
		const ImVec2 cardMax(cardPos.x + totalWidth, cardPos.y + memberCardH);

		const ImVec4 avatarColor = AvatarColorForName(row.Name);
		const char* balance = row.BalanceLabel ? row.BalanceLabel : "IDLE";
		const ImVec4 balanceCol = BalanceColor(balance);
		const bool isLeader = IsLeaderRole(row.Role);

		// Card background + soft border
		drawList->AddRectFilled(cardPos, cardMax, ImGui::GetColorU32(kMemberCardBg), 10.0f);
		drawList->AddRect(cardPos, cardMax, ImGui::GetColorU32(kMemberCardBorder), 10.0f, 0, 1.2f);

		// Left accent strip (avatar color) — makes cards visually distinct at a glance
		drawList->AddRectFilled(
			cardPos,
			ImVec2(cardPos.x + 4.0f, cardMax.y),
			ImGui::GetColorU32(avatarColor),
			10.0f,
			ImDrawFlags_RoundCornersLeft);

		// Subtle top highlight
		drawList->AddRectFilledMultiColor(
			ImVec2(cardPos.x + 4.0f, cardPos.y),
			ImVec2(cardMax.x, cardPos.y + 3.0f),
			WithAlpha(avatarColor, 0.25f), WithAlpha(avatarColor, 0.05f),
			WithAlpha(avatarColor, 0.0f), WithAlpha(avatarColor, 0.0f));

		// --- Header: avatar + name/role + balance ---
		const ImVec2 avatarCenter(cardPos.x + kPad + 4.0f + kAvatarR, cardPos.y + kPad + kAvatarR);
		char initials[4];
		InitialsFromName(row.Name, initials, sizeof(initials));
		DrawAvatar(drawList, avatarCenter, kAvatarR, initials, avatarColor);

		const float textLeft = avatarCenter.x + kAvatarR + 14.0f;
		const float nameY = cardPos.y + kPad + 6.0f;
		drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 1.12f,
			ImVec2(textLeft, nameY), ImGui::GetColorU32(kWhiteText), row.Name.c_str());

		// Role pill under the name
		char roleBuf[64];
		CapitalizeRole(row.Role, roleBuf, sizeof(roleBuf));
		const ImVec4 roleColor = isLeader ? kCyanColor : kGrayText;
		const float rolePillY = nameY + ImGui::GetFontSize() * 1.12f + 6.0f;
		DrawPillBadge(drawList, ImVec2(textLeft, rolePillY), roleBuf, roleColor, 0.72f);

		// Balance badge top-right
		const float balFont = ImGui::GetFontSize() * 0.78f;
		const ImVec2 balTextSize = ImGui::GetFont()->CalcTextSizeA(balFont, FLT_MAX, 0.0f, balance);
		const float balW = balTextSize.x + 20.0f;
		const float balH = balTextSize.y + 8.0f;
		const ImVec2 balPos(cardMax.x - kPad - balW, cardPos.y + kPad + 8.0f);
		drawList->AddRectFilled(balPos, ImVec2(balPos.x + balW, balPos.y + balH), WithAlpha(balanceCol, 0.16f), 10.0f);
		drawList->AddRect(balPos, ImVec2(balPos.x + balW, balPos.y + balH), WithAlpha(balanceCol, 0.9f), 10.0f, 0, 1.2f);
		drawList->AddText(ImGui::GetFont(), balFont,
			ImVec2(balPos.x + 10.0f, balPos.y + 4.0f), ImGui::GetColorU32(balanceCol), balance);

		// Active tasks caption near balance (under it)
		char activeLine[48];
		std::snprintf(activeLine, sizeof(activeLine), "%d active task%s",
			row.ActiveTaskCount, row.ActiveTaskCount == 1 ? "" : "s");
		const ImVec2 activeSize = ImGui::CalcTextSize(activeLine);
		drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 0.82f,
			ImVec2(cardMax.x - kPad - activeSize.x, balPos.y + balH + 6.0f),
			ImGui::GetColorU32(kGrayText), activeLine);

		// --- Status chips (Pending / In Progress / Done) ---
		const float chipsTop = cardPos.y + kPad + kHeaderH + kCardInnerGap;
		const float chipsLeft = cardPos.x + kPad + 4.0f;
		const float chipsWidth = totalWidth - kPad * 2.0f - 4.0f;
		const float chipW = (chipsWidth - kChipGap * 2.0f) / 3.0f;

		char pendingVal[24];
		char inProgVal[24];
		char doneVal[24];
		if (row.TotalHours > 0.0f)
		{
			char hPend[16], hProg[16], hDone[16];
			FormatHours(hPend, sizeof(hPend), row.PendingHours);
			FormatHours(hProg, sizeof(hProg), row.InProgressHours);
			FormatHours(hDone, sizeof(hDone), row.DoneHours);
			std::snprintf(pendingVal, sizeof(pendingVal), "%d · %s", row.PendingCount, hPend);
			std::snprintf(inProgVal, sizeof(inProgVal), "%d · %s", row.InProgressCount, hProg);
			std::snprintf(doneVal, sizeof(doneVal), "%d · %s", row.DoneCount, hDone);
		}
		else
		{
			FormatCount(pendingVal, sizeof(pendingVal), row.PendingCount);
			FormatCount(inProgVal, sizeof(inProgVal), row.InProgressCount);
			FormatCount(doneVal, sizeof(doneVal), row.DoneCount);
		}

		DrawStatChip(drawList, ImVec2(chipsLeft, chipsTop), chipW, kChipH,
			"PENDING", pendingVal, kPendingAccent);
		DrawStatChip(drawList, ImVec2(chipsLeft + chipW + kChipGap, chipsTop), chipW, kChipH,
			"IN PROGRESS", inProgVal, kInProgressAccent);
		DrawStatChip(drawList, ImVec2(chipsLeft + (chipW + kChipGap) * 2.0f, chipsTop), chipW, kChipH,
			"DONE", doneVal, kDoneAccent);

		// --- Progress block ---
		const float progressTop = chipsTop + kChipH + kCardInnerGap;
		const float progressLeft = chipsLeft;
		const float progressWidth = chipsWidth;

		char progressLabel[64];
		const int pct = static_cast<int>(row.Progress * 100.0f + 0.5f);
		if (row.TotalHours > 0.0f)
		{
			char remH[16], totH[16];
			FormatHours(remH, sizeof(remH), row.RemainingHours);
			FormatHours(totH, sizeof(totH), row.TotalHours);
			std::snprintf(progressLabel, sizeof(progressLabel),
				"Completion  %d%%   ·   %s remaining of %s", pct, remH, totH);
		}
		else if (row.TotalTaskCount > 0)
		{
			std::snprintf(progressLabel, sizeof(progressLabel),
				"Completion  %d%%   ·   %d of %d tasks done",
				pct, row.DoneCount, row.TotalTaskCount);
		}
		else
		{
			std::snprintf(progressLabel, sizeof(progressLabel), "No tasks assigned yet");
		}

		drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 0.82f,
			ImVec2(progressLeft, progressTop), ImGui::GetColorU32(kGrayText), progressLabel);

		const float barY = progressTop + 20.0f;
		const ImVec2 barMin(progressLeft, barY);
		const ImVec2 barMax(progressLeft + progressWidth, barY + kProgressH);
		drawList->AddRectFilled(barMin, barMax, ImGui::GetColorU32(kProgressBg), 4.0f);

		const float fillW = progressWidth * row.Progress;
		if (fillW > 0.5f)
		{
			// Cyan fill that tips toward green as completion approaches 100%.
			ImVec4 fill = kCyanColor;
			fill.x = kCyanColor.x * (1.0f - row.Progress) + kDoneAccent.x * row.Progress;
			fill.y = kCyanColor.y * (1.0f - row.Progress) + kDoneAccent.y * row.Progress;
			fill.z = kCyanColor.z * (1.0f - row.Progress) + kDoneAccent.z * row.Progress;
			drawList->AddRectFilled(barMin, ImVec2(barMin.x + fillW, barMax.y),
				ImGui::GetColorU32(fill), 4.0f);
		}

		// Footer: total hours / tasks summary
		const float footerY = barMax.y + 10.0f;
		char footerLeft[64];
		char footerRight[48];
		if (row.TotalHours > 0.0f)
		{
			char totH[16];
			FormatHours(totH, sizeof(totH), row.TotalHours);
			std::snprintf(footerLeft, sizeof(footerLeft),
				"%s  %s total estimated", ICON_FA_CLOCK, totH);
		}
		else
		{
			std::snprintf(footerLeft, sizeof(footerLeft),
				"%s  No hour estimates", ICON_FA_CLOCK);
		}
		std::snprintf(footerRight, sizeof(footerRight),
			"%d task%s total", row.TotalTaskCount, row.TotalTaskCount == 1 ? "" : "s");

		drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 0.82f,
			ImVec2(progressLeft, footerY), ImGui::GetColorU32(kGrayText), footerLeft);
		const ImVec2 rightSize = ImGui::CalcTextSize(footerRight);
		drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 0.82f,
			ImVec2(cardMax.x - kPad - rightSize.x, footerY), ImGui::GetColorU32(kGrayText), footerRight);

		// Advance layout past the card (+ gap between members)
		ImGui::Dummy(ImVec2(totalWidth, memberCardH + 14.0f));
		ImGui::PopID();
	}
}
