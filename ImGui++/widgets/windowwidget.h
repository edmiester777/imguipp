#pragma once

#include "../widget.h"
#include <string>

namespace imguipp {
	typedef int WindowFlags;

	class WindowWidget : public Widget
	{
	public:
		WindowWidget();
		WindowWidget(std::string title);
		virtual ~WindowWidget();

		void SetTitle(std::string title);
		void IncludeTitleBar(bool include);
		void IncludeScrollbar(bool include);
		void IncludeMenuBar(bool include);
		void CanMove(bool canMove);
		void CanResize(bool canResize);
		void CanCollapse(bool canCollapse);
		void IncludeNav(bool include);
		void IncludeBackground(bool include);
		void ShouldBringToFrontOnFocus(bool bringToFront);
		void EnableDocking(bool enabled);

		virtual void Render() override;

	private:
		WindowFlags GetWindowFlags();

		std::string m_title;
		bool m_includeTitleBar;
		bool m_includeScrollBar;
		bool m_includeMenuBar;
		bool m_canMove;
		bool m_canResize;
		bool m_canCollapse;
		bool m_includeNav;
		bool m_includeBackground;
		bool m_shouldBringToFrontOnFocus;
		bool m_enableDocking;
	};
}
