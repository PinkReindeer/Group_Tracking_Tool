#include "Platform/Application.h"
#include "Database/Database.h"

#include "Authentication/AuthenticationLayer.h"
#include "NotificationLayer.h"

#include <iostream>

void* operator new(size_t size)
{
	std::cout << "Allocating " << size << " bytes\n";
	return malloc(size);
}

int main()
{
	// Init database
	TrackingTool::Database::Init();
	TrackingTool::ApplicationSpecification appSpec;
	appSpec.Name = "Tracking Tool";
	appSpec.WindowSpec.Height = 720;
	appSpec.WindowSpec.Width = 1280;

	TrackingTool::Application app(appSpec);
	app.PushLayer<AuthenticationLayer>();
	app.PushLayer<NotificationLayer>();
	app.Run();
}