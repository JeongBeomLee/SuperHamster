#pragma once
#include "MonoBehaviour.h"

class PlayerStateMachine;
class PlayerScript : public MonoBehaviour
{
public:
	PlayerScript();
	virtual void Update() override;

private:
	unique_ptr<PlayerStateMachine> stateMachine;
	InputState inputState;
	float velocity;

	void UpdateInputState();
	void SendMovePacket();
};