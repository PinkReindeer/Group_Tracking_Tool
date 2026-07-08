#include "Platform/Application.h"
#include "Database/Database.h"

#include "LoginLayer.h"
#include "RegisterLayer.h"
#include "NotificationLayer.h"

int main()
{
	// Init database
	TrackingTool::Database::Init();

	TrackingTool::ApplicationSpecification appSpec;
	appSpec.Name = "Tracking Tool";
	appSpec.WindowSpec.Height = 720;
	appSpec.WindowSpec.Width = 1280;

	TrackingTool::Application app(appSpec);
	app.PushLayer<LoginLayer>();
	//app.PushLayer<RegisterLayer>();
	app.PushLayer<NotificationLayer>();
	app.Run();
}