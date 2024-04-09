#pragma once

class RootSignature
{
public:
	void Init();

	ComPtr<ID3D12RootSignature>	GetGraphicsRootSignature()	{ return _graphicsRootSignature; }
	ComPtr<ID3D12RootSignature>	GetComputeRootSignature()	{ return _computeRootSignature; }
	ComPtr<ID3D12DescriptorHeap> GetSamplerHeap()			{ return _samplerHeap; }

private:
	void CreateGraphicsRootSignature();
	void CreateComputeRootSignature();

private:
	//D3D12_STATIC_SAMPLER_DESC	_samplerDesc;	// 정적 샘플러는 티어링을 유발하므로 사용X
	ComPtr<ID3D12DescriptorHeap> _samplerHeap;	// 동적 샘플러 힙 사용

	ComPtr<ID3D12RootSignature>	_graphicsRootSignature;	
	ComPtr<ID3D12RootSignature>	_computeRootSignature;
};

