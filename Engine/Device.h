#pragma once

// 인력 사무소
class Device
{
public:
	void Init();

	ComPtr<IDXGIFactory> GetDXGI()		{ return _dxgi; }
	ComPtr<ID3D12Device> GetDevice()	{ return _device; }

private:
	ComPtr<ID3D12Debug>		_debugController;
	ComPtr<IDXGIFactory>	_dxgi;
	ComPtr<ID3D12Device>	_device;
};
