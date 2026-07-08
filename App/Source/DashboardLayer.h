#pragma once

#include <glad/gl.h>
#include "Platform/Layer.h"

class DashboardLayer : public TrackingTool::Layer
{
public:
	DashboardLayer();
	virtual ~DashboardLayer();

	virtual void OnUpdate(float ts) override;
	virtual void OnRender() override;
};
