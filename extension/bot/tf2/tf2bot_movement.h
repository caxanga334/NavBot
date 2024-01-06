#ifndef NAVBOT_TF2_MOVEMENT_H_
#define NAVBOT_TF2_MOVEMENT_H_
#pragma once

#include <bot/interfaces/movement.h>

class CTF2BotMovement : public IMovement
{
public:
	CTF2BotMovement(CBaseBot* bot);
	virtual ~CTF2BotMovement();

	virtual void Reset() override;
	virtual void Frame() override;

	virtual float GetMovementSpeed() override { return 750.0f; }
	bool CanDoubleJump() const;
	void DoDoubleJump();

private:
	int m_doublejumptimer;
};

#endif // !NAVBOT_TF2_MOVEMENT_H_
