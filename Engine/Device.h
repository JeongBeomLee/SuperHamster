#pragma once

// 인력 사무소
class Device
{
public:
	void Init();

	ComPtr<IDXGIFactory> GetDXGI()		{ return dxgiFactory; }
	ComPtr<ID3D12Device> GetDevice()	{ return device; }

private:
	ComPtr<ID3D12Debug>		debugController;
	ComPtr<IDXGIFactory>	dxgiFactory;
	ComPtr<ID3D12Device>	device;
};
