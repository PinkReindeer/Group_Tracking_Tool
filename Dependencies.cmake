#
# Dependencies
#
include(FetchContent)

# GLFW
find_package(glfw3 3.4 QUIET)
if (NOT glfw3_FOUND)
    FetchContent_Declare(
            glfw3
            DOWNLOAD_EXTRACT_TIMESTAMP OFF
            URL https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip
    )
    FetchContent_GetProperties(glfw3)
    if (NOT glfw3_POPULATED)
        set(FETCHCONTENT_QUIET NO)
        FetchContent_Populate(glfw3)
        add_subdirectory(${glfw3_SOURCE_DIR} ${glfw3_BINARY_DIR})
    endif()
endif()

# GLM
find_package(glm 1.0.1 QUIET)
if (NOT glm_FOUND)
    FetchContent_Declare(
            glm
            DOWNLOAD_EXTRACT_TIMESTAMP OFF
            URL https://github.com/g-truc/glm/archive/refs/tags/1.0.1.zip
    )
    FetchContent_GetProperties(glm)
    if (NOT glm_POPULATED)
        set(FETCHCONTENT_QUIET NO)
        FetchContent_Populate(glm)
        add_subdirectory(${glm_SOURCE_DIR} ${glm_BINARY_DIR})
    endif()
endif()
set_target_properties(glm PROPERTIES FOLDER "Dependencies")

# PostgreSQL
find_package(PostgreSQL REQUIRED)

# libpqxx
FetchContent_Declare(
    libpqxx
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/jtv/libpqxx/archive/refs/tags/7.9.2.zip
)

FetchContent_GetProperties(libpqxx)
if (NOT libpqxx_POPULATED)
    set(FETCHCONTENT_QUIET NO)
    # libpqxx tests can cause build issues if we don't disable them
    set(SKIP_BUILD_TEST ON CACHE BOOL "" FORCE)
    set(BUILD_DOC OFF CACHE BOOL "" FORCE)
    
    FetchContent_Populate(libpqxx)
    add_subdirectory(${libpqxx_SOURCE_DIR} ${libpqxx_BINARY_DIR})
endif()

# libbcrypt
FetchContent_Declare(
    libbcrypt
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    GIT_REPOSITORY https://github.com/trusch/libbcrypt
    GIT_TAG        d6523c370de6e724ce4ec703e2449b5b028ea3b1
)

FetchContent_GetProperties(libbcrypt)
if (NOT libbcrypt_POPULATED)
    FetchContent_Populate(libbcrypt)
    
    file(GLOB BCRYPT_SOURCES 
        "${libbcrypt_SOURCE_DIR}/src/*.c" 
        "${libbcrypt_SOURCE_DIR}/src/*.cpp"
        "${libbcrypt_SOURCE_DIR}/src/*.S"
    )

    add_library(bcrypt STATIC ${BCRYPT_SOURCES})
    
    # Expose the include directory PUBLICLY so the main executable automatically inherits it
    target_include_directories(bcrypt PUBLIC "${libbcrypt_SOURCE_DIR}/include")

    if (NOT WIN32)
        target_include_directories(bcrypt PRIVATE "${libbcrypt_SOURCE_DIR}/include/bcrypt")
    endif()
endif()

# OpenGL
find_package(OpenGL REQUIRED)

# GLAD
add_library(glad STATIC "${CMAKE_CURRENT_SOURCE_DIR}/Core/Vendor/glad/src/gl.c")
target_include_directories(glad PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Core/Vendor/glad/include")
set_target_properties(glad PROPERTIES FOLDER "Dependencies")

# Dear ImGui
FetchContent_Declare(
    imgui
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/ocornut/imgui/archive/refs/tags/v1.92.3.zip
)

FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)

    add_library(imgui STATIC
        "${imgui_SOURCE_DIR}/imgui.cpp"
        "${imgui_SOURCE_DIR}/imgui_draw.cpp"
        "${imgui_SOURCE_DIR}/imgui_tables.cpp"
        "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
        "${imgui_SOURCE_DIR}/imgui_demo.cpp"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp"
     )

    target_include_directories(imgui PUBLIC 
        "${imgui_SOURCE_DIR}" 
        "${imgui_SOURCE_DIR}/backends"
    )

    target_link_libraries(imgui PUBLIC glfw OpenGL::GL)
endif()

# IconFontCppHeaders
FetchContent_Declare(
    icon_headers
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/juliettef/IconFontCppHeaders/archive/refs/heads/main.zip
)
FetchContent_GetProperties(icon_headers)
if(NOT icon_headers_POPULATED)
    FetchContent_Populate(icon_headers)
endif()