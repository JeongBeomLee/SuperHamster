#include "pch.h"
#include "Game.h"
#include "Engine.h"
#include "SceneManager.h"

void Game::Init(const WindowInfo& info)
{
	gEngine->Init(info);
	GET_SINGLE(SceneManager)->LoadScene(L"TestScene");
}

void Game::Update()
{
	gEngine->Update();
}
