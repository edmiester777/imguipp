#pragma once
#include "Common.h"
#include <vector>
#include <string>
#include <functional>
#include <thread>
#include <mutex>

namespace imguipp {
	class Window {
	public:
		Window();
		virtual ~Window();

		virtual void Close();
		virtual void Show();
		virtual void Hide();
		void Join();
		void EnableKeyboardNavigation();
		void EnableGamepadNavigation();
		void EnableDocking();

		void SetTitle(std::string title, bool triggerEvent = true);
		std::string GetTitle() const;
		void SetSize(Size size, bool triggerEvent = true);
		Size GetSize() const;
		void SetLocation(Point location, bool triggerEvent = true);
		Point GetLocation() const;
		void OnClose(std::function<void()> callback);

	
	protected:
		void RenderChildren();
		void TriggerOnCloseEvent();
		virtual void OnWindowTitleChanged() = 0;
		virtual void OnResized() = 0;
		virtual void OnMove() = 0;
		virtual void OnStart();
		virtual void Render() = 0;

	private:
		void StartRenderThread();
		void ShutdownRenderThread();

		std::thread m_renderThread;
		std::mutex m_renderMutex;
		bool m_running;
		std::string m_windowTitle;
		std::function<void()> m_onCloseCallback;
		Size m_size;
		Point m_location;
	};
}
