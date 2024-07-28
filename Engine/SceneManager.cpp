#include "pch.h"
#include "SceneManager.h"
#include "Scene.h"

#include "Engine.h"
#include "Material.h"
#include "GameObject.h"
#include "MeshRenderer.h"
#include "Transform.h"
#include "Camera.h"
#include "Light.h"

#include "TestCameraScript.h"
#include "Resources.h"
#include "ParticleSystem.h"
#include "Terrain.h"
#include "SphereCollider.h"
#include "MeshData.h"
#include "PlayerScript.h"
#include "Timer.h"
#include "TestMap.h"
#include "Mesh.h"
#include "FBXLoader.h"
#include "TestAnimation.h"
#include "WaterScript.h"

void SceneManager::Update()
{
	if (activeScene == nullptr)
		return;

	activeScene->Update();
	activeScene->LateUpdate();
	activeScene->FinalUpdate();
}

// TEMP
void SceneManager::Render()
{
	if (activeScene)
		activeScene->Render();
}

void SceneManager::LoadScene(wstring sceneName)
{
	// TODO : 기존 Scene 정리
	// TODO : 파일에서 Scene 정보 로드

	activeScene = LoadTestScene();

	activeScene->Awake();
	activeScene->Start();
}

void SceneManager::SetLayerName(uint8 index, const wstring& name)
{
	// 기존 데이터 삭제
	const wstring& prevName = layerNames[index];
	layerIndex.erase(prevName);

	layerNames[index] = name;
	layerIndex[name] = index;
}

uint8 SceneManager::LayerNameToIndex(const wstring& name)
{
	auto findIt = layerIndex.find(name);
	if (findIt == layerIndex.end())
		return 0;

	return findIt->second;
}

shared_ptr<GameObject> SceneManager::Pick(int32 screenX, int32 screenY)
{
	shared_ptr<Camera> camera = GetActiveScene()->GetMainCamera();

	float width = static_cast<float>(gEngine->GetWindow().width);
	float height = static_cast<float>(gEngine->GetWindow().height);

	Matrix projectionMatrix = camera->GetProjectionMatrix();

	// ViewSpace에서 Picking 진행
	float viewX = (+2.0f * screenX / width - 1.0f) / projectionMatrix(0, 0);
	float viewY = (-2.0f * screenY / height + 1.0f) / projectionMatrix(1, 1);

	Matrix viewMatrix = camera->GetViewMatrix();
	Matrix viewMatrixInv = viewMatrix.Invert();

	auto& gameObjects = GET_SINGLE(SceneManager)->GetActiveScene()->GetGameObjects();

	float minDistance = FLT_MAX;
	shared_ptr<GameObject> picked;

	for (auto& gameObject : gameObjects) {
		if (gameObject->GetCollider() == nullptr)
			continue;

		// ViewSpace에서의 Ray 정의
		Vec4 rayOrigin = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
		Vec4 rayDir = Vec4(viewX, viewY, 1.0f, 0.0f);

		// WorldSpace에서의 Ray 정의
		rayOrigin = XMVector3TransformCoord(rayOrigin, viewMatrixInv);
		rayDir = XMVector3TransformNormal(rayDir, viewMatrixInv);
		rayDir.Normalize();

		// WorldSpace에서 연산
		float distance = 0.f;
		if (gameObject->GetCollider()->Intersects(rayOrigin, rayDir, OUT distance) == false)
			continue;

		if (distance < minDistance) {
			minDistance = distance;
			picked = gameObject;
		}
	}

	return picked;
}

shared_ptr<Scene> SceneManager::LoadTestScene()
{
#pragma region LayerMask
	SetLayerName(0, L"Default");
	SetLayerName(1, L"UI");
#pragma endregion

#pragma region ComputeShader
	{
		shared_ptr<Shader> shader = GET_SINGLE(Resources)->Get<Shader>(L"ComputeShader");

		// UAV 용 Texture 생성
		shared_ptr<Texture> texture = GET_SINGLE(Resources)->CreateTexture(L"UAVTexture",
			DXGI_FORMAT_R8G8B8A8_UNORM, 1024, 1024,
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		shared_ptr<Material> material = GET_SINGLE(Resources)->Get<Material>(L"ComputeShader");
		material->SetShader(shader);
		material->SetInt(0, 1);
		gEngine->GetComputeDescHeap()->SetUAV(texture->GetUAVHandle(), UAV_REGISTER::u0);

		// 쓰레드 그룹 (1 * 1024 * 1)
		material->Dispatch(1, 1024, 1);
	}
#pragma endregion

	shared_ptr<Scene> scene = make_shared<Scene>();
	
#pragma region Camera
	{
		shared_ptr<GameObject> camera = make_shared<GameObject>();
		camera->SetName(L"Main_Camera");
		camera->AddComponent(make_shared<Transform>());
		camera->AddComponent(make_shared<Camera>()); // Near=1, Far=1000, FOV=45도
		camera->AddComponent(make_shared<TestCameraScript>());
		camera->GetCamera()->SetFar(10000.f);
		camera->GetTransform()->SetLocalPosition(Vec3(1250.f, 1665.f, -485.74f));
		camera->GetTransform()->SetLocalRotation(Vec3(0.848181, -0.929444f, 0.f));
		uint8 layerIndex = GET_SINGLE(SceneManager)->LayerNameToIndex(L"UI");
		camera->GetCamera()->SetCullingMaskLayerOnOff(layerIndex, true); // UI는 안 찍음
		scene->AddGameObject(camera);
	}	
#pragma endregion

#pragma region UI_Camera
	{
		shared_ptr<GameObject> camera = make_shared<GameObject>();
		camera->SetName(L"Orthographic_Camera");
		camera->AddComponent(make_shared<Transform>());
		camera->AddComponent(make_shared<Camera>()); // Near=1, Far=1000, 800*600
		camera->GetTransform()->SetLocalPosition(Vec3(0.f, 0.f, 0.f));
		camera->GetCamera()->SetProjectionType(PROJECTION_TYPE::ORTHOGRAPHIC);
		uint8 layerIndex = GET_SINGLE(SceneManager)->LayerNameToIndex(L"UI");
		camera->GetCamera()->SetCullingMaskAll(); // 다 끄고
		camera->GetCamera()->SetCullingMaskLayerOnOff(layerIndex, false); // UI만 찍음
		scene->AddGameObject(camera);
	}
#pragma endregion

#pragma region SkyBox
	{
		shared_ptr<GameObject> skybox = make_shared<GameObject>();
		skybox->AddComponent(make_shared<Transform>());
		skybox->SetCheckFrustum(false);
		shared_ptr<MeshRenderer> meshRenderer = make_shared<MeshRenderer>();
		{
			shared_ptr<Mesh> sphereMesh = GET_SINGLE(Resources)->LoadSphereMesh();
			meshRenderer->SetMesh(sphereMesh);
		}
		{
			shared_ptr<Shader> shader = GET_SINGLE(Resources)->Get<Shader>(L"Skybox");
			shared_ptr<Texture> texture = GET_SINGLE(Resources)->Load<Texture>(L"Sky01", L"..\\Resources\\Texture\\Sky01.jpg");
			shared_ptr<Material> material = make_shared<Material>();
			material->SetShader(shader);
			material->SetTexture(0, texture);
			meshRenderer->SetMaterial(material);
		}
		skybox->AddComponent(meshRenderer);
		scene->AddGameObject(skybox);
	}
#pragma endregion

#pragma region UI_Test
	for (int32 i = 0; i < 6; i++) {
		shared_ptr<GameObject> obj = make_shared<GameObject>();
		obj->SetLayerIndex(GET_SINGLE(SceneManager)->LayerNameToIndex(L"UI")); // UI
		obj->AddComponent(make_shared<Transform>());
		obj->GetTransform()->SetLocalScale(Vec3(100.f, 80.f, 100.f));
		obj->GetTransform()->SetLocalPosition(Vec3(-600.f + (i * 105), 330.f, 500.f));
		shared_ptr<MeshRenderer> meshRenderer = make_shared<MeshRenderer>();
		{
			shared_ptr<Mesh> mesh = GET_SINGLE(Resources)->LoadRectangleMesh();
			meshRenderer->SetMesh(mesh);
		}
		{
			shared_ptr<Shader> shader = GET_SINGLE(Resources)->Get<Shader>(L"Texture");

			shared_ptr<Texture> texture;
			if (i < 3)
				texture = gEngine->GetRTGroup(RENDER_TARGET_GROUP_TYPE::G_BUFFER)->GetRTTexture(i);
			else if (i < 5)
				texture = gEngine->GetRTGroup(RENDER_TARGET_GROUP_TYPE::LIGHTING)->GetRTTexture(i - 3);
			else
				texture = gEngine->GetRTGroup(RENDER_TARGET_GROUP_TYPE::SHADOW)->GetRTTexture(0);

			shared_ptr<Material> material = make_shared<Material>();
			material->SetShader(shader);
			material->SetTexture(0, texture);
			meshRenderer->SetMaterial(material);
		}
		obj->AddComponent(meshRenderer);
		scene->AddGameObject(obj);
	}
#pragma endregion

#pragma region Directional Light
	{
		shared_ptr<GameObject> light = make_shared<GameObject>();
		light->AddComponent(make_shared<Transform>());
		light->AddComponent(make_shared<Light>());
		light->GetTransform()->SetLocalPosition(Vec3(1447.f, 5600.f, -2099.f));
		light->GetLight()->SetLightDirection(Vec3(0.f, -1.f, 1.f));
		light->GetLight()->SetLightType(LIGHT_TYPE::DIRECTIONAL_LIGHT);
		// Diffuse: 밝고 강한 색상, 카툰 스타일에서 명확한 경계를 만듭니다.
		light->GetLight()->SetDiffuse(Vec3(1.f, 1.f, 1.f));

		// Ambient: 낮은 값, 전체 장면의 기본 밝기를 설정합니다.
		light->GetLight()->SetAmbient(Vec3(0.05f, 0.05f, 0.05f));

		// Specular: 강조된 반사, 카툰 스타일 하이라이트를 강조합니다.
		light->GetLight()->SetSpecular(Vec3(0.2f, 0.2f, 0.2f));

		scene->AddGameObject(light);
	}
#pragma endregion

#pragma region Map
	{
		shared_ptr<MeshData> meshData = GET_SINGLE(Resources)->LoadFBX(L"..\\Resources\\FBX\\Stage1.fbx");
		vector<shared_ptr<GameObject>> gameObjects = meshData->Instantiate();

		for (auto& gameObject : gameObjects) {
			gameObject->SetName(L"Map");
			gameObject->SetCheckFrustum(false);
			gameObject->SetStatic(false);
			gameObject->GetTransform()->SetLocalPosition(Vec3(0.f, 0.f, 0.f));
			gameObject->GetTransform()->SetLocalScale(Vec3(1.f, 1.f, 1.f));
			gameObject->GetTransform()->SetLocalRotation(Vec3(-XM_PIDIV2, 0.f, 0.f));

			scene->AddGameObject(gameObject);
			gameObject->AddComponent(make_shared<TestMap>());
		}
	}

	{
		shared_ptr<MeshData> meshData = GET_SINGLE(Resources)->LoadFBX(L"..\\Resources\\FBX\\Gun 01.fbx");
		vector<shared_ptr<GameObject>> gameObjects = meshData->Instantiate();

		for (auto& gameObject : gameObjects) {
			gameObject->SetName(L"Gun");
			gameObject->SetCheckFrustum(false);
			gameObject->SetStatic(false);
			gameObject->GetTransform()->SetLocalPosition(Vec3(-108.f, 250.f, 1877.f));
			gameObject->GetTransform()->SetLocalScale(Vec3(2.f, 2.f, 2.f));
			gameObject->GetTransform()->SetLocalRotation(Vec3(0.f, 0.f, 0.f));

			scene->AddGameObject(gameObject);
		}
	}

	{
		//shared_ptr<MeshData> meshData = GET_SINGLE(Resources)->LoadFBX(L"..\\Resources\\FBX\\Stage1_Mimic.fbx");
		shared_ptr<MeshData> meshData = GET_SINGLE(Resources)->Load<MeshData>(L"Stage1_Mimic", L"..\\Resources\\FBX\\Stage2_Haunt.meshdata");

		vector<shared_ptr<GameObject>> gameObjects = meshData->Instantiate();
		for (auto& gameObject : gameObjects) {
			gameObject->SetName(L"Stage1_Mimic");
			gameObject->SetCheckFrustum(false);
			gameObject->SetStatic(false);
			gameObject->GetTransform()->SetLocalPosition(Vec3(-1493.09143, 173.648636, -399.480347));
			gameObject->GetTransform()->SetLocalScale(Vec3(1.f, 1.f, 1.f));
			gameObject->GetTransform()->SetLocalRotation(Vec3(0.f, -0.9f, 0.f));

			scene->AddGameObject(gameObject);
			gameObject->AddComponent(make_shared<TestAnimation>());
		}
	}

	{
		//shared_ptr<MeshData> meshData = GET_SINGLE(Resources)->LoadFBX(L"..\\Resources\\FBX\\Stage1_Mimic.fbx");
		shared_ptr<MeshData> meshData = GET_SINGLE(Resources)->Load<MeshData>(L"Stage4_Metal Robot", L"..\\Resources\\FBX\\Stage4_Metal Robot.meshdata");

		vector<shared_ptr<GameObject>> gameObjects = meshData->Instantiate();

		for (auto& gameObject : gameObjects) {
			gameObject->SetName(L"Metal Robot");
			gameObject->SetCheckFrustum(false);
			gameObject->SetStatic(false);
			gameObject->GetTransform()->SetLocalPosition(Vec3(-30.f, 172.f, 1684.f));
			gameObject->GetTransform()->SetLocalScale(Vec3(1.f, 1.f, 1.f));

			scene->AddGameObject(gameObject);
			gameObject->AddComponent(make_shared<TestAnimation2>());
		}
	}

#pragma endregion

	{
		shared_ptr<GameObject> bullet = make_shared<GameObject>();
		bullet->AddComponent(make_shared<Transform>());
		bullet->SetName(L"Bullet");
		shared_ptr<Transform> transform = bullet->GetTransform();
		transform->SetLocalPosition(Vec3(-30.f, 272.f, 1584.f));
		transform->SetLocalScale(Vec3(10.f, 10.f, 10.f));
		bullet->SetCheckFrustum(false);
		shared_ptr<MeshRenderer> meshRenderer = make_shared<MeshRenderer>();
		{
			shared_ptr<Mesh> sphereMesh = GET_SINGLE(Resources)->LoadSphereMesh();
			meshRenderer->SetMesh(sphereMesh);
		}
		{
			shared_ptr<Material> material = GET_SINGLE(Resources)->Get<Material>(L"Bullet");
			material->SetFloat(0, 0.f);
			meshRenderer->SetMaterial(material);
		}
		bullet->AddComponent(meshRenderer);
		scene->AddGameObject(bullet);
	}

#pragma region ParticleSystem
	{
		shared_ptr<GameObject> particle = make_shared<GameObject>();
		particle->AddComponent(make_shared<Transform>());
		particle->AddComponent(make_shared<ParticleSystem>());
		particle->SetCheckFrustum(false);
		particle->GetTransform()->SetLocalPosition(Vec3(1447.f, 256.f, -2099.f));
		scene->AddGameObject(particle);
	}
#pragma endregion

	shared_ptr<GameObject> water = make_shared<GameObject>();
	water->SetName(L"Water");
	water->AddComponent(make_shared<Transform>());
	water->GetTransform()->SetLocalScale(Vec3(3000.f, 300.f, 3000.f));
	water->GetTransform()->SetLocalPosition(Vec3(1000.f, 500.f, 1004.f));
	// x축 90도 회전
	water->SetCheckFrustum(false);
	water->SetStatic(false);

	shared_ptr<MeshRenderer> meshRenderer = make_shared<MeshRenderer>();
	shared_ptr<Mesh> mesh = GET_SINGLE(Resources)->LoadCubeMesh();
	meshRenderer->SetMesh(mesh);

	shared_ptr<Material> material = GET_SINGLE(Resources)->Get<Material>(L"Water");
	meshRenderer->SetMaterial(material);

	water->AddComponent(meshRenderer);
	water->AddComponent(make_shared<WaterScript>());

	//scene->AddGameObject(water);

	return scene;	
}