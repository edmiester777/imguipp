#include <imguipp.h>
#include <impl/dx12_window.h>
#include <memory>
#include <thread>
#include <chrono>

using namespace imguipp;

int main(int argc, char* argv[])
{
	std::shared_ptr<Dx12Window> window(new Dx12Window());
	window->CaptureMouse(true);
	window->CaptureKeyboard(true);
	window->EnableDocking();
	window->EnableKeyboardNavigation();
	
	{
		std::shared_ptr<WindowWidget> wwidget(new WindowWidget("Example Window Widget"));
		window->AddChild(wwidget);
	}

	window->Show();
	window->Join();
}
