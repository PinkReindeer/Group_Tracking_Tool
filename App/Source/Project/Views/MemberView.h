#pragma once

class MemberView
{
public:
	MemberView() = default;
	~MemberView() = default;

	void OnRender(int projectId, const char* projectName, const char* createdDate);
};
