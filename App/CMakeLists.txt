#
# App
#

set(SOURCES
	Source/Main.cpp
	Source/VoidLayer.cpp
	Source/VoidLayer.h
	Source/RegisterLayer.h
	Source/RegisterLayer.cpp
	Source/NotificationLayer.h
	Source/NotificationLayer.cpp
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