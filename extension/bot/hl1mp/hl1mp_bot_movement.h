#ifndef __NAVBOT_HL1MPBOT_MOVEMENT_H_
#define __NAVBOT_HL1MPBOT_MOVEMENT_H_

#include <bot/interfaces/movement.h>

class CHL1MPBot;

class CHL1MPBotMovement : public IMovement
{
public:
	CHL1MPBotMovement(CHL1MPBot* bot);

	float GetMaxJumpHeight() const override { return 68.0f; }
	float GetMaxGapJumpDistance() const override;
};

#endif // !__NAVBOT_HL1MPBOT_MOVEMENT_H_
