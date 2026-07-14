#pragma once

class MemberView
{
public:
	MemberView() = default;
	~MemberView() = default;

	void OnRender(const char* projectName, const char* createdDate);
};
