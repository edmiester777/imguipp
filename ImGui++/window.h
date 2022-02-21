#pragma once

#include "common.h"
#include "widget.h"
#include <vector>
#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <queue>

namespace imguipp {
	class Window : public Widget {
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
		virtual void Render() = 0;

		void SetTitle(std::string title, bool triggerEvent = true);
		std::string GetTitle() const;
		void SetSize(Size size, bool triggerEvent = true);
		Size GetSize() const;
		void SetLocation(Point location, bool triggerEvent = true);
		Point GetLocation() const;
		void OnClose(std::function<void()> callback);

		/**
		 * Queue a work action to be executed on render thread. Use this for manipulation
		 * that does not require thread safety.
		 * 
		 * For thread safety, please use QueueSafeAction(std::function<void()> action).
		 * 
		 * @param action Action to perform on render thread.
		 */
		void QueueAction(std::function<void()> action);

		/**
		 * Thread-safe implementation of QueueAction(std::function<void()>. This is
		 * used for property manipulation.
		 * 
		 * @param action Action to perform on render thread.
		 */
		void QueueSafeAction(std::function<void()> action);

		virtual void AddChild(std::shared_ptr<Widget> child) override;

	
	protected:
		void QueueRender();
		void TriggerOnCloseEvent();
		virtual void OnWindowTitleChanged() = 0;
		virtual void OnResized() = 0;
		virtual void OnMove() = 0;
		virtual void OnStart();

	private:
		void StartRenderThread();
		void ShutdownRenderThread();

		std::thread m_renderThread;
		std::mutex m_renderMutex;
		std::mutex m_workMutex;
		std::queue<std::function<void()>> m_workQueue;
		bool m_running;
		std::string m_windowTitle;
		std::function<void()> m_onCloseCallback;
		Size m_size;
		Point m_location;
	};
}
