#include "pch.h"
#include "Device.h"

void Device::Init()
{
#ifdef _DEBUG
	// ����� ���̾� ���
	D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
	debugController->EnableDebugLayer();
#endif
	// DXGI ���丮�� ����̽� ����
	CreateDXGIFactory(IID_PPV_ARGS(&dxgi));
	D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
}
