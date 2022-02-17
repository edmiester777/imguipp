#include "window.h"
#include "backend_includes.h"

namespace imguipp {
	Window::Window()
	{
		m_running = false;
		m_windowTitle = "Untitled Window";
		m_size = { 1280, 800 };
		m_location = { 100, 100 };
	}

	Window::~Window()
	{
		Close();
	}

	void Window::Close()
	{
		ShutdownRenderThread();
	}

	void Window::Show()
	{
		StartRenderThread();
	}

	void Window::Hide()
	{
	}

	void Window::Join()
	{
		if (m_renderThread.joinable())
		{
			m_renderThread.join();
		}
	}

	void Window::EnableKeyboardNavigation()
	{
		ImGuiIO& io = ImGui::GetIO();
		if (!(io.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard))
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	}

	void Window::EnableGamepadNavigation()
	{
		ImGuiIO& io = ImGui::GetIO();
		if (!(io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad))
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	}

	void Window::EnableDocking()
	{
		ImGuiIO& io = ImGui::GetIO();
		if (!(io.ConfigFlags & ImGuiConfigFlags_DockingEnable))
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	}

	void Window::SetTitle(std::string title, bool triggerEvent)
	{
		m_windowTitle = title;
		if (triggerEvent)
			OnWindowTitleChanged();
	}

	std::string Window::GetTitle() const
	{
		return m_windowTitle;
	}
	void Window::SetSize(Size size, bool triggerEvent)
	{
		m_size = size;
		if (triggerEvent)
			OnResized();
	}
	Size Window::GetSize() const
	{
		return m_size;
	}
	void Window::SetLocation(Point location, bool triggerEvent)
	{
		m_location = location;
		if (triggerEvent)
			OnMove();
	}
	Point Window::GetLocation() const
	{
		return m_location;
	}
	void Window::OnClose(std::function<void()> callback)
	{
		m_onCloseCallback = callback;
	}
	void Window::RenderChildren()
	{
		// TODO: Implement children.
	}
	void Window::TriggerOnCloseEvent()
	{
		if (m_onCloseCallback)
			m_onCloseCallback();
	}
	void Window::OnStart()
	{
	}
	void Window::StartRenderThread()
	{
		// rendering already started.
		if (m_renderThread.joinable())
			return;

		// main render thread
		m_running = true;
		m_renderThread = std::thread([this] {
			OnStart();
			while (true)
			{
				m_renderMutex.lock();
				bool running = m_running;
				m_renderMutex.unlock();

				if (!running)
					break;

				Render();
			}
		});
	}
	void Window::ShutdownRenderThread()
	{
		if (m_renderThread.joinable())
		{
			m_renderMutex.lock();
			m_running = false;
			m_renderMutex.unlock();
			m_renderThread.join();
		}
	}
}
