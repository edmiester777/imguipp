#pragma once

#include "common.h"
#include <list>
#include <memory>

namespace imguipp {
	class Widget {
	public:
		Widget();
		virtual ~Widget();

		virtual void Show();
		virtual void Hide();

		virtual void AddChild(std::shared_ptr<Widget> child);
		virtual void Render() = 0;
		void SetSize(Size size);
		void SetPosition(Point position);
		const Rect& GetBounds();


	protected:
		void RenderChildren();
		bool Showing();

	private:
		std::shared_ptr<Widget> m_parent;
		std::list<std::shared_ptr<Widget>> m_children;
		Rect m_bounds;
		bool m_show;
	};
}
