#pragma once

class MilestonesView
{
public:
	MilestonesView() = default;
	~MilestonesView() = default;

void OnRender(int projectId, const char* projectName, const char* createdDate);
};
