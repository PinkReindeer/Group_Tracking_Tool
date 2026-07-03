#include "Platform/Application.h"

#include "VoidLayer.h"

int main()
{
	TrackingTool::ApplicationSpecification appSpec;
	appSpec.Name = "Tracking Tool";
	appSpec.WindowSpec.Height = 720;
	appSpec.WindowSpec.Width = 1280;

	TrackingTool::Application app(appSpec);
	app.PushLayer<VoidLayer>();
	app.Run();
}