#pragma once

#include <glad/gl.h>

#include "Platform/Layer.h"

class NotificationLayer : public TrackingTool::Layer
{
public:
	NotificationLayer() = default;
	virtual ~NotificationLayer() = default;

	virtual void OnUpdate(float ts) override;
	virtual void OnRender() override;
};
