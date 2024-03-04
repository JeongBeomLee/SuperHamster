#pragma once
#include "Object.h"

class Mesh;
class Material;
class GameObject;
class FBXLoader;

struct MeshRenderInfo
{
	shared_ptr<Mesh>				mesh;
	vector<shared_ptr<Material>>	materials;
};

class MeshData : public Object
{
public:
	MeshData();
	virtual ~MeshData();

public:
	static shared_ptr<MeshData> LoadFromFBX(const wstring& path);

	virtual void Load(const wstring& path);
	void Save(const wstring& path, FBXLoader& loader);
	void SaveBinary(const wstring& path, FBXLoader& loader);

	vector<shared_ptr<GameObject>> Instantiate();

private:
	vector<MeshRenderInfo> _meshRenders;
};
