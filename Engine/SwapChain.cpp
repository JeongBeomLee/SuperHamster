#include "pch.h"
#include "SwapChain.h"
#include "Engine.h"

void SwapChain::Init(const WindowInfo& info, ComPtr<ID3D12Device> device, ComPtr<IDXGIFactory> dxgi, ComPtr<ID3D12CommandQueue> cmdQueue)
{
	CreateSwapChain(info, dxgi, cmdQueue);
}

void SwapChain::Present()
{
	// Present the frame.
	dxgiSwapChain->Present(0, 0);
}

void SwapChain::SwapIndex()
{
	backBufferIndex = (backBufferIndex + 1) % SWAP_CHAIN_BUFFER_COUNT;
}

void SwapChain::CreateSwapChain(const WindowInfo& info, ComPtr<IDXGIFactory> dxgi, ComPtr<ID3D12CommandQueue> cmdQueue)
{
	dxgiSwapChain.Reset();

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferDesc.Width = static_cast<uint32>(info.width);	// 버퍼의 해상도 너비
	swapChainDesc.BufferDesc.Height	= static_cast<uint32>(info.height);	// 버퍼의 해상도 높이
	swapChainDesc.BufferDesc.Format	= DXGI_FORMAT_R8G8B8A8_UNORM;		// 버퍼의 디스플레이 형식
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;				// 화면 갱신 비율
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;				// 화면 갱신 비율
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.SampleDesc.Count = 1;									// 멀티 샘플링 사용 안함
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;		// 후면 버퍼에 렌더링할 것 
	swapChainDesc.BufferCount = SWAP_CHAIN_BUFFER_COUNT;				// 전면 + 후면
	swapChainDesc.OutputWindow = info.hwnd;
	swapChainDesc.Windowed = info.windowed;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;			// 전면 후면 버퍼 교체 시 이전 프레임 정보 버림
	swapChainDesc.Flags	= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	dxgi->CreateSwapChain(cmdQueue.Get(), &swapChainDesc, &dxgiSwapChain);
}