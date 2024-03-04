#include "pch.h"
#include "MeshData.h"
#include "FBXLoader.h"
#include "Mesh.h"
#include "Material.h"
#include "Resources.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "Animator.h"
#include "StructuredBuffer.h"

MeshData::MeshData() : Object(OBJECT_TYPE::MESH_DATA)
{
}

MeshData::~MeshData()
{
}

shared_ptr<MeshData> MeshData::LoadFromFBX(const wstring& path)
{
	FBXLoader loader;
	loader.LoadFbx(path);

	shared_ptr<MeshData> meshData = make_shared<MeshData>();

	for (int32 i = 0; i < loader.GetMeshCount(); ++i) {
		shared_ptr<Mesh> mesh = Mesh::CreateFromFBX(&loader.GetMesh(i), loader);

		GET_SINGLE(Resources)->Add<Mesh>(mesh->GetName(), mesh);

		// Material 찾아서 연동
		vector<shared_ptr<Material>> materials;
		for (size_t j = 0; j < loader.GetMesh(i).materials.size(); ++j) {
			shared_ptr<Material> material = GET_SINGLE(Resources)->Get<Material>(loader.GetMesh(i).materials[j].name);
			materials.push_back(material);
		}

		MeshRenderInfo info = {};
		info.mesh = mesh;
		info.materials = materials;
		meshData->_meshRenders.push_back(info);
	}

	// 불러온 메쉬 데이터를 txt 파일로 저장하기 위한 경로
	wstring savePath{ path };
	savePath = savePath.substr(0, savePath.length() - 4) + L".meshdata";
	meshData->Save(savePath, loader);
	//meshData->SaveBinary(savePath, loader);

	return meshData;
}

void MeshData::Load(const wstring& _strFilePath)
{
	wifstream in{ _strFilePath };
	int meshCount = 0;
	in >> meshCount;

	for (int i = 0; i < meshCount; ++i) {
		MeshRenderInfo info = {};
		shared_ptr<Mesh> mesh = make_shared<Mesh>();

		wstring meshName;
		in >> meshName;
		mesh->SetName(meshName);

		int vertexCount = 0;
		in >> vertexCount;
		vector<Vertex> vertices;
		for (int j = 0; j < vertexCount; ++j) {
			Vertex vertex = {};
			in	>> vertex.pos.x		>> vertex.pos.y		>> vertex.pos.z
				>> vertex.uv.x		>> vertex.uv.y
				>> vertex.normal.x	>> vertex.normal.y	>> vertex.normal.z
				>> vertex.tangent.x >> vertex.tangent.y >> vertex.tangent.z
				>> vertex.weights.x >> vertex.weights.y >> vertex.weights.z >> vertex.weights.w
				>> vertex.indices.x >> vertex.indices.y >> vertex.indices.z >> vertex.indices.w;
			vertices.push_back(vertex);
		}

		vector<vector<uint32>> indices;
		int indexCount = 0;
		in >> indexCount;
		for (int j = 0; j < indexCount; ++j) {
			vector<uint32> index;
			int indexSize = 0;
			in >> indexSize;
			for (int k = 0; k < indexSize; ++k) {
				uint32 idx = 0;
				in >> idx;

				index.push_back(idx);
			}

			indices.push_back(index);
		}

		mesh->CreateFromMeshData(vertices, indices);

		bool hasAnimation = false;
		in >> hasAnimation;
		if (hasAnimation) {
			// Animation Clip 정보 기록
			AnimClipInfo info = {};
			int animClipCount = 0;
			in >> animClipCount;
			for (int j = 0; j < animClipCount; ++j) {
				// 이름, 길이, 프레임 수 기록
				in >> info.animName >> info.duration >> info.frameCount;

				// KeyFrame Count 기록
				int keyFrameCount = 0;
				in >> keyFrameCount;
				info.keyFrames.resize(keyFrameCount);

				// bone 정보 기록
				int boneCount = 0;
				in >> boneCount;
				for (int b = 0; b < boneCount; ++b) {
					vector<KeyFrameInfo>& keyFrames = info.keyFrames[b];
					int keyFrameInfoCount = 0;
					in >> keyFrameInfoCount;
					keyFrames.resize(keyFrameInfoCount);

					for (int f = 0; f < keyFrameInfoCount; ++f) {
						KeyFrameInfo& keyFrame = keyFrames[f];
						in	>> keyFrame.time		>> keyFrame.frame
							>> keyFrame.scale.x		>> keyFrame.scale.y		>> keyFrame.scale.z
							>> keyFrame.rotation.x	>> keyFrame.rotation.y	>> keyFrame.rotation.z >> keyFrame.rotation.w
							>> keyFrame.translate.x >> keyFrame.translate.y >> keyFrame.translate.z;
					}
				}
				mesh->AddAnimClip(info);
			}
			
			// Bones 정보 기록
			int boneCount = 0;
			in >> boneCount;
			for (int j = 0; j < boneCount; ++j) {
				BoneInfo bone = {};
				in >> bone.boneName >> bone.parentIdx;
				for (int k = 0; k < 4; ++k) {
					for (int l = 0; l < 4; ++l) {
						in >> bone.matOffset.m[k][l];
					}
				}
				mesh->AddBone(bone);
			}

			// SkinData 생성
			if (mesh->IsAnimMesh()) {
				// BoneOffet 행렬
				vector<Matrix> offsetVec(boneCount);
				for (size_t b = 0; b < boneCount; b++) {
					offsetVec[b] = mesh->GetBones()->at(b).matOffset;
				}

				// OffsetMatrix StructuredBuffer 세팅
				shared_ptr<StructuredBuffer> offsetBuffer = make_shared<StructuredBuffer>();
				offsetBuffer->Init(sizeof(Matrix), static_cast<uint32>(offsetVec.size()), offsetVec.data());
				mesh->SetBoneOffsetBuffer(offsetBuffer);

				const int32 animCount = static_cast<int32>(mesh->GetAnimClip()->size());
				for (int32 i = 0; i < animCount; i++) {
					AnimClipInfo& animClip = mesh->GetAnimClip()->at(i);

					// 애니메이션 프레임 정보
					vector<AnimFrameParams> frameParams;
					frameParams.resize(boneCount * animClip.frameCount);

					for (int32 b = 0; b < boneCount; b++) {
						const int32 keyFrameCount = static_cast<int32>(animClip.keyFrames[b].size());
						for (int32 f = 0; f < keyFrameCount; f++) {
							int32 idx = static_cast<int32>(boneCount * f + b);

							if (idx >= frameParams.size())
								frameParams.resize(idx + 1);

							frameParams[idx] = AnimFrameParams
							{
								Vec4(animClip.keyFrames[b][f].scale),
								animClip.keyFrames[b][f].rotation, // Quaternion
								Vec4(animClip.keyFrames[b][f].translate)
							};
						}
					}

					shared_ptr<StructuredBuffer> frameBuffer = make_shared<StructuredBuffer>();
					frameBuffer->Init(sizeof(AnimFrameParams), static_cast<uint32>(frameParams.size()), frameParams.data());
					mesh->AddBoneFrameDataBuffer(frameBuffer);
				}
			}
		}

		// meshdata 확장자 삭제
		wstring resourcePath = _strFilePath.substr(0, _strFilePath.length() - 8) + L"fbm";

		// Material 정보 기록
		vector<shared_ptr<Material>> materials;
		int materialCount = 0;
		in >> materialCount;
		for (int j = 0; j < materialCount; ++j) {
			wstring materialName;
			wstring diffuseTexName;
			wstring normalTexName;
			wstring specularTexName;
			bool hasDiffuseTex = false;
			bool hasNormalTex = false;
			bool hasSpecularTex = false;

			in >> materialName;
			in >> hasDiffuseTex;

			shared_ptr<Material> material = make_shared<Material>();
			material->SetName(materialName);
			material->SetShader(GET_SINGLE(Resources)->Get<Shader>(L"Deferred"));

			if (hasDiffuseTex) {
				in >> diffuseTexName;
				shared_ptr<Texture> diffuseTexture = GET_SINGLE(Resources)->Load<Texture>(diffuseTexName, resourcePath + L"\\" + diffuseTexName);
				material->SetTexture(0, diffuseTexture);
			}

			in >> hasNormalTex;
			if (hasNormalTex) {
				in >> normalTexName;
				shared_ptr<Texture> normalTexture = GET_SINGLE(Resources)->Load<Texture>(normalTexName, resourcePath + L"\\" + normalTexName);
				material->SetTexture(1, normalTexture);
			}

			in >> hasSpecularTex;
			if (hasSpecularTex) {
				in >> specularTexName;
				shared_ptr<Texture> specularTexture = GET_SINGLE(Resources)->Load<Texture>(specularTexName, resourcePath + L"\\" + specularTexName);
				material->SetTexture(2, specularTexture);
			}

			GET_SINGLE(Resources)->Add<Material>(material->GetName(), material);
			materials.push_back(material);
		}

		info.mesh = mesh;
		info.materials = materials;
		_meshRenders.push_back(info);
	}
}

void MeshData::Save(const wstring& path, FBXLoader& loader)
{
	wofstream out{ path };
	int meshCount = loader.GetMeshCount();

	// meshCount 기록
	out << meshCount << " ";
	for (int i = 0; i < meshCount; ++i) {
		// Mesh 정보 기록
		if (loader.GetMesh(i).name.empty()) {
			out << path << i << " ";
		}
		else {
			wstring meshName = loader.GetMesh(i).name;
			// 이름이 공백이 있으면 안되니까 공백을 _로 바꿔준다.
			replace(meshName.begin(), meshName.end(), L' ', L'_');
			out << meshName << " ";
		}

		// Vertex 정보 기록
		int vertexCount = loader.GetMesh(i).vertices.size();
		out << vertexCount << " ";
		for (const Vertex& vertex : loader.GetMesh(i).vertices) {
			out << vertex.pos.x		<< " " << vertex.pos.y		<< " " << vertex.pos.z		<< " "
				<< vertex.uv.x		<< " " << vertex.uv.y		<< " "
				<< vertex.normal.x	<< " " << vertex.normal.y	<< " " << vertex.normal.z	<< " "
				<< vertex.tangent.x << " " << vertex.tangent.y	<< " " << vertex.tangent.z	<< " "
				<< vertex.weights.x << " " << vertex.weights.y	<< " " << vertex.weights.z	<< " " << vertex.weights.w << " "
				<< vertex.indices.x << " " << vertex.indices.y	<< " " << vertex.indices.z	<< " " << vertex.indices.w << " ";
		}

		// Index 정보 기록
		int indexCount = loader.GetMesh(i).indices.size();
		out << indexCount << " ";
		for (const vector<uint32>& index : loader.GetMesh(i).indices) {
			int indexSize = static_cast<int>(index.size());
			out << indexSize << " ";
			for (uint32 idx : index) {
				out << idx << " ";
			}
		}

		// Animation 정보 기록
		bool hasAnimation = loader.GetMesh(i).hasAnimation;
		out << hasAnimation << " ";
		if (hasAnimation) {
			// Animation Clip 정보 기록
			vector<shared_ptr<FbxAnimClipInfo>>& animClips = loader.GetAnimClip();
			int animClipCount = static_cast<int>(animClips.size());
			out << animClipCount << " ";
			for (shared_ptr<FbxAnimClipInfo>& animClip : animClips) {
				// 이름, 길이, 프레임 수 기록
				double duration = animClip->endTime.GetSecondDouble() - animClip->startTime.GetSecondDouble();
				int frameCount = static_cast<int>(animClip->endTime.GetFrameCount(animClip->mode)) - static_cast<int>(animClip->startTime.GetFrameCount(animClip->mode));
				wstring animName = animClip->name;
				replace(animName.begin(), animName.end(), L' ', L'_');
				out << animName << " " << duration << " " << frameCount << " ";

				// KeyFrame Count 기록
				int keyFrameCount = static_cast<int>(animClip->keyFrames.size());
				out << keyFrameCount << " ";

				// bone 정보 기록
				int boneCount = static_cast<int>(animClip->keyFrames.size());
				out << boneCount << " ";
				for (int b = 0; b < boneCount; ++b) {
					vector<FbxKeyFrameInfo>& keyFrames = animClip->keyFrames[b];
					int keyFrameInfoCount = static_cast<int>(keyFrames.size());
					out << keyFrameInfoCount << " ";
					for (int f = 0; f < keyFrameInfoCount; ++f) {
						FbxKeyFrameInfo& keyFrame = keyFrames[f];
						out << keyFrame.time << " " 
							<< keyFrameInfoCount << " "
							<< static_cast<float>(keyFrame.matTransform.GetS().mData[0]) << " " 
							<< static_cast<float>(keyFrame.matTransform.GetS().mData[1]) << " " 
							<< static_cast<float>(keyFrame.matTransform.GetS().mData[2]) << " "

							<< static_cast<float>(keyFrame.matTransform.GetQ().mData[0]) << " " 
							<< static_cast<float>(keyFrame.matTransform.GetQ().mData[1]) << " " 
							<< static_cast<float>(keyFrame.matTransform.GetQ().mData[2]) << " " 
							<< static_cast<float>(keyFrame.matTransform.GetQ().mData[3]) << " "

							<< static_cast<float>(keyFrame.matTransform.GetT().mData[0]) << " " 
							<< static_cast<float>(keyFrame.matTransform.GetT().mData[1]) << " " 
							<< static_cast<float>(keyFrame.matTransform.GetT().mData[2]) << " ";
					}
				}
			}

			// Bones 정보 기록
			vector<shared_ptr<FbxBoneInfo>>& bones = loader.GetBones();
			int boneCount = static_cast<int>(bones.size());
			out << boneCount << " ";
			for (shared_ptr<FbxBoneInfo>& bone : bones) {
				wstring boneName = bone->boneName;
				replace(boneName.begin(), boneName.end(), L' ', L'_');
				out << boneName << " " << bone->parentIndex << " ";
				for (int i = 0; i < 4; ++i) {
					for (int j = 0; j < 4; ++j) {
						out << static_cast<float>(bone->matOffset.Get(i, j)) << " ";
					}
				}
			}
		}

		// Material 정보 기록
		const vector<FbxMaterialInfo>& materials = loader.GetMesh(i).materials;
		int materialCount = static_cast<int>(materials.size());
		out << materialCount << " ";
		for (const FbxMaterialInfo& material : materials) {
			wstring diffuseTexName	= fs::path(material.diffuseTexName).filename();
			wstring normalTexName	= fs::path(material.normalTexName).filename();
			wstring specularTexName = fs::path(material.specularTexName).filename();

			wstring materialName = material.name;

			replace(materialName.begin(), materialName.end(), L' ', L'_');
			replace(diffuseTexName.begin(), diffuseTexName.end(), L' ', L'_');
			replace(normalTexName.begin(), normalTexName.end(), L' ', L'_');
			replace(specularTexName.begin(), specularTexName.end(), L' ', L'_');

			bool hasDiffuseTex	= !diffuseTexName.empty();
			bool hasNormalTex	= !normalTexName.empty();
			bool hasSpecularTex = !specularTexName.empty();

			if (hasDiffuseTex)
				out << materialName << " " << "1" << " " << diffuseTexName << " ";
			else
				out << materialName << " " << "0" << " ";

			if (hasNormalTex)
				out << "1" << " " << normalTexName << " ";
			else
				out << "0" << " ";

			if (hasSpecularTex)
				out << "1" << " " << specularTexName << " ";
			else
				out << "0" << " ";
		}
	}
}

void MeshData::SaveBinary(const std::wstring& path, FBXLoader& loader) 
{
	std::ofstream out(path, std::ios::binary); // 이진 파일로 열기

	if (!out.is_open()) {
		throw std::runtime_error("File cannot be opened for writing.");
	}

	int meshCount = loader.GetMeshCount();
	out.write(reinterpret_cast<char*>(&meshCount), sizeof(meshCount)); // meshCount 기록

	for (int i = 0; i < meshCount; ++i) {
		// Mesh 정보 기록
		auto& mesh = loader.GetMesh(i);
		std::wstring meshName = mesh.name.empty() ? L"default_mesh" : mesh.name;
		// 공백을 '_'로 바꿈
		std::replace(meshName.begin(), meshName.end(), L' ', L'_');

		size_t nameLength = meshName.size();
		out.write(reinterpret_cast<char*>(&nameLength), sizeof(nameLength)); // 이름 길이 기록
		out.write(reinterpret_cast<char*>(meshName.data()), nameLength * sizeof(wchar_t)); // 이름 기록

		// Vertex 정보 기록
		int vertexCount = static_cast<int>(mesh.vertices.size());
		out.write(reinterpret_cast<char*>(&vertexCount), sizeof(vertexCount)); // vertexCount 기록
		for (const Vertex& vertex : mesh.vertices) {
			out.write(reinterpret_cast<const char*>(&vertex), sizeof(Vertex)); // Vertex 정보 기록
		}

		// Index 정보 기록
		int indexCount = static_cast<int>(mesh.indices.size());
		out.write(reinterpret_cast<char*>(&indexCount), sizeof(indexCount)); // indexCount 기록
		for (const auto& index : mesh.indices) {
			int indexSize = static_cast<int>(index.size());
			out.write(reinterpret_cast<char*>(&indexSize), sizeof(indexSize)); // indexSize 기록
			for (uint32_t idx : index) {
				out.write(reinterpret_cast<char*>(&idx), sizeof(idx)); // 인덱스 기록
			}
		}

		// Animation 정보는 상당히 복잡하므로, 각 단계에서 필요한 정보를 정확한 크기와 함께 이진 형식으로 기록해야 합니다.
		// 예를 들어, Animation Clip 정보, KeyFrame Count, Bone 정보 등을 적절한 형식으로 변환하여 저장해야 합니다.
		// 이 예시는 구체적인 Animation 데이터 구조를 알지 못하기 때문에, Animation 데이터 저장 방법에 대한 구체적인 코드는 생략합니다.

		// Material 정보 기록
		int materialCount = static_cast<int>(mesh.materials.size());
		out.write(reinterpret_cast<char*>(&materialCount), sizeof(materialCount)); // materialCount 기록
		for (const FbxMaterialInfo& material : mesh.materials) {
			// Material 이름과 텍스처 파일 이름들을 기록하는 방식을 반복합니다.
			// 여기서도 이름 길이와 데이터를 순서대로 기록합니다.
		}

		// 파일 작업을 완료한 후에는 파일을 닫아야 합니다. 하지만 std::ofstream 객체는 소멸자에서 자동으로 파일을 닫기 때문에
		// 여기서 명시적으로 파일을 닫을 필요는 없습니다.
	}
}

vector<shared_ptr<GameObject>> MeshData::Instantiate()
{
	vector<shared_ptr<GameObject>> v;

	for (MeshRenderInfo& info : _meshRenders)
	{
		shared_ptr<GameObject> gameObject = make_shared<GameObject>();
		gameObject->AddComponent(make_shared<Transform>());
		gameObject->AddComponent(make_shared<MeshRenderer>());
		gameObject->GetMeshRenderer()->SetMesh(info.mesh);

		for (uint32 i = 0; i < info.materials.size(); i++)
			gameObject->GetMeshRenderer()->SetMaterial(info.materials[i], i);

		if (info.mesh->IsAnimMesh())
		{
			shared_ptr<Animator> animator = make_shared<Animator>();
			gameObject->AddComponent(animator);
			animator->SetBones(info.mesh->GetBones());
			animator->SetAnimClip(info.mesh->GetAnimClip());
		}

		v.push_back(gameObject);
	}


	return v;
}

