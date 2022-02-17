#pragma once
#include "../Window.h"
#include "../backend_includes.h"
#include <d3d12.h>
#include <dxgi1_4.h>

#define DX12_FRAME_BUFFER_LENGTH 3
#define DX12_BACK_BUFFER_SIZE 3

namespace imguipp {
	extern struct ImGui_ImplWin32_Data;
	struct Dx12FrameContext
	{
		ID3D12CommandAllocator* CommandAllocator;
		UINT64 FenceValue;
	};

	class Dx12Window : public Window {
	public:
		Dx12Window();
		virtual ~Dx12Window();
		void EnableViewports();
		void CaptureMouse(bool capture);
		void CaptureKeyboard(bool capture);

		// overrides
		virtual void OnWindowTitleChanged() override;
		virtual void Close() override;
		virtual void Show() override;
		virtual void Hide() override;
		virtual void OnStart() override;

	protected:
		virtual void Render() override;
		virtual void OnResized() override;
		virtual void OnMove() override;
		
	private:
		static LRESULT CALLBACK handleWinMessage(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

		void SetupDirectXWindow();
		bool CreateDeviceD3D(HWND hwnd);
		void CleanupDeviceD3D();
		void WaitForLastSubmittedFrame();
		Dx12FrameContext* WaitForNextFrameResources();
		void CleanupRenderTarget();
		void CreateRenderTarget();
		bool HandleWindowMessages();

		HWND m_hwnd;
		Dx12FrameContext m_frameBuffer[DX12_FRAME_BUFFER_LENGTH] = {};
		unsigned int m_frameIndex;
		ID3D12Device* m_device;
		ID3D12DescriptorHeap* m_descHeap;
		ID3D12CommandQueue* m_commandQueue;
		ID3D12GraphicsCommandList* m_commandList;
		ID3D12Fence* m_fence;
		ID3D12Resource* m_resourceBackBuffer[DX12_BACK_BUFFER_SIZE] = {};
		D3D12_CPU_DESCRIPTOR_HANDLE m_descBackBuffer[DX12_BACK_BUFFER_SIZE] = {};
		HANDLE m_fenceEvent;
		uint64_t m_fenceLastSignal;
		IDXGISwapChain3* m_swapChain;
		HANDLE m_swapChainWaitableObject;
		WNDCLASSEX m_wc;
	};
}
