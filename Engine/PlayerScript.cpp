#include "pch.h"
#include "PlayerScript.h"
#include "Input.h"
#include "Animator.h"
#include "Transform.h"
#include "Timer.h"
#include "Engine.h"
#include "SceneManager.h"
#include "Scene.h"
#include "GameObject.h"
#include "Resources.h"
#include "MeshData.h"

PlayerScript::PlayerScript()
	: m_PrevDirection(0)
    , m_PrevState(PLAYER_STATE::CLIMB)
	, m_CurrentState(PLAYER_STATE::IDLE)
    , m_Velocity(0.0f)
{
    GET_SINGLE(Resources)->LoadFBX(L"..\\Resources\\FBX\\Gun 02.fbx");
    GET_SINGLE(Resources)->LoadFBX(L"..\\Resources\\FBX\\Gun 07.fbx");
}

void PlayerScript::UpdateGun(PLAYER_GUN gun)
{
    shared_ptr<Scene> scene = GET_SINGLE(SceneManager)->GetActiveScene();

    if (m_Gun != gun) {
		switch (m_Gun) {
		case PLAYER_GUN::DEFAULT:
			scene->RemoveGameObject(scene->GetGameObjectByName(L"Hamster" + to_wstring(g_myid) + L"_DefaultGun"));
			break;

		case PLAYER_GUN::LASER:
			scene->RemoveGameObject(scene->GetGameObjectByName(L"Hamster" + to_wstring(g_myid) + L"_LaserGun"));
			break;

		case PLAYER_GUN::MAGNETIC:
			scene->RemoveGameObject(scene->GetGameObjectByName(L"Hamster" + to_wstring(g_myid) + L"_MagneticGun"));
			break;

		default:
			break;
		}

        switch (gun) {
        case PLAYER_GUN::DEFAULT: {
            shared_ptr<MeshData> gunMeshData = GET_SINGLE(Resources)->LoadFBX(L"..\\Resources\\FBX\\Gun 01.fbx");
            vector<shared_ptr<GameObject>> gunObjects = gunMeshData->Instantiate();
            for (auto& object : gunObjects) {
                object->SetName(L"Hamster" + to_wstring(g_myid) + L"_DefaultGun");
                object->SetCheckFrustum(false);
                object->SetStatic(false);
                object->GetTransform()->SetLocalPosition(Vec3(0.f, 0.f, 0.f));
                object->GetTransform()->SetLocalScale(Vec3(0.55f, 0.55f, 0.55f));
                object->GetTransform()->SetLocalRotation(Vec3(XMConvertToRadians(180.f), XMConvertToRadians(-20.f), XMConvertToRadians(90.f)));
                object->AttachToBone(scene->GetGameObjectByName(L"Hamster" + to_wstring(g_myid)), L"mixamorig:RightHand");

                scene->AddGameObject(object);
            }
        }
            break;

        case PLAYER_GUN::LASER: {
            shared_ptr<MeshData> gunMeshData = GET_SINGLE(Resources)->LoadFBX(L"..\\Resources\\FBX\\Gun 02.fbx");
            vector<shared_ptr<GameObject>> gunObjects = gunMeshData->Instantiate();
            for (auto& object : gunObjects) {
                object->SetName(L"Hamster" + to_wstring(g_myid) + L"_LaserGun");
                object->SetCheckFrustum(false);
                object->SetStatic(false);
                object->GetTransform()->SetLocalPosition(Vec3(0.f, 0.f, 0.f));
                object->GetTransform()->SetLocalScale(Vec3(0.65f, 0.65f, 0.65f));
                object->GetTransform()->SetLocalRotation(Vec3(XMConvertToRadians(180.f), XMConvertToRadians(-20.f), XMConvertToRadians(90.f)));
                object->AttachToBone(scene->GetGameObjectByName(L"Hamster" + to_wstring(g_myid)), L"mixamorig:RightHand");

                scene->AddGameObject(object);
            }
        }
            break;

        case PLAYER_GUN::MAGNETIC: {
            shared_ptr<MeshData> gunMeshData = GET_SINGLE(Resources)->LoadFBX(L"..\\Resources\\FBX\\Gun 07.fbx");
            vector<shared_ptr<GameObject>> gunObjects = gunMeshData->Instantiate();
            for (auto& object : gunObjects) {
                object->SetName(L"Hamster" + to_wstring(g_myid) + L"_MagneticGun");
                object->SetCheckFrustum(false);
                object->SetStatic(false);
                object->GetTransform()->SetLocalPosition(Vec3(0.f, 0.f, 0.f));
                object->GetTransform()->SetLocalScale(Vec3(0.75f, 0.75f, 0.75f));
                object->GetTransform()->SetLocalRotation(Vec3(XMConvertToRadians(180.f), XMConvertToRadians(-20.f), XMConvertToRadians(90.f)));
                object->AttachToBone(scene->GetGameObjectByName(L"Hamster" + to_wstring(g_myid)), L"mixamorig:RightHand");

                scene->AddGameObject(object);
            }
        }
            break;

        default:
            break;
        }

        m_Gun = gun;
    }
}

void PlayerScript::Update()
{
    BYTE inputDirection = 0;

    if (INPUT->GetButton(KEY_TYPE::UP)) {
        inputDirection |= 1;
    }
    if (INPUT->GetButton(KEY_TYPE::DOWN)) {
        inputDirection |= 2;
    }
    if (INPUT->GetButton(KEY_TYPE::RIGHT)) {
        inputDirection |= 4;
    }
    if (INPUT->GetButton(KEY_TYPE::LEFT)) {
        inputDirection |= 8;
    }
    if (INPUT->GetButton(KEY_TYPE::A)) {
        inputDirection |= 16;
    }
    if (INPUT->GetButtonDown(KEY_TYPE::S)) {
        CS_SHOOT_PACKET packet;
        packet.size = sizeof(CS_SHOOT_PACKET);
        packet.type = CS_SHOOT;
        send_packet(&packet);
        inputDirection |= 32;
    }
    if (INPUT->GetButtonDown(KEY_TYPE::SPACE)) {
        inputDirection |= 64;
    }
    if (INPUT->GetButtonDown(KEY_TYPE::KEY_1)) {
		UpdateGun(PLAYER_GUN::DEFAULT);
	}
    if (INPUT->GetButtonDown(KEY_TYPE::KEY_2)) {
        UpdateGun(PLAYER_GUN::LASER);
	}
    if (INPUT->GetButtonDown(KEY_TYPE::KEY_3)) {
        UpdateGun(PLAYER_GUN::MAGNETIC);
	}

    CS_MOVE_PACKET movePacket;
    movePacket.size = sizeof(movePacket);
    movePacket.type = CS_MOVE;
    movePacket.direction = inputDirection;
    movePacket.prevDirection = m_PrevDirection;

    // 서버로 이동 패킷 전송
    send_packet(&movePacket);

    m_PrevDirection = inputDirection;

    // 상태에 따른 애니메이션 변경
    if (m_PrevState != m_CurrentState) {
        shared_ptr<Animator> animator = GetAnimator();
        if (animator) {
            animator->Play(m_CurrentState);
        }
        m_PrevState = m_CurrentState;
    }
}


PlayerScript2::PlayerScript2()
    : m_PrevDirection(0)
    , m_PrevState(PLAYER_STATE::CLIMB)
    , m_CurrentState(PLAYER_STATE::IDLE)
    , m_Velocity(0.0f)
{
}

void PlayerScript2::Update()
{
    // 상태에 따른 애니메이션 변경
    if (m_PrevState != m_CurrentState) {
        shared_ptr<Animator> animator = GetAnimator();
        if (animator) {
            animator->Play(m_CurrentState);
        }
        m_PrevState = m_CurrentState;
    }
}