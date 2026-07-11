#include <glad/gl.h>

#include "imgui.h"

#include "AppLayoutLayer.h"

void AppLayoutLayer::OnRender()
{
	glClearColor(18.0f / 255.0f, 20.0f / 255.0f, 26.0f / 255.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	RenderSideNavBar();
	RenderTopNavBar();
	OnRenderContent();
}

void AppLayoutLayer::RenderSideNavBar()
{

}

void AppLayoutLayer::RenderTopNavBar()
{
	// Allow derived layers to insert additional items here, like tabs.
	OnRenderTopNavBarExtensions();
}