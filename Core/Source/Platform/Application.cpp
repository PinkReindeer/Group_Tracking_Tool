#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "IconsFontAwesome6.h"

#include "Application.h"

#include "Layer.h"

#include "Utils/Assert.h"
#include "Utils/Log.h"

static constexpr const char* UIPath = "Assets/Fonts/Inder-Regular.ttf";
static constexpr const char* FaPath = "Assets/Fonts/fa-solid-900.ttf";

namespace TrackingTool
{

    static Application* s_Application = nullptr;

    static void GLFWErrorCallback(int error, const char* description)
    {
        Log::Error("[GLFW] ({}) {}", error, description);
    }

    Application::Application(const ApplicationSpecification& spec)
        : m_Specification(spec)
    {
        TT_ASSERT(!s_Application, "Only one Application instance is allowed!");

        s_Application = this;


        glfwSetErrorCallback(GLFWErrorCallback);
        if (!glfwInit())
        {
            Log::Fatal("Failed to initialize GLFW.");
            throw std::runtime_error("Failed to initialize GLFW");
        }

        // Set window title to app name if empty
        if (m_Specification.WindowSpec.Title.empty())
            m_Specification.WindowSpec.Title = m_Specification.Name;

        m_Window = std::make_shared<Window>(m_Specification.WindowSpec);
        m_Window->Create();

        // Dear ImGui setup
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        m_UIFont = io.Fonts->AddFontFromFileTTF(UIPath, 18.0f);
        if (!m_UIFont)
        {
            Log::Warn("Failed to load UI font from '{}'. Falling back to default font.", UIPath);
            m_UIFont = io.Fonts->AddFontDefault();
        }

        static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
        ImFontConfig iconsConfig;
        iconsConfig.MergeMode = true;
        iconsConfig.PixelSnapH = true;
        iconsConfig.GlyphMinAdvanceX = 18.0f;
        iconsConfig.GlyphOffset = ImVec2(0, 1);

        m_FaFont = io.Fonts->AddFontFromFileTTF(FaPath, 18.0f, &iconsConfig, icons_ranges);
        if (!m_FaFont)
        {
            Log::Warn("Failed to load FontAwesome from '{}'. Icons will not appear.", FaPath);
        }

        io.Fonts->AddFontFromFileTTF(UIPath, 24.0f);
        iconsConfig.GlyphMinAdvanceX = 24.0f;
        io.Fonts->AddFontFromFileTTF(FaPath, 24.0f, &iconsConfig, icons_ranges);

        io.Fonts->AddFontFromFileTTF(UIPath, 44.0f);
        iconsConfig.GlyphMinAdvanceX = 44.0f;
        io.Fonts->AddFontFromFileTTF(FaPath, 44.0f, &iconsConfig, icons_ranges);

        ImGui_ImplGlfw_InitForOpenGL(m_Window->GetHandle(), true);
        ImGui_ImplOpenGL3_Init("#version 410");
    }

    Application::~Application()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        m_Window->Destroy();

        glfwTerminate();

        s_Application = nullptr;
    }

    void Application::Stop()
    {
        m_Running = false;
    }

    glm::vec2 Application::GetFramebufferSize() const
    {
        return m_Window->GetFramebufferSize();
    }

    Application& Application::Get()
    {
        TT_ASSERT(s_Application, "Application::Get() called before an Application was constructed.");
        return *s_Application;
    }

    void Application::Run()
    {
        m_Running = true;

        float lastTime = GetTime();

        // Main Application loop
        while (m_Running)
        {
            glfwPollEvents();

            if (m_Window->ShouldClose())
            {
                Stop();
                break;
            }

            float currentTime = GetTime();
            float timestep = glm::clamp(currentTime - lastTime, 0.001f, 0.1f);
            lastTime = currentTime;

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Main layer update here
            for (const std::unique_ptr<Layer>& layer : m_LayerStack)
                layer->OnUpdate(timestep);

            // NOTE: rendering can be done elsewhere (eg. render thread)
            for (const std::unique_ptr<Layer>& layer : m_LayerStack)
                layer->OnRender();

            Layer::ClearDeferredCleanup();

            glm::vec2 frameBufferSize = TrackingTool::Application::Get().GetFramebufferSize();
            glViewport(0, 0, (GLsizei)frameBufferSize.x, (GLsizei)frameBufferSize.y);

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            m_Window->Update();

        }
    }

    float Application::GetTime()
    {
        return (float)glfwGetTime();
    }

    void Application::PushNotification(const std::string& message, NotificationType type)
    {
        auto notification = GetLayer<NotificationLayer>();
        if (notification)
        {
            notification->ShowNotification(message, type);
        }
    }

}