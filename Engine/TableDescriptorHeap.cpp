#include "pch.h"
#include "TableDescriptorHeap.h"
#include "Engine.h"

////////////////////////////
// GraphicsDescriptorHeap //
////////////////////////////

void GraphicsDescriptorHeap::Init(uint32 count)
{
	groupCount = count;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = count * (CBV_SRV_REGISTER_COUNT - 1);
	desc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;		// shader에서 접근 가능
	desc.Type			= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;			// SRV, CBV, UAV를 모두 다룰 수 있는 힙

	DEVICE->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descHeap));

	handleSize	= DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	groupSize	= handleSize * (CBV_SRV_REGISTER_COUNT - 1); // b0는 전역
}

void GraphicsDescriptorHeap::Clear()
{
	currentGroupIndex = 0;
}

void GraphicsDescriptorHeap::SetCBV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle, CBV_REGISTER reg)
{
	D3D12_CPU_DESCRIPTOR_HANDLE destHandle = GetCPUHandle(reg);

	uint32 destRange = 1;
	uint32 srcRange = 1;
	DEVICE->CopyDescriptors(1, &destHandle, &destRange, 1, &srcHandle, &srcRange, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void GraphicsDescriptorHeap::SetSRV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle, SRV_REGISTER reg)
{
	D3D12_CPU_DESCRIPTOR_HANDLE destHandle = GetCPUHandle(reg);

	uint32 destRange = 1;
	uint32 srcRange = 1;
	DEVICE->CopyDescriptors(1, &destHandle, &destRange, 1, &srcHandle, &srcRange, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void GraphicsDescriptorHeap::CommitTable()
{
	D3D12_GPU_DESCRIPTOR_HANDLE handle = descHeap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += currentGroupIndex * groupSize;
	GRAPHICS_CMD_LIST->SetGraphicsRootDescriptorTable(1, handle);	// 1번 root parameter에 할당

	currentGroupIndex++;
}

D3D12_CPU_DESCRIPTOR_HANDLE GraphicsDescriptorHeap::GetCPUHandle(CBV_REGISTER reg)
{
	return GetCPUHandle(static_cast<uint8>(reg));
}

D3D12_CPU_DESCRIPTOR_HANDLE GraphicsDescriptorHeap::GetCPUHandle(SRV_REGISTER reg)
{
	return GetCPUHandle(static_cast<uint8>(reg));
}

D3D12_CPU_DESCRIPTOR_HANDLE GraphicsDescriptorHeap::GetCPUHandle(uint8 reg)
{
	assert(reg > 0);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = descHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += currentGroupIndex * groupSize;
	handle.ptr += (reg - 1) * handleSize;
	return handle;
}

///////////////////////////
// ComputeDescriptorHeap //
///////////////////////////

void ComputeDescriptorHeap::Init()
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = TOTAL_REGISTER_COUNT;
	desc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.Type			= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	DEVICE->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descHeap));

	handleSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void ComputeDescriptorHeap::SetCBV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle, CBV_REGISTER reg)
{
	D3D12_CPU_DESCRIPTOR_HANDLE destHandle = GetCPUHandle(reg);

	uint32 destRange = 1;
	uint32 srcRange = 1;
	DEVICE->CopyDescriptors(1, &destHandle, &destRange, 1, &srcHandle, &srcRange, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void ComputeDescriptorHeap::SetSRV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle, SRV_REGISTER reg)
{
	D3D12_CPU_DESCRIPTOR_HANDLE destHandle = GetCPUHandle(reg);

	uint32 destRange = 1;
	uint32 srcRange = 1;
	DEVICE->CopyDescriptors(1, &destHandle, &destRange, 1, &srcHandle, &srcRange, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void ComputeDescriptorHeap::SetUAV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle, UAV_REGISTER reg)
{
	D3D12_CPU_DESCRIPTOR_HANDLE destHandle = GetCPUHandle(reg);

	uint32 destRange = 1;
	uint32 srcRange = 1;
	DEVICE->CopyDescriptors(1, &destHandle, &destRange, 1, &srcHandle, &srcRange, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// TODO : 리소스 상태 변경
}

void ComputeDescriptorHeap::CommitTable()
{
	ID3D12DescriptorHeap* descHeap = descHeap.Get();
	COMPUTE_CMD_LIST->SetDescriptorHeaps(1, &descHeap);	

	D3D12_GPU_DESCRIPTOR_HANDLE handle = descHeap->GetGPUDescriptorHandleForHeapStart();
	COMPUTE_CMD_LIST->SetComputeRootDescriptorTable(0, handle);
}

D3D12_CPU_DESCRIPTOR_HANDLE ComputeDescriptorHeap::GetCPUHandle(CBV_REGISTER reg)
{
	return GetCPUHandle(static_cast<uint8>(reg));
}

D3D12_CPU_DESCRIPTOR_HANDLE ComputeDescriptorHeap::GetCPUHandle(SRV_REGISTER reg)
{
	return GetCPUHandle(static_cast<uint8>(reg));
}

D3D12_CPU_DESCRIPTOR_HANDLE ComputeDescriptorHeap::GetCPUHandle(UAV_REGISTER reg)
{
	return GetCPUHandle(static_cast<uint8>(reg));
}

D3D12_CPU_DESCRIPTOR_HANDLE ComputeDescriptorHeap::GetCPUHandle(uint8 reg)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle = descHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += reg * handleSize;
	return handle;
}