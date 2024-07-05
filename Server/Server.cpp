#include <WS2tcpip.h>
#include <MSWSock.h>
#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")

#include <iostream>
#include <array>
#include <algorithm>
#include <chrono>
#include <locale>
#include <string>
#include <fstream>
#include <vector>
#include <memory>
#include "protocol.h"
#include "stdafx.h"
#include "PxPhysicsAPI.h"
#pragma comment(lib, "PhysX_64.lib")
#pragma comment(lib, "PhysXCommon_64.lib")
#pragma comment(lib, "PhysXFoundation_64.lib")
#pragma comment(lib, "PhysXExtensions_static_64.lib")
#pragma comment(lib, "PhysXPvdSDK_static_64.lib")
#pragma comment(lib, "PhysXCharacterKinematic_static_64.lib")
#pragma comment(lib, "PhysXCooking_64.lib")
using namespace physx;

PxDefaultAllocator		pxAllocator;
PxDefaultErrorCallback	errorCallback;
PxFoundation*			pxFoundation		= nullptr;
PxPhysics*				pxPhysics			= nullptr;
PxPvd*					pxPvd				= nullptr;	// 디버그용
PxPvdSceneClient*		pxPvdScene			= nullptr;	// 디버그용
PxDefaultCpuDispatcher* pxCpuDispatcher		= nullptr;
PxScene*				pxDefaultScene		= nullptr;
PxMaterial*				pxDefaultMaterial	= nullptr;
PxControllerManager*	pxControllerManager = nullptr;

void error_display(const char* msg, int err_no);
int  getNewClientId();
void disconnect(int clientID);
void process_packet(int clientID, char* packet);
void initPhysX();
void LoadMap(const std::wstring& _strFilePath);
enum CLIENT_STATE { ST_FREE, ST_INGAME };

enum PLAYER_STATE
{
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

class Timer
{
public:
	void Init()
	{
		::QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&_frequency));
		::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&_prevCount));
	}

	void Update()
	{
		uint64 currentCount;
		::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currentCount));

		_deltaTime = (currentCount - _prevCount) / static_cast<float>(_frequency);
		_prevCount = currentCount;

		_frameCount++;
		_frameTime += _deltaTime;

		if (_frameTime > 1.f) {
			_fps = static_cast<uint32>(_frameCount / _frameTime);

			_frameTime = 0.f;
			_frameCount = 0;
		}
	}

	uint32 GetFps() { return _fps; }
	float GetDeltaTime() { return _deltaTime; }

private:
	uint64	_frequency = 0;
	uint64	_prevCount = 0;
	float	_deltaTime = 0.f;

private:
	uint32	_frameCount = 0;
	float	_frameTime = 0.f;
	uint32	_fps = 0;
};
Timer g_Timer;

struct Vertex
{
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
struct BoneInfo
{
	std::wstring			boneName;
	int32					parentIdx;
	Matrix					matOffset;
};
class SESSION {
public:
	SESSION() 
	{
		state = ST_FREE;
		socket = 0;
		pos	= Vec3(0, 0, 0);
		dir	= Vec3(0, 0, 0);
		scale = Vec3(1, 1, 1);
		isActive = false;
		id = -1;
		prevRemain = 0;
		velocity = 0;
		acceleration = 0;
		weight = 100.f;
		rotationSpeed = 120.0f;
		//last_move_time = 0;
	}

	int doRecv() 
	{
		DWORD recv_flag = 0;
		int ret = recv(socket, sendBuf + prevRemain, BUF_SIZE - prevRemain, recv_flag);
		return ret;
	}

	void doSend(void* packet)
	{
		char* p = reinterpret_cast<char*>(packet);
		send(socket, p, p[0], 0);
	}

	void send_LoginInfoPacket(int clientID);

	void send_MovePlayerPacket(int clientID);

	void send_AddPlayerPacket(int clientID);

	void send_RemovePlayerPacket(int clientID)
	{
		SC_REMOVE_PLAYER_PACKET p;
		p.id = clientID;
		p.size = sizeof(p);
		p.type = SC_REMOVE_PLAYER;
		doSend(&p);
	}

public:
	char			sendBuf[BUF_SIZE];
	CLIENT_STATE	state;
	PLAYER_STATE	playerState;
	SOCKET			socket;
	Vec3 			pos, dir, scale;
	bool			isActive;
	int				id;
	int				prevRemain;
	float			velocity;
	float			acceleration;
	float			weight;
	float 			rotationSpeed;
	float			rollStartTime;
	float			fireStartTime;
	//int				last_move_time;
};

std::array<class SESSION, MAX_USER> players;
int main()
{
	std::wcout.imbue(std::locale("korean"));
	g_Timer.Init();
	initPhysX();
	LoadMap(L"Stage1.meshdata");

	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	SOCKET serverSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

	SOCKADDR_IN serverAddress;
	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(PORT);
	serverAddress.sin_addr.S_un.S_addr = INADDR_ANY;
	bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress));
	listen(serverSocket, SOMAXCONN);
	SOCKADDR_IN clientAddress;
	int addressSize = sizeof(clientAddress);

	unsigned long noblock = 1;
	ioctlsocket(serverSocket, FIONBIO, &noblock);

	while (true) {
		SOCKET client = WSAAccept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddress), &addressSize, NULL, NULL);
		//SOCKET client = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddress), &addressSize);
		if (client != INVALID_SOCKET) {
			int client_id = getNewClientId();
			if (client_id != -1) {
				players[client_id].state = ST_INGAME;
				players[client_id].isActive = true;
				players[client_id].id = client_id;
				players[client_id].prevRemain = 0;
				players[client_id].socket = client;
				players[client_id].doRecv();
				std::cout << "Client connected : " << client_id << std::endl;

				/*u_long on = 1;
				ioctlsocket(client, FIONBIO, &on);*/
			}
		}

		for (auto& pl : players) {
			if (pl.state != ST_INGAME) continue;

			int num_bytes = pl.doRecv();
			if (num_bytes <= 0) continue;

			int remain_data = num_bytes + pl.prevRemain;
			char* p = pl.sendBuf;
			while (remain_data > 0) {
				int packet_size = p[0];
				if (packet_size <= remain_data) {
					process_packet(pl.id, p);
					p = p + packet_size;
					remain_data = remain_data - packet_size;
				}
				else break;
			}
			pl.prevRemain = remain_data;
			if (remain_data > 0) {
				memcpy(pl.sendBuf, p, remain_data);
			}
		}

		g_Timer.Update();
		pxDefaultScene->simulate(1.f / 60.f);
		pxDefaultScene->fetchResults(true);
	}

	pxDefaultScene->release();
	pxCpuDispatcher->release();
	if (pxPvd) {
		PxPvdTransport* transport = pxPvd->getTransport();
		pxPvd->release();
		pxPvd = NULL;
		PX_RELEASE(transport);
	}
	pxPhysics->release();
	pxFoundation->release();
	pxDefaultMaterial->release();

	closesocket(serverSocket);
	WSACleanup();
}

void SESSION::send_LoginInfoPacket(int clientID)
{
	SC_LOGIN_INFO_PACKET p;
	p.id = players[clientID].id;
	p.size = sizeof(SC_LOGIN_INFO_PACKET);
	p.type = SC_LOGIN_INFO;
	p.pos = players[clientID].pos;
	p.dir = players[clientID].dir;
	p.scale = players[clientID].scale;
	doSend(&p);
}

void SESSION::send_MovePlayerPacket(int clientID)
{
	SC_MOVE_PLAYER_PACKET p;
	p.id = players[clientID].id;
	p.size = sizeof(SC_MOVE_PLAYER_PACKET);
	p.type = SC_MOVE_PLAYER;
	p.pos = players[clientID].pos;
	p.dir = players[clientID].dir;
	p.scale = players[clientID].scale;
	p.velocity = players[clientID].velocity;
	p.state = players[clientID].playerState;
	doSend(&p);
}

void SESSION::send_AddPlayerPacket(int clientID)
{
	SC_ADD_PLAYER_PACKET p;
	p.id = players[clientID].id;
	p.size = sizeof(SC_ADD_PLAYER_PACKET);
	p.type = SC_ADD_PLAYER;
	p.pos = players[clientID].pos;
	p.dir = players[clientID].dir;
	p.scale = players[clientID].scale;
	doSend(&p);
}

void error_display(const char* msg, int err_no)
{
	WCHAR* lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	std::cout << msg;
	std::wcout << L" 에러 " << lpMsgBuf << std::endl;
	while (true); // 디버깅 용
	LocalFree(lpMsgBuf);
}

int getNewClientId()
{
	for (int i = 0; i < MAX_USER; ++i)
		if (players[i].state == ST_FREE) return i;
	return -1;
}

void disconnect(int clientID)
{
	for (auto& pl : players) {
		if (ST_INGAME != pl.state) continue;
		if (pl.id == clientID) continue;
		pl.send_RemovePlayerPacket(clientID);
	}

	closesocket(players[clientID].socket);
	players[clientID].state = ST_FREE;
}

void process_packet(int clientID, char* packet)
{
	switch (packet[1]) {
		case CS_LOGIN: {
			CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
			players[clientID].pos = Vec3(1447.f, 256.f, -2099.f);
			players[clientID].dir = Vec3(-XM_PIDIV2, 3.2f, 0.f);
			players[clientID].scale = Vec3(200.f, 200.f, 200.f);

			PxCapsuleControllerDesc desc;
			desc.height = 50.0f;
			desc.radius = 25.0f;
			desc.climbingMode = PxCapsuleClimbingMode::eCONSTRAINED;
			desc.position = PxExtendedVec3(1447.f, 256.f, -2099.f);
			desc.material = pxDefaultMaterial;
			desc.contactOffset = 0.1f; // 땅과의 거리
			desc.stepOffset = 40.f; // 계단 높이
			desc.slopeLimit = cosf(PxDegToRad(45.f)); // 경사로
			desc.invisibleWallHeight = 0.0f; // 벽 높이
			desc.maxJumpHeight = 0.0f; // 점프 높이
			desc.reportCallback = nullptr; // PxUserControllerHitReport
			desc.behaviorCallback = nullptr; // PxControllerBehaviorCallback
			desc.nonWalkableMode = PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING;

			PxController* controller = pxControllerManager->createController(desc);
			controller->setUpDirection(PxVec3(0.f, 1.f, 0.f));

			players[clientID].send_LoginInfoPacket(clientID);
			std::cout << "Client " << clientID << " logged in" << std::endl;

			for (auto& other : players) {
				if (ST_INGAME != other.state) continue;
				if (other.id == clientID) continue;
				other.send_AddPlayerPacket(clientID);
				players[clientID].send_AddPlayerPacket(other.id);
			}
			break;
		}
		case CS_MOVE: {
			CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);

			// 클라이언트로부터 받은 이동 정보를 이용하여 캐릭터 컨트롤러 업데이트
			PxController* playerController = pxControllerManager->getController(clientID);
			PxVec3 disp(0.0f);
			Vec3 cameraRotation = Vec3(XMConvertToRadians(45.0f), XMConvertToRadians(-35.0f), 0.0f);

			// 카메라 회전을 고려하여 이동 방향 계산
			float cosX = cos(cameraRotation.x);
			float sinX = sin(cameraRotation.x);
			float cosY = cos(cameraRotation.y);
			float sinY = sin(cameraRotation.y);

			if (p->direction & 1) { // UP
				disp.x += sinY * cosX;
				disp.z += cosY * cosX;
			}

			if (p->direction & 2) { // DOWN
				disp.x -= sinY * cosX;
				disp.z -= cosY * cosX;
			}

			if (p->direction & 4) { // RIGHT
				disp.x += cosY;
				disp.z -= sinY;
			}

			if (p->direction & 8) { // LEFT
				disp.x -= cosY;
				disp.z += sinY;
			}

			if (p->direction & 16) { // A
				if (players[clientID].playerState != PLAYER_STATE::AIM && players[clientID].playerState != PLAYER_STATE::FIRE && players[clientID].playerState != PLAYER_STATE::ROLL) {
					players[clientID].playerState = PLAYER_STATE::AIM;
					players[clientID].velocity = 0.0f;
					players[clientID].acceleration = 0.0f;
				}
			}
			else {
				if (players[clientID].playerState == PLAYER_STATE::AIM)
					players[clientID].playerState = PLAYER_STATE::IDLE;
			}

			/*if (((players[clientID].playerState == PLAYER_STATE::AIM && p->direction & 32) || (players[clientID].playerState == PLAYER_STATE::FIRE && p->direction & 32)) && !(p->prevDirection & 32)) {
				players[clientID].playerState = PLAYER_STATE::FIRE;
			}
			else if (players[clientID].playerState == PLAYER_STATE::FIRE) {
				if (p->direction & 16)
					players[clientID].playerState = PLAYER_STATE::AIM;
				else
					players[clientID].playerState = PLAYER_STATE::IDLE;
			}*/

			if ((players[clientID].playerState == PLAYER_STATE::AIM && p->direction & 32) && !(p->prevDirection & 32)) {
				players[clientID].playerState = PLAYER_STATE::FIRE;
				players[clientID].fireStartTime = g_Timer.GetDeltaTime();
				players[clientID].velocity = 0.0f;
				players[clientID].acceleration = 0.0f;
			}
			else if (players[clientID].playerState == PLAYER_STATE::FIRE) {
				if (players[clientID].fireStartTime > 0.01f) {
					if (p->direction & 16)
						players[clientID].playerState = PLAYER_STATE::AIM;
					else
						players[clientID].playerState = PLAYER_STATE::IDLE;

					players[clientID].fireStartTime = 0.0f;
				}
				else
					players[clientID].fireStartTime += g_Timer.GetDeltaTime();

				players[clientID].velocity = 0.0f;
				players[clientID].acceleration = 0.0f;
			}

			if ((p->direction & 64) && !(p->prevDirection & 64) && players[clientID].playerState != PLAYER_STATE::AIM && players[clientID].playerState != PLAYER_STATE::FIRE && players[clientID].playerState != PLAYER_STATE::ROLL) {
				players[clientID].playerState = PLAYER_STATE::ROLL;
				players[clientID].rollStartTime = g_Timer.GetDeltaTime();
			}
			else if (players[clientID].playerState == PLAYER_STATE::ROLL) {
				players[clientID].acceleration = 1000.0f;
				players[clientID].velocity += players[clientID].acceleration * g_Timer.GetDeltaTime() * 50;

				// 구르기 지속 시간 확인
				if (players[clientID].rollStartTime > 0.012f) {
					players[clientID].playerState = PLAYER_STATE::IDLE;
					players[clientID].rollStartTime = 0.0f;
					players[clientID].velocity = 0.0f;
					players[clientID].acceleration = 0.0f;
				}
				else
					players[clientID].rollStartTime += g_Timer.GetDeltaTime();
			}

			if (false == disp.isZero()) {
				if (disp.magnitudeSquared() > 0.0f) {
					disp.normalize();

					if (players[clientID].playerState != PLAYER_STATE::AIM && players[clientID].playerState != PLAYER_STATE::FIRE && players[clientID].playerState != PLAYER_STATE::ROLL) {
						players[clientID].acceleration = disp.magnitude() * 1000.0f;
						players[clientID].velocity += players[clientID].acceleration * g_Timer.GetDeltaTime() * 50;
						players[clientID].velocity = std::clamp(players[clientID].velocity, 0.0f, 500.0f);

						if (players[clientID].velocity <= 0.0f)
							players[clientID].playerState = PLAYER_STATE::IDLE;
						else if (players[clientID].velocity > 0.0f && players[clientID].velocity <= 150.0f)
							players[clientID].playerState = PLAYER_STATE::WALK;
						else if (players[clientID].velocity > 150.0f && players[clientID].velocity <= 350.0f)
							players[clientID].playerState = PLAYER_STATE::RUN_SLOW;
						else if (players[clientID].velocity > 350.0f)
							players[clientID].playerState = PLAYER_STATE::RUN_FAST;
					}

					PxReal targetAngle = PxAtan2(-disp.x, -disp.z);
					Vec3 currentRotation = players[clientID].dir;
					PxReal currentAngle = currentRotation.y;

					PxReal deltaAngle = targetAngle - currentAngle;
					if (deltaAngle > PxPi)
						deltaAngle -= PxTwoPi;
					else if (deltaAngle < -PxPi)
						deltaAngle += PxTwoPi;

					PxReal interpolationFactor = players[clientID].rotationSpeed * g_Timer.GetDeltaTime();
					PxReal interpolatedAngle = currentAngle + deltaAngle * interpolationFactor;

					if (interpolatedAngle > PxPi)
						interpolatedAngle -= PxTwoPi;
					else if (interpolatedAngle < -PxPi)
						interpolatedAngle += PxTwoPi;

					Vec3 Rotation = players[clientID].dir;
					Rotation.y = interpolatedAngle;
					players[clientID].dir = Rotation;
				}
			}
			else {
				players[clientID].velocity = 0.0f;
				players[clientID].acceleration = 0.0f;
				if (players[clientID].playerState != PLAYER_STATE::AIM && players[clientID].playerState != PLAYER_STATE::FIRE && players[clientID].playerState != PLAYER_STATE::ROLL) {
					players[clientID].playerState = PLAYER_STATE::IDLE;
				}
			}

			disp *= players[clientID].velocity * g_Timer.GetDeltaTime() * 100.0f;
			disp.y -= 9.8f * g_Timer.GetDeltaTime() * players[clientID].weight * 100.0f;

			PxControllerFilters filters;
			playerController->move(disp, 0.001f, g_Timer.GetDeltaTime(), filters);

			Vec3 Position = players[clientID].pos;
			Position.x = playerController->getPosition().x;
			Position.y = playerController->getPosition().y - 50.f;
			Position.z = playerController->getPosition().z;

			players[clientID].pos = Position;
			players[clientID].send_MovePlayerPacket(clientID);
			for (auto& other : players) {
				if (ST_INGAME != other.state) continue;
				if (other.id == clientID) continue;
				other.send_MovePlayerPacket(clientID);
			}
		}
	}
}

void initPhysX()
{
	// PhysX Foundation 객체 생성
	pxFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, pxAllocator, errorCallback);

	// PhysX Physics 객체 생성
	pxPvd = PxCreatePvd(*pxFoundation);
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("localhost", 5425, 10);
	pxPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);
	pxPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *pxFoundation, PxTolerancesScale(), true, pxPvd);

	//pxPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *pxFoundation, PxTolerancesScale(), true);

	// PhysX Scene 생성
	PxSceneDesc sceneDesc(pxPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	pxCpuDispatcher = PxDefaultCpuDispatcherCreate(2);
	sceneDesc.cpuDispatcher = pxCpuDispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	pxDefaultScene = pxPhysics->createScene(sceneDesc);

	pxPvdScene = pxDefaultScene->getScenePvdClient();
	pxPvdScene->setScenePvdFlags(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS | PxPvdSceneFlag::eTRANSMIT_CONTACTS | PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES);

	pxDefaultMaterial = pxPhysics->createMaterial(0.5f, 0.5f, 0.6f);
	PxRigidStatic* groundPlane = PxCreatePlane(*pxPhysics, PxPlane(0, 1, 0, 0), *pxDefaultMaterial); // PxPlane(a, b, c, d) : ax + by + cz + d = 0

	// 바닥 생성
	pxDefaultScene->addActor(*groundPlane);

	pxControllerManager = PxCreateControllerManager(*pxDefaultScene);

	// PhysX Scene에서 충돌 정보 수신을 위한 콜백 함수 설정
	//pxScene->setSimulationEventCallback();

	std::cout << "PhysX Init Complete" << std::endl;
}

void LoadMap(const std::wstring& _strFilePath)
{
	std::ifstream in{ _strFilePath, std::ios::binary };
	if (!in.is_open()) {
		std::cout << "Failed to open file" << std::endl;
		return;
	}

	int meshCount = 0;
	in.read(reinterpret_cast<char*>(&meshCount), sizeof(meshCount)); // meshCount 읽기

	for (int i = 0; i < meshCount; ++i) {
		size_t nameLength = 0;

		in.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength)); // 이름 길이 읽기
		std::wstring meshName;
		meshName.resize(nameLength);
		in.read(reinterpret_cast<char*>(meshName.data()), nameLength * sizeof(wchar_t)); // 이름 읽기

		// Vertex 정보 읽기
		int vertexCount = 0;
		in.read(reinterpret_cast<char*>(&vertexCount), sizeof(vertexCount));
		std::vector<Vertex> vertices;
		vertices.resize(vertexCount);
		in.read(reinterpret_cast<char*>(vertices.data()), vertexCount * sizeof(Vertex));

		// Index 정보 읽기
		int indexCount = 0;
		in.read(reinterpret_cast<char*>(&indexCount), sizeof(indexCount));
		std::vector<std::vector<uint32_t>> indices;
		indices.resize(indexCount);
		for (int j = 0; j < indexCount; ++j) {
			int indexSize = 0;
			in.read(reinterpret_cast<char*>(&indexSize), sizeof(indexSize));
			indices[j].resize(indexSize);
			in.read(reinterpret_cast<char*>(indices[j].data()), indexSize * sizeof(uint32_t));
		}

		std::vector<PxVec3> physxVertices;
		physxVertices.reserve(vertices.size());

		std::vector<PxU32> physxIndices;
		physxIndices.reserve(indices[0].size());

		for (const auto& vertex : vertices) {
			physxVertices.emplace_back(vertex.pos.x, vertex.pos.y, vertex.pos.z);
		}

		for (const auto& index : indices[0]) {
			physxIndices.emplace_back(index);
		}

		// PxCookingParams 설정
		PxCookingParams params(pxPhysics->getTolerancesScale());
		params.meshPreprocessParams = PxMeshPreprocessingFlags(PxMeshPreprocessingFlag::eWELD_VERTICES);
		params.convexMeshCookingType = PxConvexMeshCookingType::eQUICKHULL;

		// PxTriangleMeshDesc 설정
		PxTriangleMeshDesc meshDesc;
		meshDesc.points.count = static_cast<PxU32>(physxVertices.size());
		meshDesc.points.stride = sizeof(PxVec3);
		meshDesc.points.data = physxVertices.data();
		meshDesc.triangles.count = static_cast<PxU32>(physxIndices.size() / 3);
		meshDesc.triangles.stride = 3 * sizeof(PxU32);
		meshDesc.triangles.data = physxIndices.data();

		// PxTriangleMeshDesc를 직렬화하기 위한 메모리 스트림 생성
		PxDefaultMemoryOutputStream writeBuffer;
		PxTriangleMeshCookingResult::Enum result;
		bool status = PxCookTriangleMesh(params, meshDesc, writeBuffer, &result);
		if (!status) {
			std::cerr << "Failed to cook triangle mesh." << std::endl;
		}

		if (writeBuffer.getSize() == 0) {
			std::cerr << "WriteBuffer is empty." << std::endl;
		}

		// 직렬화된 데이터를 PxInputStream으로 변환
		PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
		PxTriangleMesh* triangleMesh = pxPhysics->createTriangleMesh(readBuffer);

		PxTriangleMeshGeometry triangleMeshGeometry(triangleMesh, PxMeshScale(PxVec3(1, 1, 1)));
		PxRigidStatic* triangleMeshActor = pxPhysics->createRigidStatic(PxTransform(PxVec3(0, 0, 0)));
		PxShape* triangleMeshShape = pxPhysics->createShape(triangleMeshGeometry, *pxDefaultMaterial);

		// x축 기준 -90도 회전
		PxQuat quat(-XM_PIDIV2, PxVec3(1, 0, 0));
		triangleMeshActor->setGlobalPose(PxTransform(PxVec3(0, 0, 0), quat));

		triangleMeshActor->attachShape(*triangleMeshShape);
		pxDefaultScene->addActor(*triangleMeshActor);

		triangleMeshShape->release();

		// Animation 정보 읽기
		bool hasAnimation = false;
		in.read(reinterpret_cast<char*>(&hasAnimation), sizeof(hasAnimation));
		if (hasAnimation) {
			// Animation Clip 정보 읽기
			int animClipCount = 0;
			in.read(reinterpret_cast<char*>(&animClipCount), sizeof(animClipCount));

			for (int j = 0; j < animClipCount; ++j) {
				// 이름, 길이, 프레임 수 읽기
				size_t nameLength = 0;
				std::wstring animName;
				double duration = 0.0;
				int frameCount = 0;

				in.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
				animName.resize(nameLength);
				in.read(reinterpret_cast<char*>(animName.data()), nameLength * sizeof(wchar_t));
				in.read(reinterpret_cast<char*>(&duration), sizeof(duration));
				in.read(reinterpret_cast<char*>(&frameCount), sizeof(frameCount));

				// KeyFrame Count 읽기
				int keyFrameCount = 0;
				in.read(reinterpret_cast<char*>(&keyFrameCount), sizeof(keyFrameCount));

				// bone 정보 읽기
				int boneCount = 0;
				in.read(reinterpret_cast<char*>(&boneCount), sizeof(boneCount));
				for (int b = 0; b < boneCount; ++b) {
					int keyFrameInfoCount = 0;
					in.read(reinterpret_cast<char*>(&keyFrameInfoCount), sizeof(keyFrameInfoCount));

					for (int f = 0; f < keyFrameInfoCount; ++f) {
						double time;
						int32 frame;
						double sx, sy, sz;
						double qx, qy, qz, qw;
						double tx, ty, tz;

						in.read(reinterpret_cast<char*>(&time), sizeof(double));
						in.read(reinterpret_cast<char*>(&frame), sizeof(int32));

						in.read(reinterpret_cast<char*>(&sx), sizeof(sx));
						in.read(reinterpret_cast<char*>(&sy), sizeof(sy));
						in.read(reinterpret_cast<char*>(&sz), sizeof(sz));

						in.read(reinterpret_cast<char*>(&qx), sizeof(qx));
						in.read(reinterpret_cast<char*>(&qy), sizeof(qy));
						in.read(reinterpret_cast<char*>(&qz), sizeof(qz));
						in.read(reinterpret_cast<char*>(&qw), sizeof(qw));

						in.read(reinterpret_cast<char*>(&tx), sizeof(tx));
						in.read(reinterpret_cast<char*>(&ty), sizeof(ty));
						in.read(reinterpret_cast<char*>(&tz), sizeof(tz));
					}
				}
			}

			// Bones 정보 읽기
			int boneCount = 0;
			in.read(reinterpret_cast<char*>(&boneCount), sizeof(boneCount));
			for (int j = 0; j < boneCount; ++j) {
				size_t nameLength = 0;
				BoneInfo bone = {};

				in.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
				bone.boneName.resize(nameLength);
				in.read(reinterpret_cast<char*>(bone.boneName.data()), nameLength * sizeof(wchar_t));
				in.read(reinterpret_cast<char*>(&bone.parentIdx), sizeof(bone.parentIdx));
				for (int y = 0; y < 4; y++) {
					for (int x = 0; x < 4; x++) {
						double temp = 0.0;
						in.read(reinterpret_cast<char*>(&temp), sizeof(temp));
						bone.matOffset.m[y][x] = static_cast<float>(temp); // matOffset 기록
					}
				}
			}
		}
	}
}
