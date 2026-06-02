#include "VoidLayer.h"

VoidLayer::VoidLayer()
{
}

VoidLayer::~VoidLayer()
{
}

void VoidLayer::OnUpdate(float ts)
{
}

void VoidLayer::OnRender()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}
