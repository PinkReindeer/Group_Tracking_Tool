#pragma once

#include <glad/gl.h>

#include "AppLayoutLayer.h"

class DashboardLayer : public AppLayoutLayer
{
public:
	DashboardLayer() = default;
	virtual ~DashboardLayer() = default;

	virtual void OnUpdate(float ts) override;

protected:
	virtual void OnRenderContent() override;
};
