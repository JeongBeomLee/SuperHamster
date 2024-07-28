#include "pch.h"
#include "EnginePch.h"
#include "Engine.h"

#include "SceneManager.h"
#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"
#include "Resources.h"
#include "MeshData.h"
#include "PlayerScript.h"
#include "MeshRenderer.h"
#include "Mesh.h"
#include "Animator.h"
#include "ParticleSystem.h"

unique_ptr<Engine> gEngine = make_unique<Engine>();
unique_ptr<Vec3> cameraPos = make_unique<Vec3>();
unique_ptr<Vec3> cameraRot = make_unique<Vec3>();
SOCKET serverSocket = INVALID_SOCKET;
int g_myid = -1;
int g_otherid = -1;

wstring s2ws(const string& s)
{
	int32 len;
	int32 slength = static_cast<int32>(s.length()) + 1;
	len = ::MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	::MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	wstring ret(buf);
	delete[] buf;
	return ret;
}

string ws2s(const wstring& s)
{
	int32 len;
	int32 slength = static_cast<int32>(s.length()) + 1;
	len = ::WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, 0, 0, 0, 0);
	string r(len, '\0');
	::WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, &r[0], len, 0, 0);
	return r;
}

Vec3 Slerp(Vec3& start, Vec3& end, float t)
{
    // Dot product - 두 벡터 사이의 각도 계산
    float dot = start.Dot(end);

    // 역방향인 경우 처리
    if (dot < 0.0f) {
        dot = -dot;
        end = -end;
    }

    // 보간 계수 조정
    if (dot > 0.9995f) {
        return Vec3::Lerp(start, end, t);
    }

    // 각도와 sin 값 계산
    float theta = acosf(dot);
    float sinTheta = sinf(theta);

    // 보간 계수 계산
    float startWeight = sinf((1.0f - t) * theta) / sinTheta;
    float endWeight = sinf(t * theta) / sinTheta;

    // 선형 보간 결과 반환
    return start * startWeight + end * endWeight;
}

float SineEaseInOut(float t)
{
    return 0.5f * (1.0f - cos(3.141592f * t));
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

void send_packet(void* packet)
{
	unsigned char* p = reinterpret_cast<unsigned char*>(packet);
	int retval = send(serverSocket, reinterpret_cast<char*>(p), p[0], 0);
	if (retval == SOCKET_ERROR) {
		if (WSAGetLastError() != WSAEWOULDBLOCK) {
			error_display("send()", WSAGetLastError());
			return;
		}
	}
}

void process_data(char* net_buf, size_t io_byte)
{
    char* ptr = net_buf;
    static size_t in_packet_size = 0;
    static size_t saved_packet_size = 0;
    static char packet_buffer[BUF_SIZE];

    while (0 != io_byte) {
        if (0 == in_packet_size) in_packet_size = ptr[0];
        if (io_byte + saved_packet_size >= in_packet_size) {
            memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
            ProcessPacket(packet_buffer);
            ptr += in_packet_size - saved_packet_size;
            io_byte -= in_packet_size - saved_packet_size;
            in_packet_size = 0;
            saved_packet_size = 0;
        }
        else {
            memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
            saved_packet_size += io_byte;
            io_byte = 0;
        }
    }
}

void ProcessPacket(char* ptr)
{
    static bool first_time = true;
    switch (ptr[1]) {
    case SC_LOGIN_INFO: {
		SC_LOGIN_INFO_PACKET* packet = reinterpret_cast<SC_LOGIN_INFO_PACKET*>(ptr);
		g_myid = packet->id;

		Scene* scene = GET_SINGLE(SceneManager)->GetActiveScene().get();
		shared_ptr<MeshData> hamsterMeshData = GET_SINGLE(Resources)->Load<MeshData>(
            L"Hamster" + to_wstring(g_myid) + L"_MeshData",
            L"..\\Resources\\FBX\\Hamster" + to_wstring(g_myid) + L".meshdata");
		vector<shared_ptr<GameObject>> hamsterObjects = hamsterMeshData->Instantiate();
		for (auto& object : hamsterObjects) {
            object->SetName(L"Hamster" + to_wstring(g_myid));
            object->SetCheckFrustum(false);
            object->GetTransform()->SetLocalPosition(packet->pos);
            object->GetTransform()->SetLocalScale(packet->scale);
            object->GetTransform()->SetLocalRotation(packet->dir);
            object->SetStatic(false);

			scene->AddGameObject(object);
            object->AddComponent(make_shared<PlayerScript>());
		}

        shared_ptr<MeshData> gunMeshData = GET_SINGLE(Resources)->LoadFBX(L"..\\Resources\\FBX\\Gun 01.fbx");
        vector<shared_ptr<GameObject>> gunObjects = gunMeshData->Instantiate();
        for (auto& object : gunObjects) {
            object->SetName(L"Hamster" + to_wstring(g_myid) + L"_DefaultGun");
            object->SetCheckFrustum(false);
            object->SetStatic(false);
            object->GetTransform()->SetLocalPosition(Vec3(0.f, 0.f, 0.f));
            object->GetTransform()->SetLocalScale(Vec3(0.5f, 0.5f, 0.5f));
            object->GetTransform()->SetLocalRotation(Vec3(XMConvertToRadians(180.f), XMConvertToRadians(-20.f), XMConvertToRadians(90.f)));
            object->AttachToBone(scene->GetGameObjectByName(L"Hamster" + to_wstring(g_myid)), L"mixamorig:RightHand");

            scene->AddGameObject(object);
        }
    }
    break;

    case SC_ADD_PLAYER: {
        SC_ADD_PLAYER_PACKET* packet = reinterpret_cast<SC_ADD_PLAYER_PACKET*>(ptr);
        int id = packet->id;

        if (id != g_myid) {
            Scene* scene = GET_SINGLE(SceneManager)->GetActiveScene().get();

            shared_ptr<MeshData> hamsterMeshData = GET_SINGLE(Resources)->Load<MeshData>(
                L"Hamster" + to_wstring(id) + L"_MeshData",
                L"..\\Resources\\FBX\\Hamster" + to_wstring(id) + L".meshdata");
            vector<shared_ptr<GameObject>> gameObjects = hamsterMeshData->Instantiate();
            for (auto& gameObject : gameObjects) {
                gameObject->SetName(L"Hamster" + to_wstring(id));
                gameObject->SetCheckFrustum(false);
                gameObject->GetTransform()->SetLocalPosition(packet->pos);
                gameObject->GetTransform()->SetLocalScale(packet->scale);
                gameObject->GetTransform()->SetLocalRotation(packet->dir);
                gameObject->SetStatic(false);

                scene->AddGameObject(gameObject);
                gameObject->AddComponent(make_shared<PlayerScript2>());
            }

            shared_ptr<MeshData> gunMeshData = GET_SINGLE(Resources)->LoadFBX(L"..\\Resources\\FBX\\Gun 02.fbx");
            vector<shared_ptr<GameObject>> gunObjects = gunMeshData->Instantiate();
            for (auto& object : gunObjects) {
                object->SetName(L"Hamster" + to_wstring(id) + L"_Gun2");
                object->SetCheckFrustum(false);
                object->SetStatic(false);
                object->GetTransform()->SetLocalPosition(Vec3(0.f, 0.f, 0.f));
                object->GetTransform()->SetLocalScale(Vec3(0.5f, 0.5f, 0.5f));
                object->GetTransform()->SetLocalRotation(Vec3(XMConvertToRadians(180.f), XMConvertToRadians(-20.f), XMConvertToRadians(90.f)));
                object->AttachToBone(scene->GetGameObjectByName(L"Hamster" + to_wstring(id)), L"mixamorig:RightHand");

                scene->AddGameObject(object);
            }
        }
        break;
    }
    case SC_MOVE_PLAYER: {
        SC_MOVE_PLAYER_PACKET* my_packet = reinterpret_cast<SC_MOVE_PLAYER_PACKET*>(ptr);
        int id = my_packet->id;

        GameObject* gameObject = GET_SINGLE(SceneManager)->GetActiveScene()->GetGameObjectByName(L"Hamster" + to_wstring(id)).get();
        if (gameObject == nullptr) 
            return;

        gameObject->GetTransform()->SetLocalPosition(my_packet->pos);
        gameObject->GetTransform()->SetLocalRotation(my_packet->dir);
        gameObject->GetTransform()->SetLocalScale(my_packet->scale);

        if (id == g_myid) {
            PlayerScript* playerScript = reinterpret_cast<PlayerScript*>(gameObject->GetScript().get());
            playerScript->SetState(static_cast<PLAYER_STATE>(my_packet->state));
        }
        else {
            PlayerScript2* playerScript = reinterpret_cast<PlayerScript2*>(gameObject->GetScript().get());
            playerScript->SetState(static_cast<PLAYER_STATE>(my_packet->state));
        }
        break;
    }

    case SC_REMOVE_PLAYER: {
        SC_REMOVE_PLAYER_PACKET* my_packet = reinterpret_cast<SC_REMOVE_PLAYER_PACKET*>(ptr);
        int other_id = my_packet->id;

        Scene* scene = GET_SINGLE(SceneManager)->GetActiveScene().get();
        scene->RemoveGameObject(scene->GetGameObjectByName(L"Hamster" + to_wstring(other_id)));
        break;
    }

    case SC_ADD_BULLET:
    {
        SC_ADD_BULLET_PACKET* packet = reinterpret_cast<SC_ADD_BULLET_PACKET*>(ptr);
        // 총알 게임 오브젝트 생성 및 초기화
        shared_ptr<GameObject> bullet = make_shared<GameObject>();
        bullet->SetName(L"Bullet_" + to_wstring(packet->bulletId));
        bullet->SetCheckFrustum(false);
        bullet->SetStatic(false);
        bullet->AddComponent(make_shared<Transform>());
        bullet->AddComponent(make_shared<ParticleSystem>());
        bullet->GetTransform()->SetLocalPosition(packet->position);
        bullet->GetTransform()->SetLocalScale(Vec3(100.f, 100.f, 100.f));

        /*shared_ptr<MeshRenderer> meshRenderer = make_shared<MeshRenderer>();
        meshRenderer->SetMesh(GET_SINGLE(Resources)->LoadSphereMesh());
        meshRenderer->SetMaterial(GET_SINGLE(Resources)->Get<Material>(L"Bullet"));
        bullet->AddComponent(meshRenderer);*/

        GET_SINGLE(SceneManager)->GetActiveScene()->AddGameObject(bullet);
        break;
    }

    case SC_MOVE_BULLET:
    {
        SC_MOVE_BULLET_PACKET* packet = reinterpret_cast<SC_MOVE_BULLET_PACKET*>(ptr);
        auto bullet = GET_SINGLE(SceneManager)->GetActiveScene()->GetGameObjectByName(L"Bullet_" + to_wstring(packet->bulletId));
        if (bullet) {
            Vec3 currentPos = bullet->GetTransform()->GetLocalPosition();
            Vec3 targetPos = packet->position;
            Vec3 newPos = Vec3::Lerp(currentPos, targetPos, 0.5f);  // 보간된 위치 계산
            bullet->GetTransform()->SetLocalPosition(newPos);
        }
        break;
    }

    case SC_REMOVE_BULLET:
    {
        SC_REMOVE_BULLET_PACKET* packet = reinterpret_cast<SC_REMOVE_BULLET_PACKET*>(ptr);
        auto bullet = GET_SINGLE(SceneManager)->GetActiveScene()->GetGameObjectByName(L"Bullet_" + to_wstring(packet->bulletId));
        if (bullet)
        {
            GET_SINGLE(SceneManager)->GetActiveScene()->RemoveGameObject(bullet);
        }
        break;
    }

    default:
        printf("Unknown PACKET type [%d]\n", ptr[1]);
    }
}

void send_login_packet()
{
    CS_LOGIN_PACKET p;
    p.size = sizeof(p);
    p.type = CS_LOGIN; 
    send_packet(&p);
    cout << "send login packet" << endl;
}

void send_logout_packet()
{
    CS_LOGOUT_PACKET p;
    p.size = sizeof(p);
    p.type = CS_LOGOUT;
    send_packet(&p);
    cout << "send logout packet" << endl;
}
