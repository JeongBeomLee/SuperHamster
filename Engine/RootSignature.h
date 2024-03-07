#pragma once

class RootSignature
{
public:
	void Init();

	ComPtr<ID3D12RootSignature>	GetGraphicsRootSignature()	{ return graphicsRootSignature; }
	ComPtr<ID3D12RootSignature>	GetComputeRootSignature()	{ return computeRootSignature; }

private:
	void CreateGraphicsRootSignature();
	void CreateComputeRootSignature();

private:
	D3D12_STATIC_SAMPLER_DESC	samplerDesc; 
	ComPtr<ID3D12RootSignature>	graphicsRootSignature;	
	ComPtr<ID3D12RootSignature>	computeRootSignature;
};

