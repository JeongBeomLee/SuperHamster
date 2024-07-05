#include <d3d12.h>
#include <DirectXMath.h>
#include "../Engine/d3dx12.h"
#include "../Engine/SimpleMath.h"
#include "../PhysX/include/PxPhysicsAPI.h"

#ifdef _DEBUG
	#pragma comment(lib, "../PhysX/bin/debug/PhysX_64.lib")
	#pragma comment(lib, "../PhysX/bin/debug/PhysXCommon_64.lib")
	#pragma comment(lib, "../PhysX/bin/debug/PhysXTask_static_64")
	#pragma comment(lib, "../PhysX/bin/debug/PhysXCooking_64.lib")
	#pragma comment(lib, "../PhysX/bin/debug/PhysXVehicle_static_64")
	#pragma comment(lib, "../PhysX/bin/debug/PhysXFoundation_64.lib")
	#pragma comment(lib, "../PhysX/bin/debug/PhysXVehicle2_static_64")
	#pragma comment(lib, "../PhysX/bin/debug/PhysXPvdSDK_static_64.lib")
	#pragma comment(lib, "../PhysX/bin/debug/PhysXExtensions_static_64.lib")
	#pragma comment(lib, "../PhysX/bin/debug/PhysXCharacterKinematic_static_64.lib")
	
	#pragma comment(lib, "../PhysX/bin/debug/PVDRuntime_64")
	#pragma comment(lib, "../PhysX/bin/debug/LowLevel_static_64")
	#pragma comment(lib, "../PhysX/bin/debug/SceneQuery_static_64")
	#pragma comment(lib, "../PhysX/bin/debug/LowLevelAABB_static_64")
	#pragma comment(lib, "../PhysX/bin/debug/LowLevelDynamics_static_64")
	#pragma comment(lib, "../PhysX/bin/debug/SimulationController_static_64")
#else
	#pragma comment(lib, "../PhysX/bin/release/PhysX_64.lib")
	#pragma comment(lib, "../PhysX/bin/release/PhysXCommon_64.lib")
	#pragma comment(lib, "../PhysX/bin/release/PhysXTask_static_64")
	#pragma comment(lib, "../PhysX/bin/release/PhysXCooking_64.lib")
	#pragma comment(lib, "../PhysX/bin/release/PhysXVehicle_static_64")
	#pragma comment(lib, "../PhysX/bin/release/PhysXFoundation_64.lib")
	#pragma comment(lib, "../PhysX/bin/release/PhysXVehicle2_static_64")
	#pragma comment(lib, "../PhysX/bin/release/PhysXPvdSDK_static_64.lib")
	#pragma comment(lib, "../PhysX/bin/release/PhysXExtensions_static_64.lib")
	#pragma comment(lib, "../PhysX/bin/release/PhysXCharacterKinematic_static_64.lib")
	
	#pragma comment(lib, "../PhysX/bin/release/PVDRuntime_64")
	#pragma comment(lib, "../PhysX/bin/release/LowLevel_static_64")
	#pragma comment(lib, "../PhysX/bin/release/SceneQuery_static_64")
	#pragma comment(lib, "../PhysX/bin/release/LowLevelAABB_static_64")
	#pragma comment(lib, "../PhysX/bin/release/LowLevelDynamics_static_64")
	#pragma comment(lib, "../PhysX/bin/release/SimulationController_static_64")
#endif
using namespace physx;

using namespace DirectX;

using int8 = __int8;
using int16 = __int16;
using int32 = __int32;
using int64 = __int64;
using uint8 = unsigned __int8;
using uint16 = unsigned __int16;
using uint32 = unsigned __int32;
using uint64 = unsigned __int64;
using Vec2 = DirectX::SimpleMath::Vector2;
using Vec3 = DirectX::SimpleMath::Vector3;
using Vec4 = DirectX::SimpleMath::Vector4;
using Matrix = DirectX::SimpleMath::Matrix;