#pragma once

// �η� �繫��
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
