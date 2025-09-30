#ifndef __NAVBOT_DODSBOT_MOVEMENT_INTERFACE_H_
#define __NAVBOT_DODSBOT_MOVEMENT_INTERFACE_H_

#include <bot/interfaces/movement.h>

class CDoDSBotMovement : public IMovement
{
public:
	CDoDSBotMovement(CBaseBot* bot);
	~CDoDSBotMovement() override;

	void Reset() override;
	void Update() override;

	// Maximum gap jump distance without sprinting
	float GetMaxGapJumpDistance() const override { return 160.0f; }
	// this is the distance bots can drop without taking fall damage
	float GetMaxDropHeight() const { return 200.0f; }

	void OnNavAreaChanged(CNavArea* oldArea, CNavArea* newArea) override;

private:
	bool m_goProne;
	CountdownTimer m_unProneTimer;
};

#endif // !__NAVBOT_DODSBOT_MOVEMENT_INTERFACE_H_
