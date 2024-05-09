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
#include "PhysXComponent.h"
#include "Timer.h"
#include "TestMap.h"
#include "Mesh.h"
#include "FBXLoader.h"
#include "TestAnimation.h"

void SceneManager::Update()
{
	if (activeScene == nullptr)
		return;

	activeScene->Update();
	activeScene->LateUpdate();
	PhysicsUpdate();
	activeScene->FinalUpdate();
}

void SceneManager::PhysicsUpdate()
{
	gEngine->GetDefaultScene()->simulate(DELTA_TIME);
	gEngine->GetDefaultScene()->fetchResults(true);
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

#pragma region Object
	{
		/*shared_ptr<GameObject> obj = make_shared<GameObject>();
		obj->SetName(L"OBJ");
		obj->AddComponent(make_shared<Transform>());
		obj->AddComponent(make_shared<SphereCollider>());
		obj->GetTransform()->SetLocalScale(Vec3(100.f, 100.f, 100.f));
		obj->GetTransform()->SetLocalPosition(Vec3(0, 0.f, 500.f));
		obj->SetStatic(false);
		shared_ptr<MeshRenderer> meshRenderer = make_shared<MeshRenderer>();
		{
			shared_ptr<Mesh> sphereMesh = GET_SINGLE(Resources)->LoadSphereMesh();
			meshRenderer->SetMesh(sphereMesh);
		}
		{
			shared_ptr<Material> material = GET_SINGLE(Resources)->Get<Material>(L"GameObject");
			meshRenderer->SetMaterial(material->Clone());
		}
		dynamic_pointer_cast<SphereCollider>(obj->GetCollider())->SetRadius(0.5f);
		dynamic_pointer_cast<SphereCollider>(obj->GetCollider())->SetCenter(Vec3(0.f, 0.f, 0.f));
		obj->AddComponent(meshRenderer);
		scene->AddGameObject(obj);*/
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
		light->GetTransform()->SetLocalPosition(Vec3(0, 100, 0));
		light->AddComponent(make_shared<Light>());
		light->GetLight()->SetLightDirection(Vec3(0.f, -1.f, 0.f));
		light->GetLight()->SetLightType(LIGHT_TYPE::DIRECTIONAL_LIGHT);
		light->GetLight()->SetDiffuse(Vec3(0.7f, 0.7f, 0.7f));
		light->GetLight()->SetAmbient(Vec3(0.2f, 0.2f, 0.2f));
		light->GetLight()->SetSpecular(Vec3(0.2f, 0.2f, 0.2f));

		scene->AddGameObject(light);
	}
#pragma endregion

	PxPhysics* physics = gEngine->GetPhysics();
	PxScene* defaultScene = gEngine->GetDefaultScene();
	PxControllerManager* controllerManager = gEngine->GetControllerManager();
	PxMaterial* defaultMaterial = gEngine->GetDefaultMaterial();
#pragma region Hamster
	{
		//shared_ptr<MeshData> meshData = GET_SINGLE(Resources)->LoadFBX(L"..\\Resources\\FBX\\Hamster.fbx");
		shared_ptr<MeshData> meshData = GET_SINGLE(Resources)->Load<MeshData>(L"HamsterMeshData", L"..\\Resources\\FBX\\Hamster.meshdata");

		vector<shared_ptr<GameObject>> gameObjects = meshData->Instantiate();

		for (auto& gameObject : gameObjects) {
			gameObject->SetName(L"Hamster");
			gameObject->SetCheckFrustum(false);
			gameObject->GetTransform()->SetLocalScale(Vec3(200.f, 200.f, 200.f));
			gameObject->GetTransform()->SetLocalRotation(Vec3(-1.6f, 3.2f, 0.f));
			gameObject->SetStatic(false);

			PxCapsuleControllerDesc desc;
			desc.height = 50.0f;
			desc.radius = 25.0f;
			desc.climbingMode = PxCapsuleClimbingMode::eCONSTRAINED;
			desc.position = PxExtendedVec3(1447.f, 256.f, -2099.f);
			desc.material = defaultMaterial;
			desc.contactOffset = 0.1f; // 땅과의 거리
			desc.stepOffset = 40.f; // 계단 높이
			desc.slopeLimit = cosf(PxDegToRad(45.f)); // 경사로
			desc.invisibleWallHeight = 0.0f; // 벽 높이
			desc.maxJumpHeight = 0.0f; // 점프 높이
			desc.reportCallback = nullptr; // PxUserControllerHitReport
			desc.behaviorCallback = nullptr; // PxControllerBehaviorCallback
			desc.nonWalkableMode = PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING;
			desc.material = defaultMaterial;

			// PxController 생성
			PxController* controller = controllerManager->createController(desc);
			controller->setUpDirection(PxVec3(0.f, 1.f, 0.f));

			scene->AddGameObject(gameObject);
			gameObject->AddComponent(make_shared<PlayerScript>());
		}
	}
#pragma endregion


#pragma region Map
	{
		shared_ptr<MeshData> meshData = GET_SINGLE(Resources)->LoadFBX(L"..\\Resources\\FBX\\Stage1.fbx");
		//shared_ptr<MeshData> meshData = GET_SINGLE(Resources)->Load<MeshData>(L"MapMeshData", L"..\\Resources\\FBX\\Hamster.meshdata");

		vector<shared_ptr<GameObject>> gameObjects = meshData->Instantiate();

		for (auto& gameObject : gameObjects) {
			gameObject->SetName(L"Map");
			gameObject->SetCheckFrustum(true);
			gameObject->SetStatic(true);
			gameObject->GetTransform()->SetLocalPosition(Vec3(0.f, 0.f, 0.f));
			gameObject->GetTransform()->SetLocalScale(Vec3(1.f, 1.f, 1.f));
			gameObject->GetTransform()->SetLocalRotation(Vec3(-XM_PIDIV2, 0.f, 0.f));

			scene->AddGameObject(gameObject);
			gameObject->AddComponent(make_shared<TestMap>());
		}

		FbxMeshInfo* meshInfo = meshData->GetMesh(0)->GetFbxMeshInfo();

		vector<PxVec3> physxVertices;
		physxVertices.reserve(meshInfo->vertices.size());

		vector<PxU32> physxIndices;
		physxIndices.reserve(meshInfo->indices[0].size());

		for (const auto& vertex : meshInfo->vertices) {
			physxVertices.emplace_back(vertex.pos.x, vertex.pos.y, vertex.pos.z);
		}

		for (const auto& index : meshInfo->indices[0]) {
			physxIndices.emplace_back(index);
		}

		// PxCookingParams 설정
		PxCookingParams params(physics->getTolerancesScale());
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
			cerr << "Failed to cook triangle mesh." << endl;
		}

		if (writeBuffer.getSize() == 0) {
			cerr << "WriteBuffer is empty." << endl;
		}

		// 직렬화된 데이터를 PxInputStream으로 변환
		PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
		PxTriangleMesh* triangleMesh = physics->createTriangleMesh(readBuffer);

		PxTriangleMeshGeometry triangleMeshGeometry(triangleMesh, PxMeshScale(PxVec3(1, 1, 1)));
		PxRigidStatic* triangleMeshActor = physics->createRigidStatic(PxTransform(PxVec3(0, 0, 0)));
		PxShape* triangleMeshShape = physics->createShape(triangleMeshGeometry, *defaultMaterial);

		// x축 기준 -90도 회전
		PxQuat quat(-XM_PIDIV2, PxVec3(1, 0, 0));
		triangleMeshActor->setGlobalPose(PxTransform(PxVec3(0, 0, 0), quat));

		triangleMeshActor->attachShape(*triangleMeshShape);
		defaultScene->addActor(*triangleMeshActor);

		triangleMeshShape->release();
	}
#pragma endregion

	//{
	//	//shared_ptr<MeshData> meshData = GET_SINGLE(Resources)->LoadFBX(L"..\\Resources\\FBX\\Stage1_Mimic.fbx");
	//	shared_ptr<MeshData> meshData = GET_SINGLE(Resources)->Load<MeshData>(L"Stage1_Mimic", L"..\\Resources\\FBX\\Stage1_Mimic.meshdata");

	//	vector<shared_ptr<GameObject>> gameObjects = meshData->Instantiate();

	//	for (auto& gameObject : gameObjects) {
	//		gameObject->SetName(L"Stage1_Mimic");
	//		gameObject->SetCheckFrustum(true);
	//		gameObject->GetTransform()->SetLocalPosition(Vec3(-2054.f, 4.f, -1173.f));
	//		gameObject->GetTransform()->SetLocalScale(Vec3(100.f, 100.f, 100.f));
	//		gameObject->GetTransform()->SetLocalRotation(Vec3(-1.6f, -1.1f, 0.f));

	//		scene->AddGameObject(gameObject);
	//		gameObject->AddComponent(make_shared<TestAnimation>());
	//	}
	//}

	//{
	//	//shared_ptr<MeshData> meshData = GET_SINGLE(Resources)->LoadFBX(L"..\\Resources\\FBX\\Stage1_SkeletonBird.fbx");
	//	shared_ptr<MeshData> meshData = GET_SINGLE(Resources)->Load<MeshData>(L"Stage1_SkeletonBird", L"..\\Resources\\FBX\\Stage1_SkeletonBird.meshdata");

	//	vector<shared_ptr<GameObject>> gameObjects = meshData->Instantiate();

	//	for (auto& gameObject : gameObjects) {
	//		gameObject->SetName(L"Stage1_SkeletonBird");
	//		gameObject->SetCheckFrustum(true);
	//		gameObject->GetTransform()->SetLocalPosition(Vec3(-1881.f, 5.7f, -1005.f));
	//		gameObject->GetTransform()->SetLocalScale(Vec3(1.f, 1.f, 1.f));
	//		gameObject->GetTransform()->SetLocalRotation(Vec3(-1.6f, -0.3f, 0.f));

	//		scene->AddGameObject(gameObject);
	//		//gameObject->AddComponent(make_shared<TestAnimation>());
	//	}
	//}

	//{
	//	//shared_ptr<MeshData> meshData = GET_SINGLE(Resources)->LoadFBX(L"..\\Resources\\FBX\\Stage2_Haunt.fbx");
	//	shared_ptr<MeshData> meshData = GET_SINGLE(Resources)->Load<MeshData>(L"Stage2_Haunt", L"..\\Resources\\FBX\\Stage2_Haunt.meshdata");

	//	vector<shared_ptr<GameObject>> gameObjects = meshData->Instantiate();

	//	for (auto& gameObject : gameObjects) {
	//		gameObject->SetName(L"Stage2_Haunt");
	//		gameObject->SetCheckFrustum(true);
	//		gameObject->GetTransform()->SetLocalPosition(Vec3(-1733.6f, 5.7f, -993.f));
	//		gameObject->GetTransform()->SetLocalScale(Vec3(1.f, 1.f, 1.f));
	//		gameObject->GetTransform()->SetLocalRotation(Vec3(-1.6f, -0.01f, 0.f));

	//		scene->AddGameObject(gameObject);
	//		//gameObject->AddComponent(make_shared<TestAnimation>());
	//	}
	//}

	//{
	//	//shared_ptr<MeshData> meshData = GET_SINGLE(Resources)->LoadFBX(L"..\\Resources\\FBX\\Stage2_TelepachyRat.fbx");
	//	shared_ptr<MeshData> meshData = GET_SINGLE(Resources)->Load<MeshData>(L"Stage2_TelepachyRat", L"..\\Resources\\FBX\\Stage2_TelepachyRat.meshdata");

	//	vector<shared_ptr<GameObject>> gameObjects = meshData->Instantiate();

	//	for (auto& gameObject : gameObjects) {
	//		gameObject->SetName(L"Stage2_TelepachyRat");
	//		gameObject->SetCheckFrustum(true);
	//		gameObject->GetTransform()->SetLocalPosition(Vec3(-1617.f, 5.6f, -1019.f));
	//		gameObject->GetTransform()->SetLocalScale(Vec3(1.f, 1.f, 1.f));
	//		gameObject->GetTransform()->SetLocalRotation(Vec3(-1.6f, -0.024f, 0.f));

	//		scene->AddGameObject(gameObject);
	//		//gameObject->AddComponent(make_shared<TestAnimation>());
	//	}
	//}

	//{
	//	//shared_ptr<MeshData> meshData = GET_SINGLE(Resources)->LoadFBX(L"..\\Resources\\FBX\\Stage3_Alien_Plant.fbx");
	//	shared_ptr<MeshData> meshData = GET_SINGLE(Resources)->Load<MeshData>(L"Stage3_Alien_Plant", L"..\\Resources\\FBX\\Stage3_Alien_Plant.meshdata");

	//	vector<shared_ptr<GameObject>> gameObjects = meshData->Instantiate();

	//	for (auto& gameObject : gameObjects) {
	//		gameObject->SetName(L"Stage3_Alien_Plant");
	//		gameObject->SetCheckFrustum(true);
	//		gameObject->GetTransform()->SetLocalPosition(Vec3(-1506.f, 5.56f, -1171.f));
	//		gameObject->GetTransform()->SetLocalScale(Vec3(1.f, 1.f, 1.f));
	//		gameObject->GetTransform()->SetLocalRotation(Vec3(-1.6f, 0.382f, 0.f));

	//		scene->AddGameObject(gameObject);
	//		//gameObject->AddComponent(make_shared<TestAnimation>());
	//	}
	//}

	//{
	//	//shared_ptr<MeshData> meshData = GET_SINGLE(Resources)->LoadFBX(L"..\\Resources\\FBX\\Stage3_EPlant.fbx");
	//	shared_ptr<MeshData> meshData = GET_SINGLE(Resources)->Load<MeshData>(L"Stage3_EPlant", L"..\\Resources\\FBX\\Stage3_EPlant.meshdata");

	//	vector<shared_ptr<GameObject>> gameObjects = meshData->Instantiate();

	//	for (auto& gameObject : gameObjects) {
	//		gameObject->SetName(L"Stage3_EPlant");
	//		gameObject->SetCheckFrustum(true);
	//		gameObject->GetTransform()->SetLocalPosition(Vec3(-1443.f, 5.4f, -1299.f));
	//		gameObject->GetTransform()->SetLocalScale(Vec3(1.f, 1.f, 1.f));
	//		gameObject->GetTransform()->SetLocalRotation(Vec3(-1.6f, 0.879f, 0.f));

	//		scene->AddGameObject(gameObject);
	//		//gameObject->AddComponent(make_shared<TestAnimation>());
	//	}
	//}

	//{
	//	//shared_ptr<MeshData> meshData = GET_SINGLE(Resources)->LoadFBX(L"..\\Resources\\FBX\\Stage4_Metal Robot.fbx");
	//	shared_ptr<MeshData> meshData = GET_SINGLE(Resources)->Load<MeshData>(L"Stage4_Metal Robot", L"..\\Resources\\FBX\\Stage4_Metal Robot.meshdata");

	//	vector<shared_ptr<GameObject>> gameObjects = meshData->Instantiate();

	//	for (auto& gameObject : gameObjects) {
	//		gameObject->SetName(L"Stage4_Metal Robot");
	//		gameObject->SetCheckFrustum(true);
	//		gameObject->GetTransform()->SetLocalPosition(Vec3(-1451.6f, 4.879f, -1452.f));
	//		gameObject->GetTransform()->SetLocalScale(Vec3(1.f, 1.f, 1.f));
	//		gameObject->GetTransform()->SetLocalRotation(Vec3(-1.6f, 1.5f, 0.f));

	//		scene->AddGameObject(gameObject);
	//		//gameObject->AddComponent(make_shared<TestAnimation>());
	//	}
	//}

	return scene;	
}