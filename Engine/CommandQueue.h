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

	void Init(ComPtr<ID3D12Device> device, shared_ptr<SwapChain> swapChain);
	void WaitSync();

	void RenderBegin();
	void RenderEnd();

	void FlushResourceCommandQueue();

	ComPtr<ID3D12CommandQueue>			GetCmdQueue()			{ return cmdQueue; }
	ComPtr<ID3D12GraphicsCommandList>	GetGraphicsCmdList()	{ return cmdList; }
	ComPtr<ID3D12GraphicsCommandList>	GetResourceCmdList()	{ return resourceCmdList; }

private:
	ComPtr<ID3D12CommandQueue>			cmdQueue;
	ComPtr<ID3D12CommandAllocator>		cmdAlloc;
	ComPtr<ID3D12GraphicsCommandList>	cmdList;

	ComPtr<ID3D12CommandAllocator>		resourceCmdAlloc;
	ComPtr<ID3D12GraphicsCommandList>	resourceCmdList;

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

	ComPtr<ID3D12CommandQueue>			GetCmdQueue()		{ return cmdQueue; }
	ComPtr<ID3D12GraphicsCommandList>	GetComputeCmdList() { return cmdList; }

private:
	ComPtr<ID3D12CommandQueue>			cmdQueue;
	ComPtr<ID3D12CommandAllocator>		cmdAlloc;
	ComPtr<ID3D12GraphicsCommandList>	cmdList;

	ComPtr<ID3D12Fence>					fence;
	uint32								fenceValue = 0;
	HANDLE								fenceEvent = INVALID_HANDLE_VALUE;
};