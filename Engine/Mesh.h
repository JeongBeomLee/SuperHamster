#pragma once
#include "Object.h"

class Material;
class StructuredBuffer;

struct IndexBufferInfo
{
	ComPtr<ID3D12Resource>		buffer;
	D3D12_INDEX_BUFFER_VIEW		bufferView;
	DXGI_FORMAT					format;
	uint32						count;
};

struct KeyFrameInfo
{
	double	time;
	int32	frame;
	Vec3	scale;
	Vec4	rotation;
	Vec3	translate;
};

struct BoneInfo
{
	wstring					boneName;
	int32					parentIdx;
	Matrix					matOffset;
};

struct AnimClipInfo
{
	wstring			animName;
	int32			frameCount;
	double			duration;
	vector<vector<KeyFrameInfo>>	keyFrames;
};

class Mesh : public Object
{
public:
	Mesh();
	virtual ~Mesh();

	void Create(const vector<Vertex>& vertexBuffer, const vector<uint32>& indexbuffer);
	void Render(uint32 instanceCount = 1, uint32 idx = 0);
	void Render(shared_ptr<class InstancingBuffer>& buffer, uint32 idx = 0);

	static shared_ptr<Mesh> CreateFromFBX(const struct FbxMeshInfo* meshInfo, class FBXLoader& loader);
	void CreateFromMeshData(const vector<Vertex>& vertexBuffer, const vector<vector<uint32>>& indexBuffer);

	void AddAnimClip(const AnimClipInfo& animClips)						{ _animClips.push_back(animClips); }
	void AddBone(const BoneInfo& bone)									{ _bones.push_back(bone); }
	void AddBoneFrameDataBuffer(shared_ptr<StructuredBuffer>& buffer)	{ _frameBuffer.push_back(buffer); }
	void SetBoneOffsetBuffer(shared_ptr<StructuredBuffer>& buffer)		{ _offsetBuffer = buffer; }

private:
	void CreateVertexBuffer(const vector<Vertex>& buffer);
	void CreateIndexBuffer(const vector<uint32>& buffer);
	void CreateBonesAndAnimations(class FBXLoader& loader);
	Matrix GetMatrix(FbxAMatrix& matrix);

public:
	uint32									GetSubsetCount()						{ return static_cast<uint32>(_vecIndexInfo.size()); }
	const vector<BoneInfo>*					GetBones()								{ return &_bones; }
	uint32									GetBoneCount()							{ return static_cast<uint32>(_bones.size()); }
	vector<AnimClipInfo>*					GetAnimClip()							{ return &_animClips; }

	bool									IsAnimMesh()							{ return !_animClips.empty(); }
	vector<shared_ptr<StructuredBuffer>>*	GetBoneFrameDataBufferVec()				{ return &_frameBuffer; }
	shared_ptr<StructuredBuffer>			GetBoneFrameDataBuffer(int32 index = 0) { return _frameBuffer[index]; } // ��ü �� ������ ����
	shared_ptr<StructuredBuffer>			GetBoneOffsetBuffer()					{ return  _offsetBuffer; }

private:
	ComPtr<ID3D12Resource>					_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW				_vertexBufferView = {};
	uint32									_vertexCount = 0;

	vector<IndexBufferInfo>					_vecIndexInfo;

	// Animation
	vector<AnimClipInfo>					_animClips;
	vector<BoneInfo>						_bones;

	shared_ptr<StructuredBuffer>			_offsetBuffer;	// �� ���� offset ���
	vector<shared_ptr<StructuredBuffer>>	_frameBuffer;	// ��ü �� ������ ����
};

