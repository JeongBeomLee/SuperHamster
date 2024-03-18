#include "pch.h"
#include "CommandQueue.h"
#include "SwapChain.h"
#include "Engine.h"

//////////////////////////
// GraphicsCommandQueue //
//////////////////////////

GraphicsCommandQueue::~GraphicsCommandQueue()
{
	CloseHandle(fenceEvent);
}

void GraphicsCommandQueue::Init(ComPtr<ID3D12Device> device, shared_ptr<SwapChain> pSwapChain)
{
	swapChain = pSwapChain;

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	// Create Graphics Command Queue
	device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));
	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
	commandList->Close();

	// Create Resource Command Queue 
	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&resourceCommandAllocator));
	device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, resourceCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&resourceCommandList));

	// CreateFence
	// - CPU와 GPU의 동기화 수단으로 쓰인다
	device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void GraphicsCommandQueue::WaitSync()
{
	fenceValue++;
	commandQueue->Signal(fence.Get(), fenceValue);

	if (fence->GetCompletedValue() < fenceValue) {
		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}
}

void GraphicsCommandQueue::RenderBegin()
{
	commandAllocator->Reset();
	commandList->Reset(commandAllocator.Get(), nullptr);

	int8 backIndex = swapChain->GetBackBufferIndex();

	D3D12_RESOURCE_BARRIER barrier = 
		CD3DX12_RESOURCE_BARRIER::Transition(
		gEngine->GetRTGroup(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)->GetRTTexture(backIndex)->GetTex2D().Get(),
		D3D12_RESOURCE_STATE_PRESENT, // 화면 출력
		D3D12_RESOURCE_STATE_RENDER_TARGET); // 외주 결과물

	// Root Signature는 오브젝트 타입마다 다르게 설정해줄 수 있음.
	commandList->SetGraphicsRootSignature(GRAPHICS_ROOT_SIGNATURE.Get());

	gEngine->GetConstantBuffer(CONSTANT_BUFFER_TYPE::TRANSFORM)->Clear();
	gEngine->GetConstantBuffer(CONSTANT_BUFFER_TYPE::MATERIAL)->Clear();

	gEngine->GetGraphicsDescHeap()->Clear();

	ID3D12DescriptorHeap* descHeap = gEngine->GetGraphicsDescHeap()->GetDescriptorHeap().Get();
	commandList->SetDescriptorHeaps(1, &descHeap);	// 1번 Descripter Heap을 사용하겠다.

	commandList->ResourceBarrier(1, &barrier);
}

void GraphicsCommandQueue::RenderEnd()
{
	int8 backIndex = swapChain->GetBackBufferIndex();

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		gEngine->GetRTGroup(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)->GetRTTexture(backIndex)->GetTex2D().Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, // 외주 결과물
		D3D12_RESOURCE_STATE_PRESENT); // 화면 출력

	commandList->ResourceBarrier(1, &barrier);
	commandList->Close();

	// 커맨드 리스트 수행
	ID3D12CommandList* cmdListArr[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(cmdListArr), cmdListArr);

	swapChain->Present();

	WaitSync();

	swapChain->SwapIndex();
}

void GraphicsCommandQueue::FlushResourceCommandQueue()
{
	resourceCommandList->Close();

	ID3D12CommandList* cmdListArr[] = { resourceCommandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(cmdListArr), cmdListArr);

	WaitSync();

	resourceCommandAllocator->Reset();
	resourceCommandList->Reset(resourceCommandAllocator.Get(), nullptr);
}

/////////////////////////
// ComputeCommandQueue //
/////////////////////////

ComputeCommandQueue::~ComputeCommandQueue()
{
	CloseHandle(fenceEvent);
}

void ComputeCommandQueue::Init(ComPtr<ID3D12Device> device)
{
	D3D12_COMMAND_QUEUE_DESC computeQueueDesc{};
	computeQueueDesc.Type  = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	computeQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	device->CreateCommandQueue(&computeQueueDesc, IID_PPV_ARGS(&commandQueue));
	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&commandAllocator));
	device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));

	// CreateFence
	// - CPU와 GPU의 동기화 수단으로 쓰인다
	device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void ComputeCommandQueue::WaitSync()
{
	++fenceValue;
	commandQueue->Signal(fence.Get(), fenceValue);

	if (fence->GetCompletedValue() < fenceValue) {
		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}
}

void ComputeCommandQueue::FlushComputeCommandQueue()
{
	commandList->Close();

	ID3D12CommandList* cmdListArr[] = { commandList.Get() };
	auto t = _countof(cmdListArr);
	commandQueue->ExecuteCommandLists(_countof(cmdListArr), cmdListArr);

	WaitSync();

	commandAllocator->Reset();
	commandList->Reset(commandAllocator.Get(), nullptr);

	COMPUTE_CMD_LIST->SetComputeRootSignature(COMPUTE_ROOT_SIGNATURE.Get());
}