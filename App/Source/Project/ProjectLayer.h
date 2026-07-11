#pragma once

#include <glad/gl.h>

#include "AppLayoutLayer.h"
#include "Views/TasksView.h"
#include "Views/MilestonesView.h"
#include "Views/ChartView.h"
#include "Views/WorkloadView.h"
#include "Views/MemberView.h"

enum class ProjectTab { Tasks, Milestones, Chart, Workload, Member};

class ProjectLayer : public AppLayoutLayer
{
public:
	ProjectLayer() = default;
	virtual ~ProjectLayer() = default;

	virtual void OnUpdate(float ts) override;

protected:
	virtual void OnRenderContent() override;
	virtual void OnRenderTopNavBarExtensions() override;

private:
	ProjectTab m_ActiveTab = ProjectTab::Tasks;

	TasksView m_TasksView;
	MilestonesView m_MilestonesView;
	ChartView m_ChartView;
	WorkloadView m_WorkloadView;
	MemberView m_MemberView;
};
