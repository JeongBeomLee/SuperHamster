#include "pch.h"
#include "Game.h"
#include "../Engine/Engine.h"
#include "../Engine/SceneManager.h"

void Game::Init(const WindowInfo& info)
{
	gEngine->Init(info);
	GET_SINGLE(SceneManager)->LoadScene(L"TestScene");
}

void Game::Update()
{
	gEngine->Update();
}

void Game::Release()
{
}
