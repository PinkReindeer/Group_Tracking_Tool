#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "stb_image.h"

#include "Window.h"

namespace TrackingTool
{
	Window::Window(const WindowSpecification& specification)
		: m_Specification(specification)
	{
	}

	Window::~Window()
	{
		Destroy();
	}

	void Window::Create()
	{
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

		m_Handle = glfwCreateWindow(m_Specification.Width, m_Specification.Height,
			m_Specification.Title.c_str(), nullptr, nullptr);

		if (!m_Handle)
		{
			std::cerr << "Failed to create GLFW window!\n";
			assert(false);
		}

		// Load image and set in on the window
		GLFWimage image = {};
		int channels;

		image.pixels = stbi_load(m_Specification.IconPath.c_str() , &image.width, &image.height, &channels, 4);

		if (image.pixels)
		{
			glfwSetWindowIcon(m_Handle, 1, &image);
			stbi_image_free(image.pixels);
		}
		else
		{
			std::cerr << "Failed to load window icon from '" << m_Specification.IconPath.c_str() << "': " << stbi_failure_reason() << std::endl;
		}

		glfwMakeContextCurrent(m_Handle);
		gladLoadGL(glfwGetProcAddress);

		glfwSwapInterval(m_Specification.VSync ? 1 : 0);
	}

	void Window::Destroy()
	{
		if (m_Handle)
			glfwDestroyWindow(m_Handle);

		m_Handle = nullptr;
	}

	void Window::Update()
	{
		glfwSwapBuffers(m_Handle);
	}

	void Window::ToggleFullscreen()
	{
		SetFullscreen(!m_IsFullscreen);
	}

	void Window::SetFullscreen(bool fullscreen)
	{
		if (m_IsFullscreen == fullscreen)
			return;

		if (fullscreen)
		{
			glfwGetWindowPos(m_Handle, &m_WindowedX, &m_WindowedY);
			glfwGetWindowSize(m_Handle, &m_WindowedWidth, &m_WindowedHeight);

			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);

			glfwSetWindowMonitor(m_Handle, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
			m_IsFullscreen = true;
		}
		else
		{
			glfwSetWindowMonitor(m_Handle, nullptr, m_WindowedX, m_WindowedY, m_WindowedWidth, m_WindowedHeight, 0);
			m_IsFullscreen = false;
		}
	}

	bool Window::IsFullscreen() const
	{
		return m_IsFullscreen;
	}

	bool Window::IsFocused() const
	{
		return glfwGetWindowAttrib(m_Handle, GLFW_FOCUSED) != 0;
	}

	glm::vec2 Window::GetFramebufferSize() const
	{
		int width, height;
		glfwGetFramebufferSize(m_Handle, &width, &height);
		return { width, height };
	}

	bool Window::ShouldClose() const
	{
		return glfwWindowShouldClose(m_Handle) != 0;
	}
}
