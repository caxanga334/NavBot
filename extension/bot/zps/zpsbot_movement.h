#ifndef __NAVBOT_ZPSBOT_MOVEMENT_H_
#define __NAVBOT_ZPSBOT_MOVEMENT_H_

#include <bot/interfaces/movement.h>

class CZPSBot;

class CZPSBotMovement : public IMovement
{
public:
	CZPSBotMovement(CZPSBot* bot);

	float GetAvoidDistance() const override { return 32.0f; }
	bool IsUseableObstacle(CBaseEntity* entity) override;
	int GetMovementCollisionGroup() const override;
	bool IsAreaTraversable(const CNavArea* area) const override;

	/* GameMovement limits the speed for us, always use max */
	virtual float GetWalkSpeed() const override { return GetMaxSpeed(); }

protected:

	void UpdateMovementButtons() override;

private:

};

#endif // !__NAVBOT_ZPSBOT_MOVEMENT_H_
