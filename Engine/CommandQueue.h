#pragma once

class SwapChain;
class DescriptorHeap;

//////////////////////////
// GraphicsCommandQueue //
//////////////////////////

class GraphicsCommandQueue
{
public:
	~GraphicsCommandQueue();

	void Init(ComPtr<ID3D12Device> device, shared_ptr<SwapChain> pSwapChain);
	void WaitSync();

	void RenderBegin();
	void RenderEnd();

	void FlushResourceCommandQueue();

	ComPtr<ID3D12CommandQueue>			GetCmdQueue()			{ return commandQueue; }
	ComPtr<ID3D12GraphicsCommandList>	GetGraphicsCmdList()	{ return commandList; }
	ComPtr<ID3D12GraphicsCommandList>	GetResourceCmdList()	{ return resourceCommandList; }

private:
	ComPtr<ID3D12CommandQueue>			commandQueue;
	ComPtr<ID3D12CommandAllocator>		commandAllocator;
	ComPtr<ID3D12GraphicsCommandList>	commandList;

	ComPtr<ID3D12CommandAllocator>		resourceCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList>	resourceCommandList;

	ComPtr<ID3D12Fence>					fence;
	uint32								fenceValue = 0;
	HANDLE								fenceEvent = INVALID_HANDLE_VALUE;

	shared_ptr<SwapChain>				swapChain;
};

/////////////////////////
// ComputeCommandQueue //
/////////////////////////

class ComputeCommandQueue
{
public:
	~ComputeCommandQueue();

	void Init(ComPtr<ID3D12Device> device);
	void WaitSync();
	void FlushComputeCommandQueue();

	ComPtr<ID3D12CommandQueue>			GetCmdQueue()		{ return commandQueue; }
	ComPtr<ID3D12GraphicsCommandList>	GetComputeCmdList() { return commandList; }

private:
	ComPtr<ID3D12CommandQueue>			commandQueue;
	ComPtr<ID3D12CommandAllocator>		commandAllocator;
	ComPtr<ID3D12GraphicsCommandList>	commandList;

	ComPtr<ID3D12Fence>					fence;
	uint32								fenceValue = 0;
	HANDLE								fenceEvent = INVALID_HANDLE_VALUE;
};