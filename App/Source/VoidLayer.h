#pragma once

#include <glad/gl.h>

#include "Platform/Layer.h"

class VoidLayer : public TrackingTool::Layer
{
public:
	VoidLayer();
	virtual ~VoidLayer();

	virtual void OnUpdate(float ts) override;
	virtual void OnRender() override;
};
