#include <imguipp.h>
#include <impl/dx12_window.h>
#include <memory>

using namespace imguipp;

int main(int argc, char* argv[])
{
	std::shared_ptr<Dx12Window> window(new Dx12Window());
	window->CaptureMouse(true);
	window->CaptureKeyboard(true);
	window->EnableDocking();
	window->EnableKeyboardNavigation();
	window->Show();
	window->Join();
}
