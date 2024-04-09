#include "pch.h"
#include "FBXLoader.h"
#include "Mesh.h"
#include "Resources.h"
#include "Shader.h"
#include "Material.h"

FBXLoader::FBXLoader()
{
	int32 count = _animNames.GetCount();
	for (int32 i = 0; i < count; i++) {
		_animNames[i]->Clear();
		delete  _animNames[i];
	}
	_animNames.Clear();
}

FBXLoader::~FBXLoader()
{
	if (_scene)
		_scene->Destroy();
	if (_manager)
		_manager->Destroy();
}

void FBXLoader::LoadFbx(const wstring& path)
{
	// 파일 데이터 로드
	Import(path);

	// Animation	
	/*LoadBones(_scene->GetRootNode());
	LoadAnimationInfo();*/

	// 로드된 데이터 파싱 (Mesh/Material/Skin)
	ParseNode(_scene->GetRootNode());

	// 우리 구조에 맞게 Texture / Material 생성
	CreateTextures();
	CreateMaterials();
}

void FBXLoader::Import(const wstring& path)
{
	// FBX SDK 관리자 객체 생성
	_manager = FbxManager::Create();

	// IOSettings 객체 생성 및 설정
	FbxIOSettings* settings = FbxIOSettings::Create(_manager, IOSROOT);
	_manager->SetIOSettings(settings);

	// FbxImporter 객체 생성
	_scene = FbxScene::Create(_manager, "");

	// 나중에 Texture 경로 계산할 때 쓸 것
	_resourceDirectory = fs::path(path).parent_path().wstring() + L"\\" + fs::path(path).filename().stem().wstring() + L".fbm";

	_importer = FbxImporter::Create(_manager, "");

	string strPath = ws2s(path);
	_importer->Initialize(strPath.c_str(), -1, _manager->GetIOSettings());

	_importer->Import(_scene);

	_scene->GetGlobalSettings().SetAxisSystem(FbxAxisSystem::DirectX);

	// 씬 내에서 삼각형화 할 수 있는 모든 노드를 삼각형화 시킨다.
	FbxGeometryConverter geometryConverter(_manager);
	geometryConverter.Triangulate(_scene, true);

	_importer->Destroy();
}

void FBXLoader::LoadBones(FbxNode* node, int32_t idx, int32_t parentIdx)
{
	FbxNodeAttribute* attribute = node->GetNodeAttribute();

	if (attribute && attribute->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	{
		shared_ptr<FbxBoneInfo> bone = make_shared<FbxBoneInfo>();
		bone->boneName = s2ws(node->GetName());
		bone->parentIndex = parentIdx;
		_bones.push_back(bone);
	}

	const int32 childCount = node->GetChildCount();
	for (int32 i = 0; i < childCount; i++)
		LoadBones(node->GetChild(i), static_cast<int32>(_bones.size()), idx);
}

void FBXLoader::LoadAnimationInfo()
{
	_scene->FillAnimStackNameArray(OUT _animNames);

	const int32 animCount = _animNames.GetCount();
	for (int32 i = 0; i < animCount; i++)
	{
		FbxAnimStack* animStack = _scene->FindMember<FbxAnimStack>(_animNames[i]->Buffer());
		if (animStack == nullptr)
			continue;

		shared_ptr<FbxAnimClipInfo> animClip = make_shared<FbxAnimClipInfo>();
		animClip->name = s2ws(animStack->GetName());
		animClip->keyFrames.resize(_bones.size()); // 키프레임은 본의 개수만큼

		FbxTakeInfo* takeInfo = _scene->GetTakeInfo(animStack->GetName());
		animClip->startTime = takeInfo->mLocalTimeSpan.GetStart();
		animClip->endTime = takeInfo->mLocalTimeSpan.GetStop();
		animClip->mode = _scene->GetGlobalSettings().GetTimeMode();

		_animClips.push_back(animClip);
	}
}

void FBXLoader::ParseNode(FbxNode* node)
{
	FbxNodeAttribute* attribute = node->GetNodeAttribute();

	if (attribute)
		if (attribute->GetAttributeType() == FbxNodeAttribute::eMesh)
			LoadMesh(node->GetMesh());

	// Material 로드
	const uint32 materialCount = node->GetMaterialCount();
	for (uint32 i = 0; i < materialCount; ++i) {
		FbxSurfaceMaterial* surfaceMaterial = node->GetMaterial(i);
		LoadMaterial(surfaceMaterial);
	}

	// Tree 구조 재귀 호출
	const int32 childCount = node->GetChildCount();
	for (int32 i = 0; i < childCount; ++i)
		ParseNode(node->GetChild(i));
}

void FBXLoader::LoadMesh(FbxMesh* mesh)
{
	// 새 메시를 위한 meshInfo 생성
	_meshes.push_back(FbxMeshInfo());
	FbxMeshInfo& meshInfo = _meshes.back();
	meshInfo.name = s2ws(mesh->GetName());

	// 제어 점 인덱스와 UV 좌표에 따른 고유 정점을 추적하기 위한 맵 생성.
	std::unordered_map<int32_t, std::vector<std::pair<int32_t, FbxVector2>>> uniqueVerticesMap;

	// FBX 메시에서 정점, 노말, UV 정보등을 가져온다.
	FbxVector4* controlPoints = mesh->GetControlPoints();
	//int controlPointsCount = mesh->GetControlPointsCount();

	const int32 materialCount = mesh->GetNode()->GetMaterialCount();
	meshInfo.indices.resize(materialCount);

	FbxGeometryElementMaterial* geometryElementMaterial = mesh->GetElementMaterial();

	// 삼각형 메시가 아니면 오류
	const int32 polygonSize = mesh->GetPolygonSize(0);
	assert(polygonSize == 3);

	uint32 arrIdx[3];
	uint32 vertexCounter = 0; // 정점의 개수

	const int32 triCount = mesh->GetPolygonCount(); // 메쉬의 삼각형 개수를 가져온다
	// 각 폴리곤에 대해 반복.
	for (int32 i = 0; i < triCount; ++i) {
		for (int32 j = 0; j < 3; ++j) {
			// 제어 점 인덱스를 가져옴.
			int32 controlPointIndex = mesh->GetPolygonVertex(i, j);

			// 현재 폴리곤의 정점에 대한 UV를 읽음.
			FbxVector2 currentUV;
			bool unmappedUV;
			const char* uvSetName = mesh->GetElementUV()->GetName();
			mesh->GetPolygonVertexUV(i, j, uvSetName, currentUV, unmappedUV);

			// 이 제어 점 인덱스와 UV 조합이 고유한지 확인.
			auto& verticesList = uniqueVerticesMap[controlPointIndex];
			auto it = std::find_if(verticesList.begin(), verticesList.end(),
				[&currentUV](const std::pair<int32_t, FbxVector2>& elem) {
					return elem.second == currentUV;
				});

			if (it != verticesList.end()) {
				// 기존 정점 인덱스를 다시 사용하여 폴리곤 인덱스 배열에 추가.
				arrIdx[j] = it->first;
			}
			else {
				// 새 정점을 만들고 메쉬의 정점에 추가.
				// 이것은 controlPointIndex에 있는 정점을 복제하고 새 UV 좌표를 설정하는 것을 포함함.
				Vertex newVertex = {};
				newVertex.pos.x = static_cast<float>(controlPoints[controlPointIndex].mData[0]);
				newVertex.pos.y = static_cast<float>(controlPoints[controlPointIndex].mData[2]);
				newVertex.pos.z = static_cast<float>(controlPoints[controlPointIndex].mData[1]);
				newVertex.uv = Vec2(currentUV.mData[0], 1.f - currentUV.mData[1]);
				meshInfo.vertices.push_back(newVertex);

				BoneWeight newBoneWeight = {};
				meshInfo.boneWeights.push_back(newBoneWeight);

				// 새 인덱스를 저장.
				int32_t newVertexIndex = static_cast<int32_t>(meshInfo.vertices.size()) - 1;
				arrIdx[j] = newVertexIndex;

				// 이 정점 인덱스와 현재 UV를 맵에 저장.
				verticesList.push_back(std::make_pair(newVertexIndex, currentUV));

				// 노말, 탄젠트 등의 데이터를 새 정점에 설정.
				GetNormal(mesh, &meshInfo, newVertexIndex, vertexCounter);
				GetTangent(mesh, &meshInfo, newVertexIndex, vertexCounter);
			}
			++vertexCounter;
		}

		// 삼각형의 정점 인덱스를 메쉬의 인덱스 배열에 추가.
		const uint32 subsetIdx = geometryElementMaterial->GetIndexArray().GetAt(i);
		meshInfo.indices[subsetIdx].push_back(arrIdx[0]);
		meshInfo.indices[subsetIdx].push_back(arrIdx[2]);
		meshInfo.indices[subsetIdx].push_back(arrIdx[1]);
	}

	// Animation
	LoadAnimationData(mesh, &meshInfo);
}

//void FBXLoader::LoadMesh(FbxMesh* mesh)
//{
//	// 새 메시를 위한 meshInfo 생성
//	_meshes.push_back(FbxMeshInfo());
//	FbxMeshInfo& meshInfo = _meshes.back();
//	meshInfo.name = s2ws(mesh->GetName());
//
//	// 제어 점 인덱스와 UV 좌표에 따른 고유 정점을 추적하기 위한 맵 생성.
//	std::unordered_map<int32_t, std::vector<std::pair<int32_t, FbxVector2>>> uniqueVerticesMap;
//
//	// FBX 메시에서 정점, 노말, UV 정보등을 가져온다.
//	//const int32 vertexCount = mesh->GetControlPointsCount();
//	//const int32 vertexCount = vertexCounter;
//	//meshInfo.vertices.resize(vertexCount);
//	//meshInfo.boneWeights.resize(vertexCount);
//
//	FbxVector4* controlPoints = mesh->GetControlPoints();
//	//int controlPointsCount = mesh->GetControlPointsCount();
//	//meshInfo.boneWeights.resize(controlPointsCount);
//	//for (int32 i = 0; i < vertexCount; ++i) {
//	//	meshInfo.vertices[i].pos.x = static_cast<float>(controlPoints[i].mData[0]);
//	//	meshInfo.vertices[i].pos.y = static_cast<float>(controlPoints[i].mData[2]);
//	//	meshInfo.vertices[i].pos.z = static_cast<float>(controlPoints[i].mData[1]);
//	//}
//
//	const int32 materialCount = mesh->GetNode()->GetMaterialCount();
//	meshInfo.indices.resize(materialCount);
//
//	FbxGeometryElementMaterial* geometryElementMaterial = mesh->GetElementMaterial();
//
//	// 삼각형 메시가 아니면 오류
//	const int32 polygonSize = mesh->GetPolygonSize(0);
//	assert(polygonSize == 3);
//
//	uint32 arrIdx[3];
//	uint32 vertexCounter = 0; // 정점의 개수
//
//	const int32 triCount = mesh->GetPolygonCount(); // 메쉬의 삼각형 개수를 가져온다
//	// 각 폴리곤에 대해 반복.
//	for (int32 i = 0; i < triCount; ++i) {
//		for (int32 j = 0; j < 3; ++j) {
//			// 제어 점 인덱스를 가져옴.
//			int32 controlPointIndex = mesh->GetPolygonVertex(i, j);
//
//			// 현재 폴리곤의 정점에 대한 UV를 읽음.
//			FbxVector2 currentUV;
//			bool unmappedUV;
//			const char* uvSetName = mesh->GetElementUV()->GetName();
//			mesh->GetPolygonVertexUV(i, j, uvSetName, currentUV, unmappedUV);
//
//			// 이 제어 점 인덱스와 UV 조합이 고유한지 확인.
//			auto& verticesList = uniqueVerticesMap[controlPointIndex];
//			auto it = std::find_if(verticesList.begin(), verticesList.end(),
//				[&currentUV](const std::pair<int32_t, FbxVector2>& elem) {
//					return elem.second == currentUV;
//				});
//
//			if (it != verticesList.end()) {
//				// 기존 정점 인덱스를 다시 사용하여 폴리곤 인덱스 배열에 추가.
//				arrIdx[j] = it->first;
//			}
//			else {
//				// 새 정점을 만들고 메쉬의 정점에 추가.
//				// 이것은 controlPointIndex에 있는 정점을 복제하고 새 UV 좌표를 설정하는 것을 포함함.
//				Vertex newVertex = {};
//				newVertex.pos.x = static_cast<float>(controlPoints[controlPointIndex].mData[0]);
//				newVertex.pos.y = static_cast<float>(controlPoints[controlPointIndex].mData[2]);
//				newVertex.pos.z = static_cast<float>(controlPoints[controlPointIndex].mData[1]);
//				newVertex.uv = Vec2(currentUV.mData[0], 1.f - currentUV.mData[1]);
//				meshInfo.vertices.push_back(newVertex);
//
//				BoneWeight newBoneWeight = {};
//				meshInfo.boneWeights.push_back(newBoneWeight);
//
//				// 새 인덱스를 저장.
//				int32_t newVertexIndex = static_cast<int32_t>(meshInfo.vertices.size()) - 1;
//				arrIdx[j] = newVertexIndex;
//
//				// 이 정점 인덱스와 현재 UV를 맵에 저장.
//				verticesList.push_back(std::make_pair(newVertexIndex, currentUV));
//
//				// 노말, 탄젠트 등의 데이터를 새 정점에 설정.
//				GetNormal(mesh, &meshInfo, newVertexIndex, vertexCounter);
//				GetTangent(mesh, &meshInfo, newVertexIndex, vertexCounter);
//			}
//			++vertexCounter;
//		}
//
//		// 삼각형의 정점 인덱스를 메쉬의 인덱스 배열에 추가.
//		const uint32 subsetIdx = geometryElementMaterial->GetIndexArray().GetAt(i);
//		meshInfo.indices[subsetIdx].push_back(arrIdx[0]);
//		meshInfo.indices[subsetIdx].push_back(arrIdx[2]);
//		meshInfo.indices[subsetIdx].push_back(arrIdx[1]);
//	}
//
//	//int32 vertexCount = meshInfo.vertices.size();
//	//for (int32 i = 0; i < vertexCount; ++i) {
//	//	meshInfo.vertices[i].pos.x = static_cast<float>(controlPoints[i].mData[0]);
//	//	meshInfo.vertices[i].pos.y = static_cast<float>(controlPoints[i].mData[2]);
//	//	meshInfo.vertices[i].pos.z = static_cast<float>(controlPoints[i].mData[1]);
//	//}
//
//
//	//for (int32 i = 0; i < triCount; ++i) {
//	//	for (int32 j = 0; j < 3; ++j) {
//	//		int32 controlPointIndex = mesh->GetPolygonVertex(i, j); // 제어점의 인덱스 추출
//	//		arrIdx[j] = controlPointIndex;
//
//	//		GetNormal(mesh, &meshInfo, controlPointIndex, vertexCounter);
//	//		GetTangent(mesh, &meshInfo, controlPointIndex, vertexCounter);
//	//		GetUV(mesh, &meshInfo, controlPointIndex, mesh->GetTextureUVIndex(i, j));
//
//	//		++vertexCounter;
//	//	}
//
//	//	const uint32 subsetIdx = geometryElementMaterial->GetIndexArray().GetAt(i);
//	//	meshInfo.indices[subsetIdx].push_back(arrIdx[0]);
//	//	meshInfo.indices[subsetIdx].push_back(arrIdx[2]);
//	//	meshInfo.indices[subsetIdx].push_back(arrIdx[1]);
//	//}
//
//	// Animation
//	//meshInfo.boneWeights.resize(meshInfo.vertices.size());
//	LoadAnimationData(mesh, &meshInfo);
//}

void FBXLoader::LoadMaterial(FbxSurfaceMaterial* surfaceMaterial)
{
	FbxMaterialInfo material{};

	material.name = s2ws(surfaceMaterial->GetName());

	material.diffuse = GetMaterialData(surfaceMaterial, FbxSurfaceMaterial::sDiffuse, FbxSurfaceMaterial::sDiffuseFactor);
	material.ambient = GetMaterialData(surfaceMaterial, FbxSurfaceMaterial::sAmbient, FbxSurfaceMaterial::sAmbientFactor);
	material.specular = GetMaterialData(surfaceMaterial, FbxSurfaceMaterial::sSpecular, FbxSurfaceMaterial::sSpecularFactor);

	material.diffuseTexName = GetTextureRelativeName(surfaceMaterial, FbxSurfaceMaterial::sDiffuse);
	material.normalTexName = GetTextureRelativeName(surfaceMaterial, FbxSurfaceMaterial::sNormalMap);
	material.specularTexName = GetTextureRelativeName(surfaceMaterial, FbxSurfaceMaterial::sSpecular);

	_meshes.back().materials.push_back(material);
}

void FBXLoader::GetNormal(FbxMesh* mesh, FbxMeshInfo* container, int32_t idx, int32_t vertexCounter)
{
	if (mesh->GetElementNormalCount() == 0)
		return;

	FbxGeometryElementNormal* normal = mesh->GetElementNormal();
	uint32 normalIdx = 0;

	if (normal->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
	{
		if (normal->GetReferenceMode() == FbxGeometryElement::eDirect)
			normalIdx = vertexCounter;
		else
			normalIdx = normal->GetIndexArray().GetAt(vertexCounter);
	}
	else if (normal->GetMappingMode() == FbxGeometryElement::eByControlPoint)
	{
		if (normal->GetReferenceMode() == FbxGeometryElement::eDirect)
			normalIdx = idx;
		else
			normalIdx = normal->GetIndexArray().GetAt(idx);
	}

	FbxVector4 vec = normal->GetDirectArray().GetAt(normalIdx);
	container->vertices[idx].normal.x = static_cast<float>(vec.mData[0]);
	container->vertices[idx].normal.y = static_cast<float>(vec.mData[2]);
	container->vertices[idx].normal.z = static_cast<float>(vec.mData[1]);
}

void FBXLoader::GetTangent(FbxMesh* mesh, FbxMeshInfo* meshInfo, int32_t idx, int32_t vertexCounter)
{
	if (mesh->GetElementTangentCount() == 0) {
		// TEMP : 원래는 이런 저런 알고리즘으로 Tangent 만들어줘야 함
		meshInfo->vertices[idx].tangent.x = 1.f;
		meshInfo->vertices[idx].tangent.y = 0.f;
		meshInfo->vertices[idx].tangent.z = 0.f;
		return;
	}

	FbxGeometryElementTangent* tangent = mesh->GetElementTangent();
	uint32 tangentIdx = 0;

	if (tangent->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
	{
		if (tangent->GetReferenceMode() == FbxGeometryElement::eDirect)
			tangentIdx = vertexCounter;
		else
			tangentIdx = tangent->GetIndexArray().GetAt(vertexCounter);
	}
	else if (tangent->GetMappingMode() == FbxGeometryElement::eByControlPoint)
	{
		if (tangent->GetReferenceMode() == FbxGeometryElement::eDirect)
			tangentIdx = idx;
		else
			tangentIdx = tangent->GetIndexArray().GetAt(idx);
	}

	FbxVector4 vec = tangent->GetDirectArray().GetAt(tangentIdx);
	meshInfo->vertices[idx].tangent.x = static_cast<float>(vec.mData[0]);
	meshInfo->vertices[idx].tangent.y = static_cast<float>(vec.mData[2]);
	meshInfo->vertices[idx].tangent.z = static_cast<float>(vec.mData[1]);
}

void FBXLoader::GetUV(FbxMesh* mesh, FbxMeshInfo* meshInfo, int32_t idx, int32_t uvIndex)
{
	FbxLayerElementUV* pFbxLayerElementUV = mesh->GetLayer(0)->GetUVs();
	if (pFbxLayerElementUV == nullptr) {
		return;
	}

	Vec2 uv;
	switch (pFbxLayerElementUV->GetMappingMode()) {
		case FbxLayerElementUV::eByControlPoint: {
			switch (pFbxLayerElementUV->GetReferenceMode()) {
				case FbxLayerElementUV::eDirect: {
					fbxsdk::FbxVector2 fbxUv = pFbxLayerElementUV->GetDirectArray().GetAt(idx);
					uv.x = fbxUv.mData[0];
					uv.y = fbxUv.mData[1];
					break;
				}

				case FbxLayerElementUV::eIndexToDirect: {
					int id = pFbxLayerElementUV->GetIndexArray().GetAt(idx);
					fbxsdk::FbxVector2 fbxUv = pFbxLayerElementUV->GetDirectArray().GetAt(id);
					uv.x = fbxUv.mData[0];
					uv.y = fbxUv.mData[1];
					break;
				}
			}
			break;
		}

		case FbxLayerElementUV::eByPolygonVertex: {
			switch (pFbxLayerElementUV->GetReferenceMode()) {
				// Always enters this part for the example model
				case FbxLayerElementUV::eDirect:
				case FbxLayerElementUV::eIndexToDirect: {
					uv.x = pFbxLayerElementUV->GetDirectArray().GetAt(uvIndex).mData[0];
					uv.y = pFbxLayerElementUV->GetDirectArray().GetAt(uvIndex).mData[1];
					break;
				}
			}
			break;
		}
	}

	if (meshInfo->vertices[idx].uv.x == 0.0f && meshInfo->vertices[idx].uv.y == 0.0f) {
		meshInfo->vertices[idx].uv.x = uv.x;
		meshInfo->vertices[idx].uv.y = 1.f - uv.y;
	}
}

Vec4 FBXLoader::GetMaterialData(FbxSurfaceMaterial* surface, const char* materialName, const char* factorName)
{
	FbxDouble3  material;
	FbxDouble	factor = 0.f;

	FbxProperty materialProperty = surface->FindProperty(materialName);
	FbxProperty factorProperty = surface->FindProperty(factorName);

	if (materialProperty.IsValid() && factorProperty.IsValid())
	{
		material = materialProperty.Get<FbxDouble3>();
		factor = factorProperty.Get<FbxDouble>();
	}

	Vec4 ret = Vec4(
		static_cast<float>(material.mData[0] * factor),
		static_cast<float>(material.mData[1] * factor),
		static_cast<float>(material.mData[2] * factor),
		static_cast<float>(factor));

	return ret;
}

wstring FBXLoader::GetTextureRelativeName(FbxSurfaceMaterial* surface, const char* materialProperty)
{
	string name;

	FbxProperty textureProperty = surface->FindProperty(materialProperty);
	if (textureProperty.IsValid())
	{
		uint32 count = textureProperty.GetSrcObjectCount();

		if (1 <= count)
		{
			FbxFileTexture* texture = textureProperty.GetSrcObject<FbxFileTexture>(0);
			if (texture)
				name = texture->GetRelativeFileName();
		}
	}

	return s2ws(name);
}

void FBXLoader::CreateTextures()
{
	for (size_t i = 0; i < _meshes.size(); i++)
	{
		for (size_t j = 0; j < _meshes[i].materials.size(); j++)
		{
			// DiffuseTexture
			{
				wstring relativePath = _meshes[i].materials[j].diffuseTexName.c_str();
				wstring filename = fs::path(relativePath).filename();
				wstring fullPath = _resourceDirectory + L"\\" + filename;
				if (filename.empty() == false)
					GET_SINGLE(Resources)->Load<Texture>(filename, fullPath);
			}

			// NormalTexture
			{
				wstring relativePath = _meshes[i].materials[j].normalTexName.c_str();
				wstring filename = fs::path(relativePath).filename();
				wstring fullPath = _resourceDirectory + L"\\" + filename;
				if (filename.empty() == false)
					GET_SINGLE(Resources)->Load<Texture>(filename, fullPath);
			}

			// SpecularTexture
			{
				wstring relativePath = _meshes[i].materials[j].specularTexName.c_str();
				wstring filename = fs::path(relativePath).filename();
				wstring fullPath = _resourceDirectory + L"\\" + filename;
				if (filename.empty() == false)
					GET_SINGLE(Resources)->Load<Texture>(filename, fullPath);
			}
		}
	}
}

void FBXLoader::CreateMaterials()
{
	for (size_t i = 0; i < _meshes.size(); i++)
	{
		for (size_t j = 0; j < _meshes[i].materials.size(); j++)
		{
			shared_ptr<Material> material = make_shared<Material>();
			wstring key = _meshes[i].materials[j].name;
			material->SetName(key);
			material->SetShader(GET_SINGLE(Resources)->Get<Shader>(L"Deferred"));

			{
				wstring diffuseName = _meshes[i].materials[j].diffuseTexName.c_str();
				wstring filename = fs::path(diffuseName).filename();
				wstring key = filename;
				shared_ptr<Texture> diffuseTexture = GET_SINGLE(Resources)->Get<Texture>(key);
				if (diffuseTexture)
					material->SetTexture(0, diffuseTexture);
			}

			{
				wstring normalName = _meshes[i].materials[j].normalTexName.c_str();
				wstring filename = fs::path(normalName).filename();
				wstring key = filename;
				shared_ptr<Texture> normalTexture = GET_SINGLE(Resources)->Get<Texture>(key);
				if (normalTexture)
					material->SetTexture(1, normalTexture);
			}

			{
				wstring specularName = _meshes[i].materials[j].specularTexName.c_str();
				wstring filename = fs::path(specularName).filename();
				wstring key = filename;
				shared_ptr<Texture> specularTexture = GET_SINGLE(Resources)->Get<Texture>(key);
				if (specularTexture)
					material->SetTexture(2, specularTexture);
			}

			GET_SINGLE(Resources)->Add<Material>(material->GetName(), material);
		}
	}
}

void FBXLoader::LoadAnimationData(FbxMesh* mesh, FbxMeshInfo* meshInfo)
{
	const int32 skinCount = mesh->GetDeformerCount(FbxDeformer::eSkin);
	if (skinCount <= 0 || _animClips.empty())
		return;

	meshInfo->hasAnimation = true;

	for (int32 i = 0; i < skinCount; i++)
	{
		FbxSkin* fbxSkin = static_cast<FbxSkin*>(mesh->GetDeformer(i, FbxDeformer::eSkin));

		if (fbxSkin)
		{
			FbxSkin::EType type = fbxSkin->GetSkinningType();
			if (FbxSkin::eRigid == type || FbxSkin::eLinear)
			{
				const int32 clusterCount = fbxSkin->GetClusterCount();
				for (int32 j = 0; j < clusterCount; j++)
				{
					FbxCluster* cluster = fbxSkin->GetCluster(j);
					if (cluster->GetLink() == nullptr)
						continue;

					int32 boneIdx = FindBoneIndex(cluster->GetLink()->GetName());
					assert(boneIdx >= 0);

					FbxAMatrix matNodeTransform = GetTransform(mesh->GetNode());
					LoadBoneWeight(cluster, boneIdx, meshInfo);
					LoadOffsetMatrix(cluster, matNodeTransform, boneIdx, meshInfo);

					const int32 animCount = _animNames.Size();
					for (int32 k = 0; k < animCount; k++)
						LoadKeyframe(k, mesh->GetNode(), cluster, matNodeTransform, boneIdx, meshInfo);
				}
			}
		}
	}

	FillBoneWeight(mesh, meshInfo);
}

void FBXLoader::FillBoneWeight(FbxMesh* mesh, FbxMeshInfo* meshInfo)
{
	const int32 size = static_cast<int32>(meshInfo->boneWeights.size());
	for (int32 v = 0; v < size; v++)
	{
		BoneWeight& boneWeight = meshInfo->boneWeights[v];
		boneWeight.Normalize();

		float animBoneIndex[4] = {};
		float animBoneWeight[4] = {};

		const int32 weightCount = static_cast<int32>(boneWeight.boneWeights.size());
		for (int32 w = 0; w < weightCount; w++)
		{
			animBoneIndex[w] = static_cast<float>(boneWeight.boneWeights[w].first);
			animBoneWeight[w] = static_cast<float>(boneWeight.boneWeights[w].second);
		}

		memcpy(&meshInfo->vertices[v].indices, animBoneIndex, sizeof(Vec4));
		memcpy(&meshInfo->vertices[v].weights, animBoneWeight, sizeof(Vec4));
	}
}

void FBXLoader::LoadBoneWeight(FbxCluster* cluster, int32_t boneIdx, FbxMeshInfo* meshInfo)
{
	const int32 indicesCount = cluster->GetControlPointIndicesCount();
	for (int32 i = 0; i < indicesCount; i++)
	{
		double weight = cluster->GetControlPointWeights()[i];
		int32 vtxIdx = cluster->GetControlPointIndices()[i];
		meshInfo->boneWeights[vtxIdx].AddWeights(boneIdx, weight);
	}
}

void FBXLoader::LoadOffsetMatrix(FbxCluster* cluster, const FbxAMatrix& matNodeTransform, int32_t boneIdx, FbxMeshInfo* meshInfo)
{
	FbxAMatrix matClusterTrans;
	FbxAMatrix matClusterLinkTrans;
	// The transformation of the mesh at binding time 
	cluster->GetTransformMatrix(matClusterTrans);
	// The transformation of the cluster(joint) at binding time from joint space to world space 
	cluster->GetTransformLinkMatrix(matClusterLinkTrans);

	FbxVector4 V0 = { 1, 0, 0, 0 };
	FbxVector4 V1 = { 0, 0, 1, 0 };
	FbxVector4 V2 = { 0, 1, 0, 0 };
	FbxVector4 V3 = { 0, 0, 0, 1 };

	FbxAMatrix matReflect;
	matReflect[0] = V0;
	matReflect[1] = V1;
	matReflect[2] = V2;
	matReflect[3] = V3;

	FbxAMatrix matOffset;
	matOffset = matClusterLinkTrans.Inverse() * matClusterTrans;
	matOffset = matReflect * matOffset * matReflect;

	_bones[boneIdx]->matOffset = matOffset.Transpose();
}

void FBXLoader::LoadKeyframe(int32_t animIndex, FbxNode* node, FbxCluster* cluster, const FbxAMatrix& matNodeTransform, int32_t boneIdx, FbxMeshInfo* meshInfo)
{
	if (_animClips.empty())
		return;

	FbxVector4	v1 = { 1, 0, 0, 0 };
	FbxVector4	v2 = { 0, 0, 1, 0 };
	FbxVector4	v3 = { 0, 1, 0, 0 };
	FbxVector4	v4 = { 0, 0, 0, 1 };
	FbxAMatrix	matReflect;
	matReflect.mData[0] = v1;
	matReflect.mData[1] = v2;
	matReflect.mData[2] = v3;
	matReflect.mData[3] = v4;

	FbxTime::EMode timeMode = _scene->GetGlobalSettings().GetTimeMode();

	// 애니메이션 골라줌
	FbxAnimStack* animStack = _scene->FindMember<FbxAnimStack>(_animNames[animIndex]->Buffer());
	_scene->SetCurrentAnimationStack(OUT animStack);

	FbxLongLong startFrame = _animClips[animIndex]->startTime.GetFrameCount(timeMode);
	FbxLongLong endFrame = _animClips[animIndex]->endTime.GetFrameCount(timeMode);

	for (FbxLongLong frame = startFrame; frame < endFrame; frame++)
	{
		FbxKeyFrameInfo keyFrameInfo = {};
		FbxTime fbxTime = 0;

		fbxTime.SetFrame(frame, timeMode);

		FbxAMatrix matFromNode = node->EvaluateGlobalTransform(fbxTime);
		FbxAMatrix matTransform = matFromNode.Inverse() * cluster->GetLink()->EvaluateGlobalTransform(fbxTime);
		matTransform = matReflect * matTransform * matReflect;

		keyFrameInfo.time = fbxTime.GetSecondDouble();
		keyFrameInfo.matTransform = matTransform;

		_animClips[animIndex]->keyFrames[boneIdx].push_back(keyFrameInfo);
	}
}

int32_t FBXLoader::FindBoneIndex(const string& name)
{
	wstring boneName = wstring(name.begin(), name.end());

	for (UINT i = 0; i < _bones.size(); ++i)
	{
		if (_bones[i]->boneName == boneName)
			return i;
	}

	return -1;
}

FbxAMatrix FBXLoader::GetTransform(FbxNode* node)
{
	const FbxVector4 translation = node->GetGeometricTranslation(FbxNode::eSourcePivot);
	const FbxVector4 rotation = node->GetGeometricRotation(FbxNode::eSourcePivot);
	const FbxVector4 scaling = node->GetGeometricScaling(FbxNode::eSourcePivot);
	return FbxAMatrix(translation, rotation, scaling);
}
