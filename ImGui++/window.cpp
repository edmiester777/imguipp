#include "window.h"
#include "backend_includes.h"

namespace imguipp {
	Window::Window() : Widget()
	{
		m_running = false;
		m_windowTitle = "Untitled Window";
		m_size = { 1280, 800 };
		m_location = { 100, 100 };
	}

	Window::~Window()
	{
		Widget::~Widget();
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
	void Window::QueueAction(std::function<void()> action)
	{
		std::lock_guard<std::mutex> lock(m_workMutex);
		m_workQueue.push(action);
	}
	void Window::QueueSafeAction(std::function<void()> action)
	{
		QueueAction([this, action] {
			std::lock_guard<std::mutex> lock(m_renderMutex);
			action();
		});
	}
	void Window::AddChild(std::shared_ptr<Widget> child)
	{
		QueueSafeAction([this, child] {
			Widget::AddChild(child);
		});
	}
	void Window::QueueRender()
	{
		QueueSafeAction([this] {
			Render();
		});
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
		QueueSafeAction([this] {
			OnStart();
			Render();
		});
		m_renderThread = std::thread([this] {
			// continue to process work queue
			INFINITE_LOOP
			{
				m_renderMutex.lock();
				bool running = m_running;
				m_renderMutex.unlock();

				if (!running)
					break;

				m_workMutex.lock();
				if (m_workQueue.empty())
				{
					m_workMutex.unlock();
					break;
				}
				std::function<void()> work = m_workQueue.front();
				m_workQueue.pop();
				m_workMutex.unlock();
				work();
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
