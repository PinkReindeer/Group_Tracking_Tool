#include "MemberView.h"
#include "imgui.h"
#include "IconsFontAwesome6.h"

#include "Database/Database.h"
#include "Platform/Application.h"
#include "Service/AuthService.h"
#include "Service/ProjectService.h"

#include <cctype>
#include <string>
#include <vector>

namespace
{
	bool IsLeaderRole(const std::string& role)
	{
		static constexpr char kLeader[] = "leader";
		if (role.size() != sizeof(kLeader) - 1)
			return false;
		for (size_t i = 0; i < role.size(); ++i)
		{
			const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(role[i])));
			if (c != kLeader[i])
				return false;
		}
		return true;
	}
}

void MemberView::OnRender(int projectId, const char* projectName, const char* createdDate)
{
	static int lastProjectId = -1;
	static bool needsRefresh = true;
	static std::vector<TrackingTool::MemberInfo> members;

	// Kick confirmation modal state
	static bool openKickConfirm = false;
	static std::string pendingKickMember;
	static int pendingKickProjectId = 0;

	if (projectId != lastProjectId)
	{
		lastProjectId = projectId;
		needsRefresh = true;
		// Cancel any pending kick when switching projects.
		openKickConfirm = false;
		pendingKickMember.clear();
	}

	if (needsRefresh)
	{
		members.clear();
		TrackingTool::Database::GetProjectMembers(projectId, members);
		needsRefresh = false;
	}

	const bool isLoggedIn = TrackingTool::AuthService::IsLoggedIn();
	const std::string currentUser = isLoggedIn
		? TrackingTool::AuthService::GetLoggedInUser()
		: std::string{};

	// Prefer role from the active project session; fall back to membership list.
	bool canManageMembers = false;
	TrackingTool::ProjectInfo activeProject;
	if (TrackingTool::ProjectService::GetActiveProject(activeProject) && activeProject.Id == projectId)
	{
		canManageMembers = IsLeaderRole(activeProject.Role);
	}
	else if (isLoggedIn)
	{
		for (const auto& m : members)
		{
			if (m.Name == currentUser)
			{
				canManageMembers = IsLeaderRole(m.Role);
				break;
			}
		}
	}

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	float totalWidth = ImGui::GetContentRegionAvail().x;

	// --- Colors ---
	const ImVec4 cyanColor = ImVec4(0.0f, 173.0f / 255.0f, 181.0f / 255.0f, 1.0f);
	const ImVec4 grayText = ImVec4(187.0f / 255.0f, 201.0f / 255.0f, 202.0f / 255.0f, 1.0f);
	const ImVec4 whiteText = ImVec4(226.0f / 255.0f, 226.0f / 255.0f, 226.0f / 255.0f, 1.0f);
	const ImVec4 borderColor = ImVec4(60.0f / 255.0f, 73.0f / 255.0f, 74.0f / 255.0f, 1.0f);
	const ImVec4 redColor = ImVec4(238.0f / 255.0f, 56.0f / 255.0f, 56.0f / 255.0f, 1.0f);
	const ImVec4 boxBgColor = ImVec4(26.0f / 255.0f, 28.0f / 255.0f, 28.0f / 255.0f, 1.0f);

	// --- Header ---
	ImGui::TextColored(whiteText, "%s", (projectName && projectName[0] != '\0') ? projectName : "Project");
	ImGui::SameLine(0.0f, 24.0f);
	ImGui::TextColored(grayText, "%s %s", ICON_FA_CALENDAR, (createdDate && createdDate[0] != '\0') ? createdDate : "—");

	ImGui::Dummy(ImVec2(0.0f, 10.0f));
	ImVec2 cursor = ImGui::GetCursorScreenPos();
	drawList->AddLine(cursor, ImVec2(cursor.x + totalWidth, cursor.y), ImGui::GetColorU32(borderColor), 1.0f);
	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	// --- Table Header ---
	ImGui::PushStyleColor(ImGuiCol_Text, cyanColor);
	ImGui::SetCursorPosX(30.0f); ImGui::Text("APPLICANT NAME");
	ImGui::SameLine(totalWidth * 0.5f); ImGui::Text("TIMESTAMP");
	ImGui::SameLine(totalWidth - 80.0f); ImGui::Text("ACTIONS");
	ImGui::PopStyleColor();

	for (const auto& member : members)
	{
		ImGui::Dummy(ImVec2(0.0f, 10.0f));

		const bool isLeader = IsLeaderRole(member.Role);
		const bool isSelf = isLoggedIn && (member.Name == currentUser);

		ImGui::SetCursorPosX(30.0f);
		if (isSelf && isLeader)
			ImGui::TextColored(cyanColor, "%s (Leader · You)", member.Name.c_str());
		else if (isSelf)
			ImGui::TextColored(whiteText, "%s (You)", member.Name.c_str());
		else if (isLeader)
			ImGui::TextColored(cyanColor, "%s (Leader)", member.Name.c_str());
		else
			ImGui::TextColored(whiteText, "%s", member.Name.c_str());

		ImGui::SameLine(totalWidth * 0.5f);
		ImGui::TextColored(grayText, "%s", member.JoinDate.c_str());

		// Kick only for project leaders, and only against other non-leader members.
		const bool canKick = canManageMembers && !isSelf && !isLeader;

		ImGui::SameLine(totalWidth - 70.0f);
		if (canKick)
		{
			std::string btnId = "Kick##" + member.Name;

			ImGui::PushStyleColor(ImGuiCol_Button, redColor);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
			if (ImGui::Button(btnId.c_str(), ImVec2(60.0f, 24.0f)))
			{
				// Open confirmation — do not remove yet.
				pendingKickMember = member.Name;
				pendingKickProjectId = projectId;
				openKickConfirm = true;
			}
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
		}
		else
		{
			ImGui::TextColored(grayText, "—");
		}
	}

	// --- Kick confirmation modal ---
	if (openKickConfirm)
	{
		ImGui::OpenPopup("Kick Member");
		openKickConfirm = false;
	}

	const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	ImGui::PushStyleColor(ImGuiCol_PopupBg, boxBgColor);
	ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 24.0f));

	if (ImGui::BeginPopupModal("Kick Member", nullptr,
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
	{
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		ImGui::SetWindowFontScale(1.2f);
		ImGui::Text(ICON_FA_USER_MINUS " Remove Member");
		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 8.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, grayText);
		ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 350.0f);
		ImGui::TextWrapped(
			"Are you sure you want to remove \"%s\" from this project? They will lose access until they rejoin with the invite code.",
			pendingKickMember.c_str());
		ImGui::PopTextWrapPos();
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 20.0f));

		const float modalBtnWidth = (350.0f - 10.0f) / 2.0f;

		// Cancel (default focus for destructive action)
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
		if (ImGui::Button("Cancel", ImVec2(modalBtnWidth, 36.0f)))
		{
			pendingKickMember.clear();
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
		if (ImGui::Button("Remove", ImVec2(modalBtnWidth, 36.0f)))
		{
			std::string message;
			if (TrackingTool::ProjectService::RemoveMember(pendingKickProjectId, pendingKickMember, message))
			{
				TrackingTool::Application::Get().PushNotification(message, NotificationType::Info);
				needsRefresh = true;
				pendingKickMember.clear();
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
