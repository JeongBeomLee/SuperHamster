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

	vector<shared_ptr<GameObject>> Instantiate();

	shared_ptr<Mesh> GetMesh(int32 idx) { return _meshRenders[idx].mesh; }

private:
	vector<MeshRenderInfo> _meshRenders;
};
