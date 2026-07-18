#
# App
#

set(SOURCES
	Source/Main.cpp
	Source/Authentication/AuthenticationLayer.h
	Source/Authentication/AuthenticationLayer.cpp
	Source/Authentication/Views/LoginView.h
	Source/Authentication/Views/LoginView.cpp
	Source/Authentication/Views/RegisterView.h
	Source/Authentication/Views/RegisterView.cpp
	Source/NotificationLayer.h
	Source/NotificationLayer.cpp
	Source/Dashboard/DashboardLayer.h
	Source/Dashboard/DashboardLayer.cpp
	Source/AppLayoutLayer.h
	Source/AppLayoutLayer.cpp
	Source/Project/ProjectLayer.h
	Source/Project/ProjectLayer.cpp
	Source/Project/Views/TasksView.h
	Source/Project/Views/TasksView.cpp
	Source/Project/Views/MilestonesView.h
	Source/Project/Views/MilestonesView.cpp
	Source/Project/Views/ChartView.h
	Source/Project/Views/ChartView.cpp
	Source/Project/Views/WorkloadView.h
	Source/Project/Views/WorkloadView.cpp
	Source/Project/Views/MemberView.h
	Source/Project/Views/MemberView.cpp
)

add_executable(App)

target_sources(App PRIVATE ${SOURCES})

# Match VS filters to directory structure on disk
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES})

target_link_libraries(App PRIVATE Core)

target_include_directories(App PRIVATE Source)

add_custom_command(TARGET App POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Assets"
        "$<TARGET_FILE_DIR:App>/Assets"
)

# Set working directory for VS debugger
set_target_properties(App PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/App"
	FOLDER "Projects"
)