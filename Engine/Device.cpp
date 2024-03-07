#include "pch.h"
#include "Device.h"

void Device::Init()
{
#ifdef _DEBUG
	// 디버그 레이어 사용
	D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
	debugController->EnableDebugLayer();
#endif
	// DXGI 팩토리와 디바이스 생성
	CreateDXGIFactory(IID_PPV_ARGS(&dxgi));
	D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
}
