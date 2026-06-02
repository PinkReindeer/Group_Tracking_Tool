#
# Core
#

set(SOURCES
    Source/Application.cpp
    Source/Application.h
    Source/Layer.cpp
    Source/Layer.h
    Source/Window.cpp
    Source/Window.h
    Vendor/stb/stb_image.h
    Vendor/stb/stb_image.cpp
)

add_library(Core STATIC)

target_sources(Core PRIVATE ${SOURCES})

# Match VS filters to directory structure on disk
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES})

target_compile_definitions(Core PUBLIC GLFW_INCLUDE_NONE)

target_link_libraries(Core PUBLIC
    glfw
    glad
    glm
    imgui
    pqxx
    bcrypt
    OpenGL::GL
)

target_include_directories(Core PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${icon_headers_SOURCE_DIR}
    Source
    Vendor/stb
)

# Precompiled header
target_precompile_headers(Core PUBLIC "${CMAKE_SOURCE_DIR}/gpch.h")

set_target_properties(Core PROPERTIES FOLDER "Projects")