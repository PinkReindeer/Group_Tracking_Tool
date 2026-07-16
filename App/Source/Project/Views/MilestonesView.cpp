#include "Database/Database.h"
#include "MilestonesView.h"

#include "imgui.h"
#include "IconsFontAwesome6.h"

#include "Platform/Application.h"
#include "Service/ProjectService.h"

#include <cstdio>
#include <cstring>

namespace
{

	const char* ToBadgeLabel(const std::string& status)
	{
		if (status == "completed")
			return "COMPLETED";
		if (status == "in progress")
			return "IN PROGRESS";
		if (status == "not started")
			return "NOT STARTED";
		return "UNKNOWN";
	}

	ImVec4 StatusBadgeColor(const std::string& status, const ImVec4& cyanColor,
		const ImVec4& grayText, const ImVec4& greenColor)
	{
		if (status == "completed")
			return greenColor;
		if (status == "in progress")
			return cyanColor;
		return grayText; // not started / unknown
	}

	float StatusBadgeWidth(const char* badgeLabel)
	{
		// Enough room for "IN PROGRESS" / "NOT STARTED" / "COMPLETED"
		const size_t len = std::strlen(badgeLabel);
		if (len >= 11)
			return 90.0f;
		return 80.0f;
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
}

void MilestonesView::EnsureMilestonesLoaded(int projectId, bool forceRefresh)
{
	if (projectId <= 0)
	{
		m_Milestones.clear();
		m_LoadedProjectId = 0;
		m_LoadedCacheGeneration = -1;
		m_HasLoaded = false;
		return;
	}

	const int cacheGen = TrackingTool::ProjectService::GetMilestonesCacheGeneration();

	// Hot path: already have this project's list and the service cache was not invalidated
	// (task create/update/delete/approve bumps the generation). Zero allocations.
	if (!forceRefresh && m_HasLoaded && m_LoadedProjectId == projectId
		&& m_LoadedCacheGeneration == cacheGen)
	{
		return;
	}

	// Load into reusable members (capacity retained across frames / reloads).
	if (TrackingTool::ProjectService::GetProjectMilestones(
		projectId, m_LoadScratch, m_LoadMessage, forceRefresh))
	{
		m_Milestones.swap(m_LoadScratch);
		m_LoadScratch.clear(); // keeps capacity; drops old elements after swap
		m_LoadedProjectId = projectId;
		m_LoadedCacheGeneration = TrackingTool::ProjectService::GetMilestonesCacheGeneration();
		m_HasLoaded = true;
	}
	else
	{
		const bool projectChanged = (m_LoadedProjectId != projectId);
		if (projectChanged)
		{
			m_Milestones.clear();
			m_LoadedProjectId = projectId;
			m_LoadedCacheGeneration = -1;
			m_HasLoaded = false;
		}
		// Notify once per failed load attempt for this project (not every frame).
		if (forceRefresh || projectChanged || !m_HasLoaded)
			TrackingTool::Application::Get().PushNotification(m_LoadMessage, NotificationType::Error);
	}
}

void MilestonesView::OpenEditForMilestone(const TrackingTool::MilestoneInfo& milestone)
{
	m_EditMilestoneId = milestone.Id;
	CopyToBuffer(m_EditMilestoneName, sizeof(m_EditMilestoneName), milestone.Name);
	CopyToBuffer(m_EditStartDate, sizeof(m_EditStartDate), milestone.StartDate);
	CopyToBuffer(m_EditEndDate, sizeof(m_EditEndDate), milestone.EndDate);
	m_OpenEditModal = true;
}

void MilestonesView::RenderCreateMilestoneModal(int projectId)
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

	if (ImGui::BeginPopupModal("Create Milestone", nullptr,
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
	{
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetWindowFontScale(1.2f);
		ImGui::Text(ICON_FA_FLAG " Create Milestone");
		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 8.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 350.0f);
		ImGui::TextWrapped("Dates must use MM-DD-YYYY. Progress starts at 0%%; status is set from the date window.");
		ImGui::PopTextWrapPos();
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 15.0f));

		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(15.0f / 255.0f, 16.0f / 255.0f, 16.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(35.0f / 255.0f, 38.0f / 255.0f, 38.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(45.0f / 255.0f, 48.0f / 255.0f, 48.0f / 255.0f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 10.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::Text("MILESTONE NAME");
		ImGui::PopStyleColor();

		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetNextItemWidth(350.0f);
		if (ImGui::IsWindowAppearing())
			ImGui::SetKeyboardFocusHere();
		ImGui::InputText("##MilestoneName", m_NewMilestoneName, IM_ARRAYSIZE(m_NewMilestoneName));
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 10.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::Text("START DATE (MM-DD-YYYY)");
		ImGui::PopStyleColor();

		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetNextItemWidth(350.0f);
		ImGui::InputTextWithHint("##MilestoneStart", "e.g. 07-14-2026", m_NewStartDate, IM_ARRAYSIZE(m_NewStartDate));
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 10.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::Text("END DATE (MM-DD-YYYY)");
		ImGui::PopStyleColor();

		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetNextItemWidth(350.0f);
		ImGui::InputTextWithHint("##MilestoneEnd", "e.g. 08-01-2026", m_NewEndDate, IM_ARRAYSIZE(m_NewEndDate));
		ImGui::PopStyleColor();

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(3);

		ImGui::Dummy(ImVec2(0.0f, 20.0f));

		const float modalBtnWidth = (350.0f - 10.0f) / 2.0f;

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		if (ImGui::Button("Cancel", ImVec2(modalBtnWidth, 36.0f)))
		{
			memset(m_NewMilestoneName, 0, sizeof(m_NewMilestoneName));
			memset(m_NewStartDate, 0, sizeof(m_NewStartDate));
			memset(m_NewEndDate, 0, sizeof(m_NewEndDate));
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
		if (ImGui::Button("Create", ImVec2(modalBtnWidth, 36.0f)))
		{
			std::string message;
			if (TrackingTool::ProjectService::CreateMilestone(
				projectId, m_NewMilestoneName, m_NewStartDate, m_NewEndDate, message))
			{
				TrackingTool::Application::Get().PushNotification(message, NotificationType::Info);
				EnsureMilestonesLoaded(projectId, true);

				memset(m_NewMilestoneName, 0, sizeof(m_NewMilestoneName));
				memset(m_NewStartDate, 0, sizeof(m_NewStartDate));
				memset(m_NewEndDate, 0, sizeof(m_NewEndDate));
				ImGui::CloseCurrentPopup();
			}
			else
			{
				TrackingTool::Application::Get().PushNotification(message, NotificationType::Error);
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

void MilestonesView::RenderEditMilestoneModal(int projectId)
{
	const ImVec4 cyanColor = ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f);
	const ImVec4 grayText = ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f);
	const ImVec4 whiteText = ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f);
	const ImVec4 borderColor = ImVec4(60.0f / 255.0f, 73.0f / 255.0f, 74.0f / 255.0f, 1.0f);
	const ImVec4 boxBgColor = ImVec4(26.0f / 255.0f, 28.0f / 255.0f, 28.0f / 255.0f, 1.0f);

	if (m_OpenEditModal)
	{
		ImGui::OpenPopup("Edit Milestone");
		m_OpenEditModal = false;
	}

	const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	ImGui::PushStyleColor(ImGuiCol_PopupBg, boxBgColor);
	ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 24.0f));

	if (ImGui::BeginPopupModal("Edit Milestone", nullptr,
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
	{
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetWindowFontScale(1.2f);
		ImGui::Text(ICON_FA_PEN_TO_SQUARE " Edit Milestone");
		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 8.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 350.0f);
		ImGui::TextWrapped("Dates must use MM-DD-YYYY. Completion is calculated from tasks under this milestone.");
		ImGui::PopTextWrapPos();
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 15.0f));

		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(15.0f / 255.0f, 16.0f / 255.0f, 16.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(35.0f / 255.0f, 38.0f / 255.0f, 38.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(45.0f / 255.0f, 48.0f / 255.0f, 48.0f / 255.0f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 10.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::Text("MILESTONE NAME");
		ImGui::PopStyleColor();

		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetNextItemWidth(350.0f);
		if (ImGui::IsWindowAppearing())
			ImGui::SetKeyboardFocusHere();
		ImGui::InputText("##EditMilestoneName", m_EditMilestoneName, IM_ARRAYSIZE(m_EditMilestoneName));
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 10.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::Text("START DATE (MM-DD-YYYY)");
		ImGui::PopStyleColor();

		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetNextItemWidth(350.0f);
		ImGui::InputTextWithHint("##EditMilestoneStart", "e.g. 07-14-2026", m_EditStartDate, IM_ARRAYSIZE(m_EditStartDate));
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 10.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::Text("END DATE (MM-DD-YYYY)");
		ImGui::PopStyleColor();

		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetNextItemWidth(350.0f);
		ImGui::InputTextWithHint("##EditMilestoneEnd", "e.g. 08-01-2026", m_EditEndDate, IM_ARRAYSIZE(m_EditEndDate));
		ImGui::PopStyleColor();

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(3);

		ImGui::Dummy(ImVec2(0.0f, 20.0f));

		const float modalBtnWidth = (350.0f - 10.0f) / 2.0f;

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		if (ImGui::Button("Cancel##EditMilestone", ImVec2(modalBtnWidth, 36.0f)))
		{
			m_EditMilestoneId = 0;
			memset(m_EditMilestoneName, 0, sizeof(m_EditMilestoneName));
			memset(m_EditStartDate, 0, sizeof(m_EditStartDate));
			memset(m_EditEndDate, 0, sizeof(m_EditEndDate));
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
		if (ImGui::Button("Save##EditMilestone", ImVec2(modalBtnWidth, 36.0f)))
		{
			std::string message;
			if (TrackingTool::ProjectService::UpdateMilestone(
				projectId, m_EditMilestoneId, m_EditMilestoneName, m_EditStartDate, m_EditEndDate, message))
			{
				TrackingTool::Application::Get().PushNotification(message, NotificationType::Info);
				EnsureMilestonesLoaded(projectId, true);

				m_EditMilestoneId = 0;
				memset(m_EditMilestoneName, 0, sizeof(m_EditMilestoneName));
				memset(m_EditStartDate, 0, sizeof(m_EditStartDate));
				memset(m_EditEndDate, 0, sizeof(m_EditEndDate));
				ImGui::CloseCurrentPopup();
			}
			else
			{
				TrackingTool::Application::Get().PushNotification(message, NotificationType::Error);
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

void MilestonesView::RenderDeleteMilestoneModal(int projectId)
{
	const ImVec4 grayText = ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f);
	const ImVec4 whiteText = ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f);
	const ImVec4 borderColor = ImVec4(60.0f / 255.0f, 73.0f / 255.0f, 74.0f / 255.0f, 1.0f);
	const ImVec4 boxBgColor = ImVec4(26.0f / 255.0f, 28.0f / 255.0f, 28.0f / 255.0f, 1.0f);
	const ImVec4 redColor = ImVec4(238.0f / 255.0f, 56.0f / 255.0f, 56.0f / 255.0f, 1.0f);

	if (m_OpenDeleteModal)
	{
		ImGui::OpenPopup("Delete Milestone");
		m_OpenDeleteModal = false;
	}

	const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	ImGui::PushStyleColor(ImGuiCol_PopupBg, boxBgColor);
	ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 24.0f));

	if (ImGui::BeginPopupModal("Delete Milestone", nullptr,
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
	{
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetWindowFontScale(1.2f);
		ImGui::Text(ICON_FA_TRASH " Delete Milestone");
		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 8.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 350.0f);
		ImGui::TextWrapped(
			"Are you sure you want to delete \"%s\"? All tasks under this milestone will also be removed. This cannot be undone.",
			m_PendingDeleteMilestoneName.empty() ? "this milestone" : m_PendingDeleteMilestoneName.c_str());
		ImGui::PopTextWrapPos();
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 20.0f));

		const float modalBtnWidth = (350.0f - 10.0f) / 2.0f;

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		if (ImGui::Button("Cancel##DeleteMilestone", ImVec2(modalBtnWidth, 36.0f)))
		{
			m_PendingDeleteMilestoneId = 0;
			m_PendingDeleteMilestoneName.clear();
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
		if (ImGui::Button("Delete##DeleteMilestone", ImVec2(modalBtnWidth, 36.0f)))
		{
			std::string message;
			if (TrackingTool::ProjectService::DeleteMilestone(projectId, m_PendingDeleteMilestoneId, message))
			{
				TrackingTool::Application::Get().PushNotification(message, NotificationType::Info);
				EnsureMilestonesLoaded(projectId, true);
				m_PendingDeleteMilestoneId = 0;
				m_PendingDeleteMilestoneName.clear();
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

void MilestonesView::OnRender(const char* projectName, const char* createdDate, int projectId, bool isLeader)
{
	EnsureMilestonesLoaded(projectId, false);

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	float totalWidth = ImGui::GetContentRegionAvail().x;

	ImVec4 cyanColor = ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f); // #00ADB5
	ImVec4 grayText = ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f); // #BBC9CA
	ImVec4 whiteText = ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f); // #E2E2E2
	ImVec4 borderColor = ImVec4(60.0f / 255.0f, 73.0f / 255.0f, 74.0f / 255.0f, 1.0f); // #3C494A
	ImVec4 greenColor = ImVec4(76.0f / 255.0f, 175.0f / 255.0f, 80.0f / 255.0f, 1.0f);

	const char* displayName = (projectName && projectName[0] != '\0') ? projectName : "Project";
	const char* displayDate = (createdDate && createdDate[0] != '\0') ? createdDate : "—";

	// --- Header Section ---
	ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
	ImGui::SetWindowFontScale(1.1f);
	ImGui::Text("%s", displayName);
	ImGui::SetWindowFontScale(1.0f);
	ImGui::PopStyleColor();

	ImGui::SameLine(0.0f, 24.0f);

	ImGui::PushStyleColor(ImGuiCol_Text, grayText);
	ImGui::Text("%s %s", ICON_FA_CALENDAR, displayDate);
	ImGui::PopStyleColor();

	// Right aligned button (leaders only)
	if (isLeader)
	{
		float btnWidth = 140.0f;
		ImGui::SameLine(totalWidth - btnWidth);

		ImGui::PushStyleColor(ImGuiCol_Button, cyanColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 193.0f / 255.0f, 201.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 153.0f / 255.0f, 161.0f / 255.0f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(18.0f / 255.0f, 20.0f / 255.0f, 20.0f / 255.0f, 1.0f));
		if (ImGui::Button(ICON_FA_PLUS " New Milestone", ImVec2(btnWidth, 32.0f)))
		{
			ImGui::OpenPopup("Create Milestone");
		}
		ImGui::PopStyleColor(4);
		ImGui::PopStyleVar();
	}

	RenderCreateMilestoneModal(projectId);
	RenderEditMilestoneModal(projectId);
	RenderDeleteMilestoneModal(projectId);

	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	// Separator Line
	ImVec2 cursor = ImGui::GetCursorScreenPos();
	drawList->AddLine(cursor, ImVec2(cursor.x + totalWidth, cursor.y), ImGui::GetColorU32(borderColor), 1.0f);
	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	// --- Active Milestones Title ---
	ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
	ImGui::Text(ICON_FA_FLAG " Active Milestones");
	ImGui::PopStyleColor();

	char totalBuf[32];
	std::snprintf(totalBuf, sizeof(totalBuf), "Total: %02zu", m_Milestones.size());
	const float totalLabelWidth = ImGui::CalcTextSize(totalBuf).x;
	ImGui::SameLine(totalWidth - totalLabelWidth);
	ImGui::PushStyleColor(ImGuiCol_Text, grayText);
	ImGui::TextUnformatted(totalBuf);
	ImGui::PopStyleColor();

	auto DrawRowSeparator = [&]() {
		ImGui::Dummy(ImVec2(0.0f, 8.0f));
		ImVec2 p = ImGui::GetCursorScreenPos();
		drawList->AddLine(p, ImVec2(p.x + totalWidth, p.y), ImGui::GetColorU32(ImVec4(40.0f / 255.0f, 43.0f / 255.0f, 43.0f / 255.0f, 1.0f)), 1.0f);
		ImGui::Dummy(ImVec2(0.0f, 8.0f));
		};

	DrawRowSeparator();

	if (m_Milestones.empty())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		if (isLeader)
			ImGui::TextUnformatted("No milestones yet. Create one with NEW MILESTONE.");
		else
			ImGui::TextUnformatted("No milestones yet. Ask a project leader to create one.");
		ImGui::PopStyleColor();
		return;
	}

	auto DrawOutlineBadge = [&](const char* label, ImVec4 outlineColor, ImVec4 textColor, float width) {
		// Dummy reserves layout space so the next widget starts on a new line
		// (draw-list only painting leaves SameLine active and pushes following text off-screen).
		const ImVec2 pos = ImGui::GetCursorScreenPos();
		const float badgeHeight = 20.0f;
		drawList->AddRect(ImVec2(pos.x, pos.y - 2.0f), ImVec2(pos.x + width, pos.y - 2.0f + badgeHeight), ImGui::GetColorU32(outlineColor), 4.0f, 0, 1.0f);

		const ImVec2 textSize = ImGui::CalcTextSize(label);
		const float fontSize = ImGui::GetFontSize() * 0.8f;
		drawList->AddText(ImGui::GetFont(), fontSize, ImVec2(pos.x + (width - textSize.x * 0.8f) * 0.5f, pos.y + 1.0f), ImGui::GetColorU32(textColor), label);

		ImGui::Dummy(ImVec2(width, badgeHeight));
	};

	for (const TrackingTool::MilestoneInfo& milestone : m_Milestones)
	{
		ImGui::PushID(milestone.Id);

		const char* badgeLabel = ToBadgeLabel(milestone.Status);
		const ImVec4 badgeColor = StatusBadgeColor(milestone.Status, cyanColor, grayText, greenColor);
		const float badgeWidth = StatusBadgeWidth(badgeLabel);

		// Title row: name left; edit/delete (leader) + status badge right.
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::TextUnformatted(milestone.Name.c_str());
		ImGui::PopStyleColor();

		const float actionBtnW = 22.0f;
		const float actionsGap = 6.0f;
		const float actionsWidth = isLeader ? (actionBtnW * 2.0f + actionsGap + 8.0f) : 0.0f;
		const float rightClusterWidth = badgeWidth + actionsWidth;

		ImGui::SameLine(totalWidth - rightClusterWidth);

		if (isLeader)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 0.18f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 0.3f));
			ImGui::PushStyleColor(ImGuiCol_Text, grayText);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 2.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);

			if (ImGui::SmallButton(ICON_FA_PEN_TO_SQUARE "##edit"))
				OpenEditForMilestone(milestone);
			if (ImGui::IsItemHovered())
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

			ImGui::SameLine(0.0f, actionsGap);

			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(238.0f / 255.0f, 56.0f / 255.0f, 56.0f / 255.0f, 0.2f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(238.0f / 255.0f, 56.0f / 255.0f, 56.0f / 255.0f, 0.35f));
			if (ImGui::SmallButton(ICON_FA_TRASH "##delete"))
			{
				m_PendingDeleteMilestoneId = milestone.Id;
				m_PendingDeleteMilestoneName = milestone.Name;
				m_OpenDeleteModal = true;
			}
			if (ImGui::IsItemHovered())
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			ImGui::PopStyleColor(2);

			ImGui::PopStyleVar(2);
			ImGui::PopStyleColor(4);

			ImGui::SameLine(0.0f, 8.0f);
		}

		DrawOutlineBadge(badgeLabel, badgeColor, badgeColor, badgeWidth);

		if (!milestone.StartDate.empty() || !milestone.EndDate.empty())
		{
			ImGui::PushStyleColor(ImGuiCol_Text, grayText);
			ImGui::Text("%s %s  -  %s", ICON_FA_CALENDAR,
				milestone.StartDate.empty() ? "—" : milestone.StartDate.c_str(),
				milestone.EndDate.empty() ? "—" : milestone.EndDate.c_str());
			ImGui::PopStyleColor();
		}

		ImGui::Dummy(ImVec2(0.0f, 5.0f));

		const float progress01 = milestone.ProgressPercentage <= 0.0f ? 0.0f
			: (milestone.ProgressPercentage >= 100.0f ? 1.0f : milestone.ProgressPercentage / 100.0f);

		ImVec2 pbPos = ImGui::GetCursorScreenPos();
		const float pbHeight = 4.0f;
		drawList->AddRectFilled(pbPos, ImVec2(pbPos.x + totalWidth, pbPos.y + pbHeight),
			ImGui::GetColorU32(ImVec4(40.0f / 255.0f, 43.0f / 255.0f, 43.0f / 255.0f, 1.0f)));
		if (progress01 > 0.0f)
		{
			drawList->AddRectFilled(pbPos, ImVec2(pbPos.x + totalWidth * progress01, pbPos.y + pbHeight),
				ImGui::GetColorU32(cyanColor));
		}

		ImGui::Dummy(ImVec2(0.0f, 8.0f));

		char subtasksBuf[48];
		std::snprintf(subtasksBuf, sizeof(subtasksBuf), "Subtasks: %d/%d",
			milestone.DoneTasks, milestone.TotalTasks);

		char percentBuf[16];
		std::snprintf(percentBuf, sizeof(percentBuf), "%.0f%%",
			static_cast<double>(milestone.ProgressPercentage));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::TextUnformatted(subtasksBuf);
		const float percentWidth = ImGui::CalcTextSize(percentBuf).x;
		ImGui::SameLine(totalWidth - percentWidth);
		ImGui::TextUnformatted(percentBuf);
		ImGui::PopStyleColor();

		DrawRowSeparator();

		ImGui::PopID();
	}
}
