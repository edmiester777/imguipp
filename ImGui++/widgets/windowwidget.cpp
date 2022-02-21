#include "windowwidget.h"
#include "../backend_includes.h"

namespace imguipp {
	WindowWidget::WindowWidget() : Widget()
	{
		SetTitle("<Unnamed>");
		IncludeTitleBar(true);
		IncludeScrollbar(false);
		IncludeMenuBar(false);
		CanMove(true);
		CanResize(true);
		CanCollapse(true);
		IncludeNav(true);
		IncludeBackground(true);
		ShouldBringToFrontOnFocus(true);
		EnableDocking(true);
	}

	WindowWidget::WindowWidget(std::string title) : WindowWidget()
	{
		SetTitle(title);
	}

	WindowWidget::~WindowWidget()
	{
	}

	void WindowWidget::SetTitle(std::string title)
	{
		m_title = title;
	}

	void WindowWidget::IncludeTitleBar(bool include)
	{
		m_includeTitleBar = include;
	}

	void WindowWidget::IncludeScrollbar(bool include)
	{
		m_includeScrollBar = include;
	}

	void WindowWidget::IncludeMenuBar(bool include)
	{
		m_includeMenuBar = include;
	}

	void WindowWidget::CanMove(bool canMove)
	{
		m_canMove = canMove;
	}

	void WindowWidget::CanResize(bool canResize)
	{
		m_canResize = canResize;
	}

	void WindowWidget::CanCollapse(bool canCollapse)
	{
		m_canCollapse = canCollapse;
	}

	void WindowWidget::IncludeNav(bool include)
	{
		m_includeNav = include;
	}

	void WindowWidget::IncludeBackground(bool include)
	{
		m_includeBackground = include;
	}

	void WindowWidget::ShouldBringToFrontOnFocus(bool bringToFront)
	{
		m_shouldBringToFrontOnFocus = bringToFront;
	}

	void WindowWidget::EnableDocking(bool enabled)
	{
		m_enableDocking = enabled;
	}

	void WindowWidget::Render()
	{
		bool open = Showing();
		if (!open)
			return;

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImVec2 pos(GetBounds().x, GetBounds().y);
		ImVec2 size(GetBounds().w, GetBounds().h);
		ImGui::SetNextWindowPos(pos, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);

		ImGui::Begin(m_title.c_str(), &open, GetWindowFlags());

		RenderChildren();

		ImGui::End();

		// moving and resizing if values changed
		if (pos.x != GetBounds().x || pos.y != GetBounds().y)
			SetPosition({ pos.x, pos.y });
		if (size.x != GetBounds().x || size.y != GetBounds().h)
			SetSize({ size.x, size.y });

		if (!open)
			Hide();
	}
	WindowFlags WindowWidget::GetWindowFlags()
	{
		WindowFlags flags = ImGuiWindowFlags_None;

		if (!m_includeTitleBar)
			flags |= ImGuiWindowFlags_NoTitleBar;
		if (!m_includeScrollBar)
			flags |= ImGuiWindowFlags_NoScrollbar;
		if (m_includeMenuBar)
			flags |= ImGuiWindowFlags_MenuBar;
		if (!m_canMove)
			flags |= ImGuiWindowFlags_NoMove;
		if (!m_canResize)
			flags |= ImGuiWindowFlags_NoResize;
		if (!m_canCollapse)
			flags |= ImGuiWindowFlags_NoCollapse;
		if (!m_includeNav)
			flags |= ImGuiWindowFlags_NoNav;
		if (!m_includeBackground)
			flags |= ImGuiWindowFlags_NoBackground;
		if (!m_shouldBringToFrontOnFocus)
			flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
		if (!m_enableDocking)
			flags |= ImGuiWindowFlags_NoDocking;

		return flags;
	}
}