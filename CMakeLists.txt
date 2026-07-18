cmake_minimum_required(VERSION 3.14)

project(GroupTrackingTool)

set(CMAKE_CXX_STANDARD 20)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies
include(Dependencies.cmake)

# Projects
add_subdirectory(Core)
add_subdirectory(App)