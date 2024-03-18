#pragma once

#include "Device.h"
#include "CommandQueue.h"
#include "SwapChain.h"
#include "RootSignature.h"
#include "Mesh.h"
#include "Shader.h"
#include "ConstantBuffer.h"
#include "TableDescriptorHeap.h"
#include "Texture.h"
#include "RenderTargetGroup.h"

class Engine
{
public:
	void Init(const WindowInfo& info);
	void Update();

public:
	const WindowInfo&					GetWindow()										{ return window; }
	shared_ptr<Device>					GetDevice()										{ return device; }
	shared_ptr<GraphicsCommandQueue>	GetGraphicsCmdQueue()							{ return graphicsCommandQueue; }
	shared_ptr<ComputeCommandQueue>		GetComputeCmdQueue()							{ return computeCommandQueue; }
	shared_ptr<SwapChain>				GetSwapChain()									{ return swapChain; }
	shared_ptr<RootSignature>			GetRootSignature()								{ return rootSignature; }
	shared_ptr<GraphicsDescriptorHeap>	GetGraphicsDescHeap()							{ return graphicsDescriptorHeap; }
	shared_ptr<ComputeDescriptorHeap>	GetComputeDescHeap()							{ return computeDescriptorHeap; }

	shared_ptr<ConstantBuffer>			GetConstantBuffer(CONSTANT_BUFFER_TYPE type)	{ return constantBuffers[static_cast<uint8>(type)]; }
	shared_ptr<RenderTargetGroup>		GetRTGroup(RENDER_TARGET_GROUP_TYPE type)		{ return renderTargetGroups[static_cast<uint8>(type)]; }

public:
	void Render();
	void RenderBegin();
	void RenderEnd();

	void ResizeWindow(int32 width, int32 height);

private:
	void ShowFps();
	void CreateConstantBuffer(CBV_REGISTER reg, uint32 bufferSize, uint32 count);
	void CreateRenderTargetGroups();

private:
	WindowInfo		window;
	D3D12_VIEWPORT	viewport    = {};
	D3D12_RECT		scissorRect = {};

	shared_ptr<Device>					device					= make_shared<Device>();
	shared_ptr<GraphicsCommandQueue>	graphicsCommandQueue	= make_shared<GraphicsCommandQueue>();
	shared_ptr<ComputeCommandQueue>		computeCommandQueue		= make_shared<ComputeCommandQueue>();
	shared_ptr<SwapChain>				swapChain				= make_shared<SwapChain>();
	shared_ptr<RootSignature>			rootSignature			= make_shared<RootSignature>();
	shared_ptr<GraphicsDescriptorHeap>	graphicsDescriptorHeap	= make_shared<GraphicsDescriptorHeap>();
	shared_ptr<ComputeDescriptorHeap>	computeDescriptorHeap	= make_shared<ComputeDescriptorHeap>();

	vector<shared_ptr<ConstantBuffer>>	constantBuffers;
	array<shared_ptr<RenderTargetGroup>, RENDER_TARGET_GROUP_COUNT> renderTargetGroups;
};

