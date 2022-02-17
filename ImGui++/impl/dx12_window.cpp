// A lot about this file is very alpha. Major changes are needed before it can be considered
// complete.
// TODO list
// 1. Manually implement ImGui contexts for this specific window. We want to allow implementers
//    to create multiple windows. Each of which will need their own ImGui context.
//    imgui_impl_win32.cpp and imgui_impl_dx12.cpp both use global variables that we will need to
//    replace with members in the future.
// 2. Threading for show/hide and startup behavior need to be reconsidered. There are some problems
//    with the current approach. The top things that come to mind is:
//      a) HWND needs to be created in the same thread that rendering is done so we may use message
//         processing (::PeekMessage) for mouse / keyboard inputs. This fact presents an issue where
//         the normal flow of (create -> set props -> show -> close -> join) encounters some flow 
//         issues. We need to have a HWND to show / hide but we can't create an hwnd without our
//         render thread being set up. May need to do some unique_lock stuff later on to begin render
//         thread and wait on OnStart() to be called before completing construction.
//      b) Resource cleanup is not complete. We must ensure we release resources on Close().
// 3. No window customization. Right now window is minimal with only the render. No customization or
//    titlebar. For broad-adoption and to meet expectations of existing frameworks, we must allow for
//    customization of these properties.
//    I tried to include this, but I've done something wrong with my initialization to where I do not
//    quite understand why ::CreateWindow(..., WS_OVERLAPPEDWINDOW, ...) does not have a titlebar or
//    buttons. I'll investigate later.
//
// I will be continuing for now and loop back to this when I have a clearer head.

#include "dx12_window.h"
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx12.h>

#pragma comment(lib, "dxgi")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

namespace imguipp {

	Dx12Window::Dx12Window() : Window()
	{
		m_frameIndex = 0;
		m_device = NULL;
		m_descHeap = NULL;
		m_commandQueue = NULL;
		m_commandList = NULL;
		m_fence = NULL;
		m_fenceEvent = NULL;
		m_fenceLastSignal = 0;
		m_swapChain = NULL;
		m_swapChainWaitableObject = NULL;

		SetTitle("Untitled DX12 Window", false);
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
	}

	Dx12Window::~Dx12Window()
	{
		Window::~Window();
	}

	void Dx12Window::EnableViewports()
	{
		ImGuiIO& io = ImGui::GetIO();
		if (!(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
			io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	}

	void Dx12Window::CaptureMouse(bool capture)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.WantCaptureMouse = capture;
	}

	void Dx12Window::CaptureKeyboard(bool capture)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.WantCaptureMouse = capture;
	}

	void Dx12Window::OnWindowTitleChanged()
	{
		::SetWindowText(m_hwnd, GetTitle().c_str());
	}

	void Dx12Window::Close()
	{
		Window::Close();
		WaitForLastSubmittedFrame();
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		CleanupDeviceD3D();
		::DestroyWindow(m_hwnd);
		::UnregisterClass(m_wc.lpszClassName, m_wc.hInstance);
		TriggerOnCloseEvent();
	}

	void Dx12Window::Show()
	{
		if (m_hwnd != NULL)
			ShowWindow(m_hwnd, SW_NORMAL);

		Window::Show();
	}

	void Dx12Window::Hide()
	{
		if (m_hwnd != NULL)
			ShowWindow(m_hwnd, SW_HIDE);

		Window::Hide();
	}

	void Dx12Window::OnStart()
	{
		SetupDirectXWindow();

		// show the new window
		::ShowWindow(m_hwnd, SW_SHOWDEFAULT);
		::UpdateWindow(m_hwnd);
	}

	void Dx12Window::Render()
	{
		if (!HandleWindowMessages())
			return;

		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		RenderChildren();

		ImGui::Render();

		Dx12FrameContext* frame = WaitForNextFrameResources();
		UINT bufferindex = m_swapChain->GetCurrentBackBufferIndex();
		frame->CommandAllocator->Reset();

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = m_resourceBackBuffer[bufferindex];
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		m_commandList->Reset(frame->CommandAllocator, NULL);
		m_commandList->ResourceBarrier(1, &barrier);

		// Render Dear ImGui graphics
		static ImVec4 clear_color(0.45f, 0.55f, 0.60f, 1.00f);
		const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
		m_commandList->ClearRenderTargetView(m_descBackBuffer[bufferindex], clear_color_with_alpha, 0, NULL);
		m_commandList->OMSetRenderTargets(1, &m_descBackBuffer[bufferindex], FALSE, NULL);
		m_commandList->SetDescriptorHeaps(1, &m_descHeap);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList);
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		m_commandList->ResourceBarrier(1, &barrier);
		m_commandList->Close();

		m_commandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&m_commandList);

		// Update and Render additional Platform Windows
		ImGuiIO io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault(NULL, (void*)m_commandList);
		}

		m_swapChain->Present(1, 0); // Present with vsync
		//g_pSwapChain->Present(0, 0); // Present without vsync

		UINT64 fenceValue = m_fenceLastSignal + 1;
		m_commandQueue->Signal(m_fence, fenceValue);
		m_fenceLastSignal = fenceValue;
		frame->FenceValue = fenceValue;
	}

	void Dx12Window::OnResized()
	{
	}

	void Dx12Window::OnMove()
	{
	}

	void Dx12Window::SetupDirectXWindow()
	{
		// Create window
		m_wc = {
			sizeof(WNDCLASSEX),
			CS_CLASSDC,
			Dx12Window::handleWinMessage,
			0L, 0L,
			GetModuleHandle(NULL),
			NULL, NULL, NULL, NULL,
			"DX12WND",
			NULL
		};
		::RegisterClassEx(&m_wc);
		Point loc = GetLocation();
		Size size = GetSize();
		m_hwnd = ::CreateWindow(
			m_wc.lpszClassName,
			NULL,
			WS_OVERLAPPEDWINDOW,
			loc.x, loc.y,
			size.w, size.h,
			NULL, NULL,
			m_wc.hInstance,
			NULL
		);
		::SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

		if (!CreateDeviceD3D(m_hwnd))
		{
			CleanupDeviceD3D();
			::UnregisterClass(m_wc.lpszClassName, m_wc.hInstance);
			exit(1);
		}

		// setting up platforms
		ImGui_ImplWin32_Init(m_hwnd);
		ImGui_ImplDX12_Init(
			m_device,
			DX12_FRAME_BUFFER_LENGTH,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			m_descHeap,
			m_descHeap->GetCPUDescriptorHandleForHeapStart(),
			m_descHeap->GetGPUDescriptorHandleForHeapStart()
		);
	}

	bool Dx12Window::CreateDeviceD3D(HWND hwnd)
	{
		DXGI_SWAP_CHAIN_DESC1 sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.BufferCount = DX12_BACK_BUFFER_SIZE;
		sd.Width = 0;
		sd.Height = 0;
		sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		sd.Scaling = DXGI_SCALING_STRETCH;
		sd.Stereo = FALSE;

		// Create device
		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
		if (D3D12CreateDevice(NULL, featureLevel, IID_PPV_ARGS(&m_device)) != S_OK)
			return false;

		{
			D3D12_DESCRIPTOR_HEAP_DESC desc = {};
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			desc.NumDescriptors = DX12_BACK_BUFFER_SIZE;
			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			desc.NodeMask = 1;
			if (m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_descHeap)) != S_OK)
				return false;

			SIZE_T rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_descHeap->GetCPUDescriptorHandleForHeapStart();
			for (UINT i = 0; i < DX12_BACK_BUFFER_SIZE; i++)
			{
				m_descBackBuffer[i] = rtvHandle;
				rtvHandle.ptr += rtvDescriptorSize;
			}
		}

		{
			D3D12_DESCRIPTOR_HEAP_DESC desc = {};
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			desc.NumDescriptors = 1;
			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			if (m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_descHeap)) != S_OK)
				return false;
		}

		{
			D3D12_COMMAND_QUEUE_DESC desc = {};
			desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			desc.NodeMask = 1;
			if (m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_commandQueue)) != S_OK)
				return false;
		}

		for (UINT i = 0; i < DX12_FRAME_BUFFER_LENGTH; ++i)
			if (m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_frameBuffer[i].CommandAllocator)) != S_OK)
				return false;

		if (m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_frameBuffer[0].CommandAllocator, NULL, IID_PPV_ARGS(&m_commandList)) != S_OK ||
			m_commandList->Close() != S_OK)
			return false;

		if (m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)) != S_OK)
			return false;

		m_fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (m_fenceEvent == NULL)
			return false;

		{
			IDXGIFactory4* dxgiFactory = NULL;
			IDXGISwapChain1* swapChain1 = NULL;
			if (CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)) != S_OK)
				return false;
			if (dxgiFactory->CreateSwapChainForHwnd(m_commandQueue, m_hwnd, &sd, NULL, NULL, &swapChain1) != S_OK)
				return false;
			if (swapChain1->QueryInterface(IID_PPV_ARGS(&m_swapChain)) != S_OK)
				return false;
			swapChain1->Release();
			dxgiFactory->Release();
			m_swapChain->SetMaximumFrameLatency(DX12_BACK_BUFFER_SIZE);
			m_swapChainWaitableObject = m_swapChain->GetFrameLatencyWaitableObject();
		}

		CreateRenderTarget();
		return true;
	}

	void Dx12Window::CleanupDeviceD3D()
	{
		CleanupRenderTarget();
		if (m_swapChain)
		{
			m_swapChain->SetFullscreenState(false, NULL);
			m_swapChain->Release();
			m_swapChain = NULL;
		}
		if (m_swapChainWaitableObject != NULL)
		{
			CloseHandle(m_swapChainWaitableObject);
		}
		for (UINT i = 0; i < DX12_FRAME_BUFFER_LENGTH; ++i)
		{
			if (m_frameBuffer[i].CommandAllocator)
			{
				m_frameBuffer[i].CommandAllocator->Release();
				m_frameBuffer[i].CommandAllocator = NULL;
			}
		}
		if (m_commandQueue)
		{
			m_commandQueue->Release();
			m_commandQueue = NULL;
		}
		if (m_commandList)
		{
			m_commandList->Release();
			m_commandList = NULL;
		}
		if (m_descHeap)
		{
			m_descHeap->Release();
			m_descHeap = NULL;
		}
		if (m_fence)
		{
			m_fence->Release();
			m_fence = NULL;
		}
		if (m_fenceEvent)
		{
			CloseHandle(m_fenceEvent);
			m_fenceEvent = NULL;
		}
		if (m_device)
		{
			m_device->Release();
			m_device = NULL;
		}
	}

	void Dx12Window::WaitForLastSubmittedFrame()
	{
		Dx12FrameContext* ctx = &m_frameBuffer[m_frameIndex % DX12_FRAME_BUFFER_LENGTH];

		UINT64 fenceVal = ctx->FenceValue;
		if (fenceVal == 0)
			return; // fence not signaled

		ctx->FenceValue = 0;
		if (m_fence->GetCompletedValue() >= fenceVal)
			return;

		m_fence->SetEventOnCompletion(fenceVal, m_fenceEvent);
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	Dx12FrameContext* Dx12Window::WaitForNextFrameResources()
	{
		++m_frameIndex;

		HANDLE waitableObjects[] = { m_swapChainWaitableObject, NULL };
		DWORD numWaitableObjects = 1;

		Dx12FrameContext* frameCtx = &m_frameBuffer[m_frameIndex % DX12_FRAME_BUFFER_LENGTH];
		UINT64 fenceValue = frameCtx->FenceValue;
		if (fenceValue != 0) // means no fence was signaled
		{
			frameCtx->FenceValue = 0;
			m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent);
			waitableObjects[1] = m_fenceEvent;
			numWaitableObjects = 2;
		}

		WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);

		return frameCtx;
	}

	void Dx12Window::CleanupRenderTarget()
	{
		WaitForLastSubmittedFrame();
		for (UINT i = 0; i < DX12_BACK_BUFFER_SIZE; ++i)
		{
			if (m_resourceBackBuffer[i])
			{
				m_resourceBackBuffer[i]->Release();
				m_resourceBackBuffer[i] = NULL;
			}
		}
	}

	void Dx12Window::CreateRenderTarget()
	{
		for (UINT i = 0; i < DX12_BACK_BUFFER_SIZE; ++i)
		{
			ID3D12Resource* backBuffer = NULL;
			m_swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
			m_device->CreateRenderTargetView(backBuffer, NULL, m_descBackBuffer[i]);
			m_resourceBackBuffer[i] = backBuffer;
		}
	}

	bool Dx12Window::HandleWindowMessages()
	{
		// check for a quit message
		int i = 0;
		MSG msg;
		::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE);
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
		if (msg.message == WM_QUIT)
		{
			Close();
			return false;
		}
		return true;
	}

	LRESULT Dx12Window::handleWinMessage(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
	{
		Dx12Window* window = (Dx12Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

		if (ImGui_ImplWin32_WndProcHandler(hwnd, message, wparam, lparam))
			return true;

		switch (message)
		{
		case WM_SIZE:
			// must resize our render target
			if (window->m_device != NULL && wparam != SIZE_MINIMIZED)
			{
				window->WaitForLastSubmittedFrame();
				window->CleanupRenderTarget();
				HRESULT result = window->m_swapChain->ResizeBuffers(
					0,
					(UINT)LOWORD(lparam),
					(UINT)HIWORD(lparam),
					DXGI_FORMAT_UNKNOWN,
					DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
				);
				assert(SUCCEEDED(result) && "Failed to resize swapchain.");
				window->CreateRenderTarget();
			}
			break;
		case WM_SYSCOMMAND:
			if ((wparam & 0xfff0) == SC_KEYMENU)
				return 0;
			break;
		case WM_DESTROY:
			window->TriggerOnCloseEvent();
			::PostQuitMessage(0);
			return 0;
		}

		return true;
	}
}
