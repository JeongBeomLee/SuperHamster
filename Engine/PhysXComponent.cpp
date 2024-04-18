#include "pch.h"
#include "PhysXComponent.h"
#include "GameObject.h"
#include "Transform.h"
#include "Engine.h"

PhysXComponent::PhysXComponent(PxRigidActor* physicsActor) : Component(COMPONENT_TYPE::PHYSX)
{
}

PhysXComponent::~PhysXComponent()
{
    if (mPhysicsActor)
    {
        mPhysicsActor->release();
        mPhysicsActor = nullptr;
    }
}

void PhysXComponent::SetPhysicsActor(PxRigidActor* physicsActor)
{
    mPhysicsActor = physicsActor;
}

PxRigidActor* PhysXComponent::GetPhysicsActor() const
{
    return mPhysicsActor;
}

void PhysXComponent::Awake()
{
    // PhysX 객체 초기화 작업 수행
    if (mPhysicsActor)
    {
		mPhysicsActor->userData = GetGameObject().get();
	}
}

void PhysXComponent::Update()
{
	// 게임 객체의 위치와 회전을 피직스 객체에 동기화
	/*Vec3 position = GetGameObject()->GetTransform()->GetLocalPosition();
	Vec3 rotation = GetGameObject()->GetTransform()->GetLocalRotation();
	PxTransform transform(PxVec3(position.x, position.y, position.z), PxQuat(rotation.x, rotation.y, rotation.z, 1.0f));
	mPhysicsActor->setGlobalPose(transform);*/
}

void PhysXComponent::FinalUpdate()
{
	// 피직스 객체의 위치와 회전을 게임 객체에 동기화
	PxTransform transform = mPhysicsActor->getGlobalPose();
	GetGameObject()->GetTransform()->SetLocalPosition(Vec3(transform.p.x, transform.p.y, transform.p.z));
	GetGameObject()->GetTransform()->SetLocalRotation(Vec3(transform.q.x, transform.q.y, transform.q.z));
}
