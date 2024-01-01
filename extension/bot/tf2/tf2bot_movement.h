#ifndef NAVBOT_TF2_MOVEMENT_H_
#define NAVBOT_TF2_MOVEMENT_H_
#pragma once

#include <bot/interfaces/movement.h>

class CTF2BotMovement : public IMovement
{
public:
	CTF2BotMovement(CBaseBot* bot);
	virtual ~CTF2BotMovement();

	virtual float GetMovementSpeed() override { return 750.0f; }
private:
	
};

#endif // !NAVBOT_TF2_MOVEMENT_H_
