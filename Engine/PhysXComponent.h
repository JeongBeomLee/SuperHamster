#pragma once
#include "Component.h"

class PhysXComponent : public Component
{
public:
    PhysXComponent(PxRigidActor* physicsActor);
    virtual ~PhysXComponent();

    void SetPhysicsActor(PxRigidActor* physicsActor);
    PxRigidActor* GetPhysicsActor() const;

    virtual void Awake() override;
    virtual void Update() override;
    virtual void FinalUpdate() override;

private:
    PxRigidActor* mPhysicsActor;
};