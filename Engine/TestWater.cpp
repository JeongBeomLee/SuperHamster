#include "pch.h"
#include "TestWater.h"
#include "Camera.h"
#include "Scene.h"
#include "Transform.h"
#include "SceneManager.h"
#include "Timer.h"
#include "Resources.h"

TestWater::TestWater() 	: MonoBehaviour()
{
	_material = GET_SINGLE(Resources)->Get<Material>(L"Water");
}

void TestWater::Update()
{
	// 카메라 위치와 시간 업데이트
	Vec3 cameraPos = GET_SINGLE(SceneManager)->GetActiveScene()->GetMainCamera()->GetTransform()->GetWorldPosition();
    
    if (_material)
    {
        _time += DELTA_TIME;
        _material->SetFloat(0, _time);  // g_float_0에 시간 값을 설정
        _material->SetVec4(0, Vec4(cameraPos.x, cameraPos.y, cameraPos.z, 1.0f));  // g_vec4_0에 카메라 위치 설정

    }


}
