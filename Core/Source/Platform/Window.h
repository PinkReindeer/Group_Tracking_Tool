#pragma once

#include <cstdint>
#include <string>

#include <glm/glm.hpp>

namespace TrackingTool
{
	struct WindowSpecification
	{
		std::string Title;
		std::string IconPath = "Assets/icon.png";
		uint32_t Width = 1440;
		uint32_t Height = 1024;
		bool IsResizeable = true;
		bool VSync = true;
	};

	class Window
	{
	public:
		Window(const WindowSpecification& specification = WindowSpecification());
		~Window();

		void Create();
		void Destroy();

		void Update();
		void ToggleFullscreen();
		void SetFullscreen(bool fullscreen);
		bool IsFullscreen() const;
		bool IsFocused() const;

		glm::vec2 GetFramebufferSize() const;

		bool ShouldClose() const;

		GLFWwindow* GetHandle() const { return m_Handle; }
	private:
		WindowSpecification m_Specification;

		GLFWwindow* m_Handle = nullptr;

		bool m_IsFullscreen = false;
		int m_WindowedX = 0, m_WindowedY = 0;
		int m_WindowedWidth = 0, m_WindowedHeight = 0;

	};
}