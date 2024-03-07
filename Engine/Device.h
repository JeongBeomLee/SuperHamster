#pragma once

// �η� �繫��
class Device
{
public:
	void Init();

	ComPtr<IDXGIFactory> GetDXGI()		{ return dxgi; }
	ComPtr<ID3D12Device> GetDevice()	{ return device; }

private:
	ComPtr<ID3D12Debug>		debugController;
	ComPtr<IDXGIFactory>	dxgi;
	ComPtr<ID3D12Device>	device;
};
