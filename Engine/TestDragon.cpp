#include "pch.h"
#include "TestDragon.h"
#include "Input.h"
#include "Animator.h"

void TestDragon::Update()
{
	if (INPUT->GetButtonDown(KEY_TYPE::KEY_1)) {
		shared_ptr<Animator> animator = GetAnimator();
		if(!animator)
			return;

		int32 count = animator->GetAnimCount();
		int32 currentIndex = animator->GetCurrentClipIndex();

		int32 index = (currentIndex + 1) % count;
		
		animator->Play(index);
	}

	if (INPUT->GetButtonDown(KEY_TYPE::KEY_2)) {
		shared_ptr<Animator> animator = GetAnimator();
		if (!animator)
			return;

		int32 count = animator->GetAnimCount();
		int32 currentIndex = animator->GetCurrentClipIndex();

		int32 index = (currentIndex - 1 + count) % count;

		animator->Play(index);
	}
}