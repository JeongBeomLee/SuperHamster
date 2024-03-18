#include "pch.h"
#include "Engine.h"
#include "Material.h"
#include "Transform.h"
#include "Input.h"
#include "Timer.h"
#include "SceneManager.h"
#include "Light.h"
#include "Resources.h"
#include "InstancingManager.h"

void Engine::Init(const WindowInfo& info)
{
	window = info;	

	// 그려질 화면 크기를 설정
	viewport	= { 0, 0, static_cast<FLOAT>(info.width), static_cast<FLOAT>(info.height), 0.0f, 1.0f };
	scissorRect = CD3DX12_RECT(0, 0, info.width, info.height);

	// 엔진 초기화
	device->Init();
	graphicsCommandQueue->Init(device->GetDevice(), swapChain);
	computeCommandQueue->Init(device->GetDevice());
	swapChain->Init(info, device->GetDevice(), device->GetDXGI(), graphicsCommandQueue->GetCmdQueue());
	rootSignature->Init();
	graphicsDescriptorHeap->Init(256);
	computeDescriptorHeap->Init();

	// 상수 버퍼 생성
	CreateConstantBuffer(CBV_REGISTER::b0, sizeof(LightParams), 1);
	CreateConstantBuffer(CBV_REGISTER::b1, sizeof(TransformParams), 256);
	CreateConstantBuffer(CBV_REGISTER::b2, sizeof(MaterialParams), 256);

	// 렌더 타겟 그룹 생성
	CreateRenderTargetGroups();

	ResizeWindow(info.width, info.height);

	GET_SINGLE(Input)->Init(info.hwnd);
	GET_SINGLE(Timer)->Init();
	GET_SINGLE(Resources)->Init();
}

void Engine::Update()
{
	GET_SINGLE(Input)->Update();
	GET_SINGLE(Timer)->Update();
	GET_SINGLE(SceneManager)->Update();
	GET_SINGLE(InstancingManager)->ClearBuffer();

	Render();

	ShowFps();
}

void Engine::Render()
{
	// DX12에선 CommandQueue에 기본 설정된 RTV, DSV, Viewport가 없음.
	// -> DX12에선 Command List를 사용할 때마다 RTV, DSV, Viewport를 설정할 필요가 있음.
	// -> OMSetRenderTarget(), RSSetViewport등은 커맨드 초반에 먼저 호출해줄것.
	// -> Fence -> Wait -> Complete 될 때까지 리소스 상태를 유지할것. 즉, draw call이 끝날때까지 모든 리소스를 유지할것.
	
	// 현재 엔진은 하나의 Command List만 사용하고 있다.
	// 하나의 Command List에 모든 명령을 때려박고, 마지막에 Execute를 한 번 해주면, 그 전 까지 GPU는 펑펑 놀다가 마지막에 과부하를 받는다.
	// 따라서 여러 개의 Command List를 사용하여, 병렬로 명령을 처리하도록 해야 한다. -> Multi-Thread Rendering이 필요하다.
	// -> 게임 루프 중간에서 수동으로 자주자주 커맨드 리스트를 전송해야한다.
	// 
	// CommandListPooling
	// : 여러 개의 Command List를 미리 생성해둔다.
	// : Command List 하나당 N개의 오브젝트 렌더링에 대한 Command를 기록.
	// : N개에 도달하면 Execute(), 다음번 Command List에 계속해서 렌더링 Command 기록.
	// : 모든 오브젝트를 다 렌더링하거나 할당해 둔 Command List를 다 사용할 때까지 반복.
	// 
	// Multi-Thread Rendering (필수)
	// : Multi-Thread로 Command 작성 시간을 최소화시킨다.
	// : 단순계산으로 n이란 시간이 걸린다면 4 thread를 사용하면 시간을 n / 4로 줄일 수 있다.
	// : GPU Queue가 empty 되지 않도록 최선을 다한다.

	RenderBegin();

	GET_SINGLE(SceneManager)->Render();

	RenderEnd();
}

void Engine::RenderBegin()
{
	graphicsCommandQueue->RenderBegin();
}

void Engine::RenderEnd()
{
	graphicsCommandQueue->RenderEnd();
}

void Engine::ResizeWindow(int32 width, int32 height)
{
	window.width = width;
	window.height = height;

	RECT rect = { 0, 0, width, height };
	::AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
	::SetWindowPos(window.hwnd, 0, 100, 100, width, height, 0);
}

void Engine::ShowFps()
{
	uint32 fps = GET_SINGLE(Timer)->GetFps();

	WCHAR text[100] = L"";
	::wsprintf(text, L"FPS : %d", fps);

	::SetWindowText(window.hwnd, text);
}

void Engine::CreateConstantBuffer(CBV_REGISTER reg, uint32 bufferSize, uint32 count)
{
	uint8 typeInt = static_cast<uint8>(reg);
	assert(constantBuffers.size() == typeInt);

	shared_ptr<ConstantBuffer> buffer = make_shared<ConstantBuffer>();
	buffer->Init(reg, bufferSize, count);
	constantBuffers.push_back(buffer);
}


void Engine::CreateRenderTargetGroups()
{
	// DepthStencil
	shared_ptr<Texture> dsTexture = GET_SINGLE(Resources)->CreateTexture(L"DepthStencil",
		DXGI_FORMAT_D32_FLOAT, window.width, window.height,
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	// SwapChain Group
	{
		vector<RenderTarget> rtVec(SWAP_CHAIN_BUFFER_COUNT);

		for (uint32 i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
		{
			wstring name = L"SwapChainTarget_" + std::to_wstring(i);

			ComPtr<ID3D12Resource> resource;
			swapChain->GetSwapChain()->GetBuffer(i, IID_PPV_ARGS(&resource));
			rtVec[i].target = GET_SINGLE(Resources)->CreateTextureFromResource(name, resource);
		}

		renderTargetGroups[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)] = make_shared<RenderTargetGroup>();
		renderTargetGroups[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)]->Create(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN, rtVec, dsTexture);
	}

	// Shadow Group
	{
		vector<RenderTarget> rtVec(RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT);

		rtVec[0].target = GET_SINGLE(Resources)->CreateTexture(L"ShadowTarget",
			DXGI_FORMAT_R32_FLOAT, 4096, 4096,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

		shared_ptr<Texture> shadowDepthTexture = GET_SINGLE(Resources)->CreateTexture(L"ShadowDepthStencil",
			DXGI_FORMAT_D32_FLOAT, 4096, 4096,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

		renderTargetGroups[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::SHADOW)] = make_shared<RenderTargetGroup>();
		renderTargetGroups[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::SHADOW)]->Create(RENDER_TARGET_GROUP_TYPE::SHADOW, rtVec, shadowDepthTexture);
	}

	// Deferred Group
	{
		vector<RenderTarget> rtVec(RENDER_TARGET_G_BUFFER_GROUP_MEMBER_COUNT);

		rtVec[0].target = GET_SINGLE(Resources)->CreateTexture(L"PositionTarget",
			DXGI_FORMAT_R32G32B32A32_FLOAT, window.width, window.height,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

		rtVec[1].target = GET_SINGLE(Resources)->CreateTexture(L"NormalTarget",
			DXGI_FORMAT_R32G32B32A32_FLOAT, window.width, window.height,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

		rtVec[2].target = GET_SINGLE(Resources)->CreateTexture(L"DiffuseTarget",
			DXGI_FORMAT_R8G8B8A8_UNORM, window.width, window.height,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

		renderTargetGroups[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::G_BUFFER)] = make_shared<RenderTargetGroup>();
		renderTargetGroups[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::G_BUFFER)]->Create(RENDER_TARGET_GROUP_TYPE::G_BUFFER, rtVec, dsTexture);
	}

	// Lighting Group
	{
		vector<RenderTarget> rtVec(RENDER_TARGET_LIGHTING_GROUP_MEMBER_COUNT);

		rtVec[0].target = GET_SINGLE(Resources)->CreateTexture(L"DiffuseLightTarget",
			DXGI_FORMAT_R8G8B8A8_UNORM, window.width, window.height,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

		rtVec[1].target = GET_SINGLE(Resources)->CreateTexture(L"SpecularLightTarget",
			DXGI_FORMAT_R8G8B8A8_UNORM, window.width, window.height,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

		renderTargetGroups[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::LIGHTING)] = make_shared<RenderTargetGroup>();
		renderTargetGroups[static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::LIGHTING)]->Create(RENDER_TARGET_GROUP_TYPE::LIGHTING, rtVec, dsTexture);
	}
}