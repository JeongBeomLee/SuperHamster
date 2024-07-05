#pragma once

#ifdef _DEBUG
	#pragma comment(lib, "../Output/Debug/Engine.lib")
#else
	#pragma comment(lib, "../Output/Release/Engine.lib")
#endif

#include "../Engine/EnginePch.h"
