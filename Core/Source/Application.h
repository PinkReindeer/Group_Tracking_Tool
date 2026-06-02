#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Window.h"
#include "Layer.h"

namespace TrackingTool
{
    struct ApplicationSpecification
    {
        std::string Name = "Application";
        WindowSpecification WindowSpec;
    };

    class Application
    {
    public:
        Application(const ApplicationSpecification& spec = ApplicationSpecification());
        ~Application();

        void Run();
        void Stop();

        template<typename TLayer>
        requires(std::is_base_of_v<Layer, TLayer>)
        void PushLayer()
        {
            m_LayerStack.push_back(std::make_unique<TLayer>());
        }

        template<typename TLayer>
        requires(std::is_base_of_v<Layer, TLayer>)
        TLayer* GetLayer()
        {
            for (const auto& layer : m_LayerStack)
            {
                if (auto casted = dynamic_cast<TLayer*>(layer.get()))
                    return casted;
            }
            return nullptr;
        }

        glm::vec2 GetFramebufferSize() const;

        std::shared_ptr<Window> GetWindow() const { return m_Window; }

        static Application& Get();
        static float GetTime();

		ImFont* GetUIFont() const { return m_UIFont; }
		ImFont* GetFaFont() const { return m_FaFont; }

    private:
        ApplicationSpecification m_Specification;
        std::shared_ptr<Window> m_Window;
        bool m_Running = true;

		ImFont* m_UIFont = nullptr;
		ImFont* m_FaFont = nullptr;

        std::vector<std::unique_ptr<Layer>> m_LayerStack;

        friend class Layer;
    };
}