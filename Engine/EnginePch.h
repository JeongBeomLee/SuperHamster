#pragma once

// std::byte 사용하지 않음
#define _HAS_STD_BYTE 0

// 각종 include
#include <WS2tcpip.h>
#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")

#include <windows.h>
#include <tchar.h>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <list>
#include <map>
#include <unordered_map>
#include <thread>
#include <fstream>
#include <numeric>
#include <filesystem>
#include <iostream>
using namespace std;
namespace fs = std::filesystem;

#include <DirectXPackedVector.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <wrl.h>
#include <fbxsdk.h>

#include "d3dx12.h"
#include "SimpleMath.h"
#include "DDSTextureLoader12.h"
#include "../DirectXTex/DirectXTex.h"
#include "../DirectXTex/DirectXTex.inl"
#include "../Server/protocol.h"

using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace Microsoft::WRL;

// 각종 lib
#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "dxguid")
#pragma comment(lib, "d3dcompiler")

#ifdef _DEBUG
	#pragma comment(lib, "../DirectXTex/DirectXTex_debug.lib")
#else
	#pragma comment(lib, "../DirectXTex/DirectXTex.lib")
#endif

#ifdef _DEBUG
	#pragma comment(lib, "../FBX/debug/libfbxsdk-md.lib")
	#pragma comment(lib, "../FBX/debug/libxml2-md.lib")
	#pragma comment(lib, "../FBX/debug/zlib-md.lib")
#else
	#pragma comment(lib, "../FBX/release/libfbxsdk-md.lib")
	#pragma comment(lib, "../FBX/release/libxml2-md.lib")
	#pragma comment(lib, "../FBX/release/zlib-md.lib")
#endif

// 각종 typedef
using int8		= __int8;
using int16		= __int16;
using int32		= __int32;
using int64		= __int64;
using uint8		= unsigned __int8;
using uint16	= unsigned __int16;
using uint32	= unsigned __int32;
using uint64	= unsigned __int64;
using Vec2		= DirectX::SimpleMath::Vector2;
using Vec3		= DirectX::SimpleMath::Vector3;
using Vec4		= DirectX::SimpleMath::Vector4;
using Matrix	= DirectX::SimpleMath::Matrix;

struct InputState {
	bool up;
	bool down;
	bool left;
	bool right;
	bool aim;
	bool fire;
	bool roll;
};

enum class PLAYER_STATE {
	CLIMB,
	FALLING,
	FALL_DOWN,
	FIRE,
	GETUP,
	IDLE,
	HIT,
	RUN_SLOW,
	RUN_FAST,
	ROLL,
	WALK,
	AIM,

	END,
};

enum class CBV_REGISTER : uint8 {
	b0,
	b1,
	b2,
	b3,
	b4,

	END
};

enum class SRV_REGISTER : uint8 {
	t0 = static_cast<uint8>(CBV_REGISTER::END),
	t1,
	t2,
	t3,
	t4,
	t5,
	t6,
	t7,
	t8,
	t9,

	END
};

enum class UAV_REGISTER : uint8 {
	u0 = static_cast<uint8>(SRV_REGISTER::END),
	u1,
	u2,
	u3,
	u4,

	END,
};

enum {
	SAMPLER_COUNT = 1,
	SWAP_CHAIN_BUFFER_COUNT = 2,
	CBV_REGISTER_COUNT = CBV_REGISTER::END,
	SRV_REGISTER_COUNT = static_cast<uint8>(SRV_REGISTER::END) - CBV_REGISTER_COUNT,
	CBV_SRV_REGISTER_COUNT = CBV_REGISTER_COUNT + SRV_REGISTER_COUNT,
	UAV_REGISTER_COUNT = static_cast<uint8>(UAV_REGISTER::END) - CBV_SRV_REGISTER_COUNT,
	TOTAL_REGISTER_COUNT = CBV_SRV_REGISTER_COUNT + UAV_REGISTER_COUNT
};

struct WindowInfo {
	HWND	hwnd; // 출력 윈도우
	int32	width; // 너비
	int32	height; // 높이
	bool	windowed; // 창모드 or 전체화면
};

struct Vertex {
	Vertex() {}

	Vertex(Vec3 p, Vec2 u, Vec3 n, Vec3 t)
		: pos(p), uv(u), normal(n), tangent(t)
	{
	}

	Vec3 pos;
	Vec2 uv;
	Vec3 normal;
	Vec3 tangent;
	Vec4 weights;
	Vec4 indices;
};

#define DECLARE_SINGLE(type)		\
private:							\
	type() {}						\
	~type() {}						\
public:								\
	static type* GetInstance()		\
	{								\
		static type instance;		\
		return &instance;			\
	}								\

#define GET_SINGLE(type)	type::GetInstance()

#define DEVICE				gEngine->GetDevice()->GetDevice()
#define GRAPHICS_CMD_LIST	gEngine->GetGraphicsCmdQueue()->GetGraphicsCmdList()
#define RESOURCE_CMD_LIST	gEngine->GetGraphicsCmdQueue()->GetResourceCmdList()
#define COMPUTE_CMD_LIST	gEngine->GetComputeCmdQueue()->GetComputeCmdList()

#define GRAPHICS_ROOT_SIGNATURE		gEngine->GetRootSignature()->GetGraphicsRootSignature()
#define COMPUTE_ROOT_SIGNATURE		gEngine->GetRootSignature()->GetComputeRootSignature()

#define INPUT				GET_SINGLE(Input)
#define DELTA_TIME			GET_SINGLE(Timer)->GetDeltaTime()

#define CONST_BUFFER(type)	gEngine->GetConstantBuffer(type)

struct TransformParams {
	Matrix matWorld;
	Matrix matView;
	Matrix matProjection;
	Matrix matWV;
	Matrix matWVP;
	Matrix matViewInv;
};

struct AnimFrameParams {
	Vec4	scale;
	Vec4	rotation; // Quaternion
	Vec4	translation;
};

extern unique_ptr<class Engine> gEngine;
extern unique_ptr<Vec3> cameraPos;
extern unique_ptr<Vec3> cameraRot;
extern SOCKET serverSocket;
extern int g_myid;
extern int g_otherid;

// Utils
wstring s2ws(const string& s);
string ws2s(const wstring& s);
Vec3 Slerp(Vec3& start, Vec3& end, float t);
float SineEaseInOut(float t);
void error_display(const char* msg, int err_no);
void send_packet(void* packet);
void process_data(char* net_buf, size_t io_byte);
void ProcessPacket(char* ptr);