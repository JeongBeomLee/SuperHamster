#pragma once

struct FbxMaterialInfo
{
    Vec4            diffuse;
    Vec4            ambient;
    Vec4            specular;
    wstring         name;
    wstring         diffuseTexName;
    wstring         normalTexName;
    wstring         specularTexName;
};

struct BoneWeight
{
    using Pair = pair<int32_t, double>;
    vector<Pair> boneWeights;

    void AddWeights(uint32_t index, double weight)
    {
        if (weight <= 0.0) // Corrected comparison with double literal
            return;

        auto findIt = std::find_if(boneWeights.begin(), boneWeights.end(),
            [weight](const Pair& p) { return p.second < weight; }); // Removed '=' from lambda capture

        if (findIt != boneWeights.end())
            boneWeights.insert(findIt, Pair(index, weight));
        else
            boneWeights.push_back(Pair(index, weight));

        // Limit weights to a maximum of 4
        if (boneWeights.size() > 4)
            boneWeights.pop_back();
    }

    void Normalize()
    {
        double sum = std::accumulate(boneWeights.begin(), boneWeights.end(), 0.0,
            [](double currentSum, const Pair& p) { return currentSum + p.second; }); // Use std::accumulate for sum

        for (auto& p : boneWeights) p.second /= sum; // Simplified normalization
    }
};

struct FbxMeshInfo
{
    wstring                             name;
    vector<Vertex>                      vertices;
    vector<vector<uint32_t>>            indices;
    vector<FbxMaterialInfo>             materials;
    vector<BoneWeight>                  boneWeights; // Bone weights
    bool                                hasAnimation;
};

struct FbxKeyFrameInfo
{
    FbxAMatrix  matTransform;
    double      time;
};

struct FbxBoneInfo
{
    wstring                 boneName;
    int32_t                 parentIndex;
    FbxAMatrix              matOffset;
};

struct FbxAnimClipInfo
{
    wstring                             name;
    FbxTime                             startTime;
    FbxTime                             endTime;
    FbxTime::EMode                      mode;
    vector<vector<FbxKeyFrameInfo>>     keyFrames;
};

class FBXLoader
{
public:
    FBXLoader();
    ~FBXLoader();

    void LoadFbx(const wstring& path);

    int32_t GetMeshCount() const { return static_cast<int32_t>(_meshes.size()); } // Added const correctness
    const FbxMeshInfo& GetMesh(int32_t idx) const { return _meshes[idx]; } // Added const correctness
    vector<shared_ptr<FbxBoneInfo>>& GetBones() { return _bones; }
    vector<shared_ptr<FbxAnimClipInfo>>& GetAnimClip() { return _animClips; }
    wstring GetResourceDirectory() const { return _resourceDirectory; } // Added const correctness

private:
    void Import(const wstring& path);

    void ParseNode(FbxNode* root);
    void LoadMesh(FbxMesh* mesh);
    void LoadMaterial(FbxSurfaceMaterial* surfaceMaterial);

    void        GetNormal(FbxMesh* mesh, FbxMeshInfo* container, int32_t idx, int32_t vertexCounter);
    void        GetTangent(FbxMesh* mesh, FbxMeshInfo* container, int32_t idx, int32_t vertexCounter);
    void        GetUV(FbxMesh* mesh, FbxMeshInfo* container, int32_t idx, int32_t vertexCounter);
    Vec4        GetMaterialData(FbxSurfaceMaterial* surface, const char* materialName, const char* factorName);
    wstring     GetTextureRelativeName(FbxSurfaceMaterial* surface, const char* materialProperty);

    void CreateTextures();
    void CreateMaterials();

    // Animation
    void LoadBones(FbxNode* node, int32_t idx = 0, int32_t parentIdx = -1); // Made LoadBones overload explicit
    void LoadAnimationInfo();

    void LoadAnimationData(FbxMesh* mesh, FbxMeshInfo* meshInfo);
    void LoadBoneWeight(FbxCluster* cluster, int32_t boneIdx, FbxMeshInfo* meshInfo);
    void LoadOffsetMatrix(FbxCluster* cluster, const FbxAMatrix& matNodeTransform, int32_t boneIdx, FbxMeshInfo* meshInfo);
    void LoadKeyframe(int32_t animIndex, FbxNode* node, FbxCluster* cluster, const FbxAMatrix& matNodeTransform, int32_t boneIdx, FbxMeshInfo* container);

    int32_t FindBoneIndex(const string& name); // Changed parameter to const reference
    FbxAMatrix GetTransform(FbxNode* node);

    void FillBoneWeight(FbxMesh* mesh, FbxMeshInfo* meshInfo);

private:
    FbxManager*     _manager    = nullptr;
    FbxScene*       _scene      = nullptr;
    FbxImporter*    _importer   = nullptr;
    wstring         _resourceDirectory;

    vector<FbxMeshInfo>                 _meshes;
    vector<shared_ptr<FbxBoneInfo>>     _bones;
    vector<shared_ptr<FbxAnimClipInfo>> _animClips;
    FbxArray<FbxString*>                _animNames;
};