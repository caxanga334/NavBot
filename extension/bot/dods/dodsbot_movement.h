#ifndef __NAVBOT_DODSBOT_MOVEMENT_INTERFACE_H_
#define __NAVBOT_DODSBOT_MOVEMENT_INTERFACE_H_

#include <bot/interfaces/movement.h>

class CDoDSBotMovement : public IMovement
{
public:
	CDoDSBotMovement(CBaseBot* bot);
	~CDoDSBotMovement() override;

	// float GetHullWidth() override;
	// float GetStandingHullHeigh() override;
	float GetCrouchedHullHeigh() override;
	float GetProneHullHeigh() override;
	// Maximum gap jump distance without sprinting
	float GetMaxGapJumpDistance() const override { return 160.0f; }
	// this is the distance bots can drop without taking fall damage
	float GetMaxDropHeight() const { return 200.0f; }

private:

};

#endif // !__NAVBOT_DODSBOT_MOVEMENT_INTERFACE_H_
