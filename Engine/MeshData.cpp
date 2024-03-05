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

	wstring savePath{ path };
	savePath = savePath.substr(0, savePath.length() - 4) + L".meshdata";
	meshData->Save(savePath, loader);

	return meshData;
}

void MeshData::Load(const wstring& _strFilePath)
{
	ifstream in{ _strFilePath, ios::binary };

	if (!in.is_open()) {
		throw runtime_error("File cannot be opened for reading.");
	}

	int meshCount = 0;
	in.read(reinterpret_cast<char*>(&meshCount), sizeof(meshCount)); // meshCount 읽기

	for (int i = 0; i < meshCount; ++i) {
		// Mesh 정보 읽기
		MeshRenderInfo info = {};
		size_t nameLength = 0;
		shared_ptr<Mesh> mesh = make_shared<Mesh>();

		in.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength)); // 이름 길이 읽기
		wstring meshName;
		meshName.resize(nameLength);
		in.read(reinterpret_cast<char*>(meshName.data()), nameLength * sizeof(wchar_t)); // 이름 읽기
		mesh->SetName(meshName);

		// Vertex 정보 읽기
		int vertexCount = 0;
		in.read(reinterpret_cast<char*>(&vertexCount), sizeof(vertexCount));
		vector<Vertex> vertices;
		vertices.resize(vertexCount);
		in.read(reinterpret_cast<char*>(vertices.data()), vertexCount * sizeof(Vertex));

		// Index 정보 읽기
		int indexCount = 0;
		in.read(reinterpret_cast<char*>(&indexCount), sizeof(indexCount));
		vector<vector<uint32_t>> indices;
		indices.resize(indexCount);
		for (int j = 0; j < indexCount; ++j) {
			int indexSize = 0;
			in.read(reinterpret_cast<char*>(&indexSize), sizeof(indexSize));
			indices[j].resize(indexSize);
			in.read(reinterpret_cast<char*>(indices[j].data()), indexSize * sizeof(uint32_t));
		}
		mesh->CreateFromMeshData(vertices, indices);

		// Animation 정보 읽기
		bool hasAnimation = false;
		in.read(reinterpret_cast<char*>(&hasAnimation), sizeof(hasAnimation));
		if (hasAnimation) {
			// Animation Clip 정보 읽기
			AnimClipInfo info = {};
			int animClipCount = 0;
			in.read(reinterpret_cast<char*>(&animClipCount), sizeof(animClipCount));

			for (int j = 0; j < animClipCount; ++j) {
				// 이름, 길이, 프레임 수 읽기
				size_t nameLength = 0;
				wstring animName;
				double duration = 0.0;
				int frameCount = 0;

				in.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
				animName.resize(nameLength);
				in.read(reinterpret_cast<char*>(animName.data()), nameLength * sizeof(wchar_t));
				in.read(reinterpret_cast<char*>(&duration), sizeof(duration));
				in.read(reinterpret_cast<char*>(&frameCount), sizeof(frameCount));

				info.animName = animName;
				info.duration = duration;
				info.frameCount = frameCount;

				// KeyFrame Count 읽기
				int keyFrameCount = 0;
				in.read(reinterpret_cast<char*>(&keyFrameCount), sizeof(keyFrameCount));
				info.keyFrames.resize(keyFrameCount);

				// bone 정보 읽기
				int boneCount = 0;
				in.read(reinterpret_cast<char*>(&boneCount), sizeof(boneCount));
				for (int b = 0; b < boneCount; ++b) {
					vector<KeyFrameInfo>& keyFrames = info.keyFrames[b];
					int keyFrameInfoCount = 0;
					in.read(reinterpret_cast<char*>(&keyFrameInfoCount), sizeof(keyFrameInfoCount));
					keyFrames.resize(keyFrameInfoCount);

					for (int f = 0; f < keyFrameInfoCount; ++f) {
						KeyFrameInfo& keyFrame = keyFrames[f];
						FbxDouble sx, sy, sz;
						FbxDouble qx, qy, qz, qw;
						FbxDouble tx, ty, tz;

						in.read(reinterpret_cast<char*>(&keyFrame.time), sizeof(keyFrame.time));
						in.read(reinterpret_cast<char*>(&keyFrame.frame), sizeof(keyFrame.frame));

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

						keyFrame.scale.x = static_cast<float>(sx);
						keyFrame.scale.y = static_cast<float>(sy);
						keyFrame.scale.z = static_cast<float>(sz);

						keyFrame.rotation.x = static_cast<float>(qx);
						keyFrame.rotation.y = static_cast<float>(qy);
						keyFrame.rotation.z = static_cast<float>(qz);
						keyFrame.rotation.w = static_cast<float>(qw);

						keyFrame.translate.x = static_cast<float>(tx);
						keyFrame.translate.y = static_cast<float>(ty);
						keyFrame.translate.z = static_cast<float>(tz);
					}
				}
				mesh->AddAnimClip(info);
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

				mesh->AddBone(bone);
			}

			// SkinData 생성
			if (mesh->IsAnimMesh()) {
				// BoneOffet 행렬
				vector<Matrix> offsetVec(boneCount);
				for (size_t b = 0; b < boneCount; ++b) {
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

		// Material 정보 읽기
		// meshdata 확장자 삭제
		wstring resourcePath = _strFilePath.substr(0, _strFilePath.length() - 8) + L"fbm";
		vector<shared_ptr<Material>> materials;
		int materialCount = 0;
		in.read(reinterpret_cast<char*>(&materialCount), sizeof(materialCount));
		for (int j = 0; j < materialCount; ++j) {
			size_t nameLength = 0;
			wstring materialName;
			wstring diffuseTexName;
			wstring normalTexName;
			wstring specularTexName;

			in.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
			materialName.resize(nameLength);
			in.read(reinterpret_cast<char*>(materialName.data()), nameLength * sizeof(wchar_t));

			in.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
			diffuseTexName.resize(nameLength);
			in.read(reinterpret_cast<char*>(diffuseTexName.data()), nameLength * sizeof(wchar_t));

			in.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
			normalTexName.resize(nameLength);
			in.read(reinterpret_cast<char*>(normalTexName.data()), nameLength * sizeof(wchar_t));

			in.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
			specularTexName.resize(nameLength);
			in.read(reinterpret_cast<char*>(specularTexName.data()), nameLength * sizeof(wchar_t));

			shared_ptr<Material> material = make_shared<Material>();
			material->SetName(materialName);
			material->SetShader(GET_SINGLE(Resources)->Get<Shader>(L"Deferred"));

			if (!diffuseTexName.empty()) {
				shared_ptr<Texture> diffuseTexture = GET_SINGLE(Resources)->Load<Texture>(diffuseTexName, resourcePath + L"\\" + diffuseTexName);
				material->SetTexture(0, diffuseTexture);
			}

			if (!normalTexName.empty()) {
				shared_ptr<Texture> normalTexture = GET_SINGLE(Resources)->Load<Texture>(normalTexName, resourcePath + L"\\" + normalTexName);
				material->SetTexture(1, normalTexture);
			}

			if (!specularTexName.empty()) {
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

void MeshData::Save(const std::wstring& path, FBXLoader& loader) 
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
		std::wstring meshName = mesh.name;

		size_t nameLength = meshName.size();
		out.write(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));				// 이름 길이 기록
		out.write(reinterpret_cast<char*>(meshName.data()), nameLength * sizeof(wchar_t));	// 이름 기록

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

		// Animation 정보 기록
		bool hasAnimation = mesh.hasAnimation;
		out.write(reinterpret_cast<char*>(&hasAnimation), sizeof(hasAnimation)); // hasAnimation 기록

		if (hasAnimation) {
			// Animation Clip 정보 기록
			const vector<shared_ptr<FbxAnimClipInfo>>& animClips = loader.GetAnimClip();
			int animClipCount = static_cast<int>(animClips.size());
			out.write(reinterpret_cast<char*>(&animClipCount), sizeof(animClipCount)); // animClipCount 기록
			for (const shared_ptr<FbxAnimClipInfo>& animClip : animClips) {
				// 이름, 길이, 프레임 수 기록
				double duration = animClip->endTime.GetSecondDouble() - animClip->startTime.GetSecondDouble();
				int frameCount = static_cast<int>(animClip->endTime.GetFrameCount(animClip->mode)) - static_cast<int>(animClip->startTime.GetFrameCount(animClip->mode));
				std::wstring animName = animClip->name;

				size_t nameLength = animName.size();
				out.write(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));				// 이름 길이 기록
				out.write(reinterpret_cast<char*>(animName.data()), nameLength * sizeof(wchar_t));	// 이름 기록
				out.write(reinterpret_cast<char*>(&duration), sizeof(duration));					// duration 기록
				out.write(reinterpret_cast<char*>(&frameCount), sizeof(frameCount));				// frameCount 기록

				// KeyFrame Count 기록
				int keyFrameCount = static_cast<int>(animClip->keyFrames.size());
				out.write(reinterpret_cast<char*>(&keyFrameCount), sizeof(keyFrameCount)); // keyFrameCount 기록

				// bone 정보 기록
				int boneCount = static_cast<int>(animClip->keyFrames.size());
				out.write(reinterpret_cast<char*>(&boneCount), sizeof(boneCount)); // boneCount 기록

				for (int b = 0; b < boneCount; ++b) {
					vector<FbxKeyFrameInfo>& keyFrames = animClip->keyFrames[b];
					int keyFrameInfoCount = static_cast<int>(keyFrames.size());
					out.write(reinterpret_cast<char*>(&keyFrameInfoCount), sizeof(keyFrameInfoCount)); // keyFrameInfoCount 기록

					for (int f = 0; f < keyFrameInfoCount; ++f) {
						FbxKeyFrameInfo& keyFrame = keyFrames[f];
						out.write(reinterpret_cast<char*>(&keyFrame.time), sizeof(keyFrame.time));			// time 기록
						out.write(reinterpret_cast<char*>(&keyFrameInfoCount), sizeof(keyFrameInfoCount));	// frame 기록

						out.write(reinterpret_cast<char*>(&keyFrame.matTransform.GetS().mData[0]), sizeof(FbxDouble));	// scale 기록
						out.write(reinterpret_cast<char*>(&keyFrame.matTransform.GetS().mData[1]), sizeof(FbxDouble));
						out.write(reinterpret_cast<char*>(&keyFrame.matTransform.GetS().mData[2]), sizeof(FbxDouble));

						out.write(reinterpret_cast<char*>(&keyFrame.matTransform.GetQ().mData[0]), sizeof(FbxDouble));	// rotation 기록
						out.write(reinterpret_cast<char*>(&keyFrame.matTransform.GetQ().mData[1]), sizeof(FbxDouble));
						out.write(reinterpret_cast<char*>(&keyFrame.matTransform.GetQ().mData[2]), sizeof(FbxDouble));
						out.write(reinterpret_cast<char*>(&keyFrame.matTransform.GetQ().mData[3]), sizeof(FbxDouble));

						out.write(reinterpret_cast<char*>(&keyFrame.matTransform.GetT().mData[0]), sizeof(FbxDouble));	// translate 기록
						out.write(reinterpret_cast<char*>(&keyFrame.matTransform.GetT().mData[1]), sizeof(FbxDouble));
						out.write(reinterpret_cast<char*>(&keyFrame.matTransform.GetT().mData[2]), sizeof(FbxDouble));

					}
				}
			}

			// Bones 정보 기록
			const vector<shared_ptr<FbxBoneInfo>>& bones = loader.GetBones();
			int boneCount = static_cast<int>(bones.size());
			out.write(reinterpret_cast<char*>(&boneCount), sizeof(boneCount)); // boneCount 기록
			for (const shared_ptr<FbxBoneInfo>& bone : bones) {
				std::wstring boneName = bone->boneName;

				size_t nameLength = boneName.size();
				out.write(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));				// 이름 길이 기록
				out.write(reinterpret_cast<char*>(boneName.data()), nameLength * sizeof(wchar_t));	// 이름 기록
				out.write(reinterpret_cast<char*>(&bone->parentIndex), sizeof(bone->parentIndex));	// parentIndex 기록
				for (int y = 0; y < 4; y++)
					for (int x = 0; x < 4; x++)
						out.write(reinterpret_cast<char*>(&bone->matOffset.mData[y][x]), sizeof(double)); // matOffset 기록
			}
		}

		// Material 정보 기록
		int materialCount = static_cast<int>(mesh.materials.size());
		out.write(reinterpret_cast<char*>(&materialCount), sizeof(materialCount)); // materialCount 기록
		for (const FbxMaterialInfo& material : mesh.materials) {
			std::wstring materialName = material.name;
			wstring diffuseTexName = fs::path(material.diffuseTexName).filename();
			wstring normalTexName = fs::path(material.normalTexName).filename();
			wstring specularTexName = fs::path(material.specularTexName).filename();

			size_t nameLength = materialName.size();
			out.write(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));					// 이름 길이 기록
			out.write(reinterpret_cast<char*>(materialName.data()), nameLength * sizeof(wchar_t));	// 이름 기록

			nameLength = diffuseTexName.size();
			out.write(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
			out.write(reinterpret_cast<char*>(diffuseTexName.data()), nameLength * sizeof(wchar_t));

			nameLength = normalTexName.size();
			out.write(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
			out.write(reinterpret_cast<char*>(normalTexName.data()), nameLength * sizeof(wchar_t));

			nameLength = specularTexName.size();
			out.write(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
			out.write(reinterpret_cast<char*>(specularTexName.data()), nameLength * sizeof(wchar_t));
		}
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

