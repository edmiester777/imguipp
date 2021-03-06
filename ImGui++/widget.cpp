#include "widget.h"

namespace imguipp {
	Widget::Widget()
	{
		Show();
		SetSize({
			100,
			100
		});
		SetPosition({
			0, 0
		});
	}

	Widget::~Widget()
	{

	}

	void Widget::Show()
	{
		m_show = true;
	}

	void Widget::Hide()
	{
		m_show = false;
	}

	void Widget::AddChild(std::shared_ptr<Widget> child)
	{
		m_children.push_back(child);
	}
	void Widget::SetSize(Size size)
	{
		m_bounds.w = size.w;
		m_bounds.h = size.h;
	}
	void Widget::SetPosition(Point position)
	{
		m_bounds.x = position.x;
		m_bounds.y = position.y;
	}
	const Rect& Widget::GetBounds()
	{
		return m_bounds;
	}
	void Widget::Clear()
	{
		m_children.clear();
	}
	void Widget::Remove(Widget* widget)
	{
		m_children.remove_if([this, widget](const std::shared_ptr<Widget> child) {
			return widget == child.get();
		});
	}
	void Widget::Remove(std::shared_ptr<Widget> widget)
	{
		m_children.remove(widget);
	}
	void Widget::RenderChildren()
	{
		std::list<std::shared_ptr<Widget>>::iterator iter;
		for (iter = m_children.begin(); iter != m_children.end(); iter++)
		{
			(*iter)->Render();
		}
	}
	bool Widget::Showing()
	{
		return m_show;
	}
	void Widget::SetParent(std::shared_ptr<Widget> parent)
	{
		if (m_parent)
			m_parent->Remove(this);

		m_parent = parent;
	}
}
