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

		// Material ã�Ƽ� ����
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

	// �ҷ��� �޽� �����͸� txt ���Ϸ� �����ϱ� ���� ���
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
			// Animation Clip ���� ���
			AnimClipInfo info = {};
			int animClipCount = 0;
			in >> animClipCount;
			for (int j = 0; j < animClipCount; ++j) {
				// �̸�, ����, ������ �� ���
				in >> info.animName >> info.duration >> info.frameCount;

				// KeyFrame Count ���
				int keyFrameCount = 0;
				in >> keyFrameCount;
				info.keyFrames.resize(keyFrameCount);

				// bone ���� ���
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
			
			// Bones ���� ���
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

			// SkinData ����
			if (mesh->IsAnimMesh()) {
				// BoneOffet ���
				vector<Matrix> offsetVec(boneCount);
				for (size_t b = 0; b < boneCount; b++) {
					offsetVec[b] = mesh->GetBones()->at(b).matOffset;
				}

				// OffsetMatrix StructuredBuffer ����
				shared_ptr<StructuredBuffer> offsetBuffer = make_shared<StructuredBuffer>();
				offsetBuffer->Init(sizeof(Matrix), static_cast<uint32>(offsetVec.size()), offsetVec.data());
				mesh->SetBoneOffsetBuffer(offsetBuffer);

				const int32 animCount = static_cast<int32>(mesh->GetAnimClip()->size());
				for (int32 i = 0; i < animCount; i++) {
					AnimClipInfo& animClip = mesh->GetAnimClip()->at(i);

					// �ִϸ��̼� ������ ����
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

		// meshdata Ȯ���� ����
		wstring resourcePath = _strFilePath.substr(0, _strFilePath.length() - 8) + L"fbm";

		// Material ���� ���
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

	// meshCount ���
	out << meshCount << " ";
	for (int i = 0; i < meshCount; ++i) {
		// Mesh ���� ���
		if (loader.GetMesh(i).name.empty()) {
			out << path << i << " ";
		}
		else {
			wstring meshName = loader.GetMesh(i).name;
			// �̸��� ������ ������ �ȵǴϱ� ������ _�� �ٲ��ش�.
			replace(meshName.begin(), meshName.end(), L' ', L'_');
			out << meshName << " ";
		}

		// Vertex ���� ���
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

		// Index ���� ���
		int indexCount = loader.GetMesh(i).indices.size();
		out << indexCount << " ";
		for (const vector<uint32>& index : loader.GetMesh(i).indices) {
			int indexSize = static_cast<int>(index.size());
			out << indexSize << " ";
			for (uint32 idx : index) {
				out << idx << " ";
			}
		}

		// Animation ���� ���
		bool hasAnimation = loader.GetMesh(i).hasAnimation;
		out << hasAnimation << " ";
		if (hasAnimation) {
			// Animation Clip ���� ���
			vector<shared_ptr<FbxAnimClipInfo>>& animClips = loader.GetAnimClip();
			int animClipCount = static_cast<int>(animClips.size());
			out << animClipCount << " ";
			for (shared_ptr<FbxAnimClipInfo>& animClip : animClips) {
				// �̸�, ����, ������ �� ���
				double duration = animClip->endTime.GetSecondDouble() - animClip->startTime.GetSecondDouble();
				int frameCount = static_cast<int>(animClip->endTime.GetFrameCount(animClip->mode)) - static_cast<int>(animClip->startTime.GetFrameCount(animClip->mode));
				wstring animName = animClip->name;
				replace(animName.begin(), animName.end(), L' ', L'_');
				out << animName << " " << duration << " " << frameCount << " ";

				// KeyFrame Count ���
				int keyFrameCount = static_cast<int>(animClip->keyFrames.size());
				out << keyFrameCount << " ";

				// bone ���� ���
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

			// Bones ���� ���
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

		// Material ���� ���
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
	std::ofstream out(path, std::ios::binary); // ���� ���Ϸ� ����

	if (!out.is_open()) {
		throw std::runtime_error("File cannot be opened for writing.");
	}

	int meshCount = loader.GetMeshCount();
	out.write(reinterpret_cast<char*>(&meshCount), sizeof(meshCount)); // meshCount ���

	for (int i = 0; i < meshCount; ++i) {
		// Mesh ���� ���
		auto& mesh = loader.GetMesh(i);
		std::wstring meshName = mesh.name.empty() ? L"default_mesh" : mesh.name;
		// ������ '_'�� �ٲ�
		std::replace(meshName.begin(), meshName.end(), L' ', L'_');

		size_t nameLength = meshName.size();
		out.write(reinterpret_cast<char*>(&nameLength), sizeof(nameLength)); // �̸� ���� ���
		out.write(reinterpret_cast<char*>(meshName.data()), nameLength * sizeof(wchar_t)); // �̸� ���

		// Vertex ���� ���
		int vertexCount = static_cast<int>(mesh.vertices.size());
		out.write(reinterpret_cast<char*>(&vertexCount), sizeof(vertexCount)); // vertexCount ���
		for (const Vertex& vertex : mesh.vertices) {
			out.write(reinterpret_cast<const char*>(&vertex), sizeof(Vertex)); // Vertex ���� ���
		}

		// Index ���� ���
		int indexCount = static_cast<int>(mesh.indices.size());
		out.write(reinterpret_cast<char*>(&indexCount), sizeof(indexCount)); // indexCount ���
		for (const auto& index : mesh.indices) {
			int indexSize = static_cast<int>(index.size());
			out.write(reinterpret_cast<char*>(&indexSize), sizeof(indexSize)); // indexSize ���
			for (uint32_t idx : index) {
				out.write(reinterpret_cast<char*>(&idx), sizeof(idx)); // �ε��� ���
			}
		}

		// Animation ������ ����� �����ϹǷ�, �� �ܰ迡�� �ʿ��� ������ ��Ȯ�� ũ��� �Բ� ���� �������� ����ؾ� �մϴ�.
		// ���� ���, Animation Clip ����, KeyFrame Count, Bone ���� ���� ������ �������� ��ȯ�Ͽ� �����ؾ� �մϴ�.
		// �� ���ô� ��ü���� Animation ������ ������ ���� ���ϱ� ������, Animation ������ ���� ����� ���� ��ü���� �ڵ�� �����մϴ�.

		// Material ���� ���
		int materialCount = static_cast<int>(mesh.materials.size());
		out.write(reinterpret_cast<char*>(&materialCount), sizeof(materialCount)); // materialCount ���
		for (const FbxMaterialInfo& material : mesh.materials) {
			// Material �̸��� �ؽ�ó ���� �̸����� ����ϴ� ����� �ݺ��մϴ�.
			// ���⼭�� �̸� ���̿� �����͸� ������� ����մϴ�.
		}

		// ���� �۾��� �Ϸ��� �Ŀ��� ������ �ݾƾ� �մϴ�. ������ std::ofstream ��ü�� �Ҹ��ڿ��� �ڵ����� ������ �ݱ� ������
		// ���⼭ ��������� ������ ���� �ʿ�� �����ϴ�.
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

