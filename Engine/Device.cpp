#include "pch.h"
#include "Device.h"
void Device::Init()
{
	ID3D12Debug* pDebugController = nullptr; 
	ID3D12Debug5* pDebugController5 = nullptr;
#ifdef _DEBUG
	// D3D12 debug layer 劝己拳
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController)))) {
		pDebugController->EnableDebugLayer();
	}
	if (S_OK == pDebugController->QueryInterface(IID_PPV_ARGS(&pDebugController5))) {
		pDebugController5->SetEnableGPUBasedValidation(TRUE);
		pDebugController5->SetEnableAutoName(TRUE);
		pDebugController5->Release();
		pDebugController5 = nullptr;
	}
	if (pDebugController) {
		pDebugController->Release();
		pDebugController = nullptr;
	}
#endif
	// DXGIFactory 积己
	CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));

	// Device 积己
	D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device));

	if (pDebugController) {
		ID3D12InfoQueue* pInfoQueue = nullptr;
		device->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
		if (pInfoQueue) {
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

			D3D12_MESSAGE_ID hide[] = {
				D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
				D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
				// Workarounds for debug layer issues on hybrid-graphics systems
				D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
				D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
			};

			D3D12_INFO_QUEUE_FILTER filter = {};
			filter.DenyList.NumIDs = (UINT)_countof(hide);
			filter.DenyList.pIDList = hide;
			pInfoQueue->AddStorageFilterEntries(&filter);

			pInfoQueue->Release();
			pInfoQueue = nullptr;
		}
	}
}
