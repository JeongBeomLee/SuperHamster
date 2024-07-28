#pragma once

class Scene;
enum
{
	MAX_LAYER = 32
};

class SceneManager
{
	DECLARE_SINGLE(SceneManager);

public:
	void Update();
	void Render();
	void LoadScene(wstring sceneName);
	void LoadNextScene();

	void SetLayerName(uint8 index, const wstring& name);
	const wstring& IndexToLayerName(uint8 index) { return layerNames[index]; }
	uint8 LayerNameToIndex(const wstring& name);

	shared_ptr<class GameObject> Pick(int32 screenX, int32 screenY);

public:
	shared_ptr<Scene> GetActiveScene() { return activeScene; }

private:
	shared_ptr<Scene> LoadMainScene();
	shared_ptr<Scene> LoadStage1();
	shared_ptr<Scene> LoadStage2();
	shared_ptr<Scene> LoadStage3();
	shared_ptr<Scene> LoadStage4();
	shared_ptr<Scene> LoadEndStage();

private:
	shared_ptr<Scene>			activeScene;

	array<wstring, MAX_LAYER>	layerNames;
	map<wstring, uint8>			layerIndex;

	uint8 currentSceneIndex = 1;
};

