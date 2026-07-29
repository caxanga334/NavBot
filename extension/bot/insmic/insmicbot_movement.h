#ifndef __NAVBOT_INSMICBOT_MOVEMENT_H_
#define __NAVBOT_INSMICBOT_MOVEMENT_H_

#include <bot/interfaces/movement.h>

class CInsMICBot;

class CInsMICBotMovement : public IMovement
{
public:
	CInsMICBotMovement(CInsMICBot* bot);

	float GetHullWidth() const override;
	float GetStandingHullHeight() const override;
	float GetCrouchedHullHeight() const override;
	float GetProneHullHeight() const override;
	bool IsCompletelyCrouched() const override;
	bool IsInCrouchTransition() const override { return false; /* not a thing in insurgency. */ }
	float GetWalkSpeed() const override { return GetMaxSpeed() * 0.70f; }
	void AdjustSpeedForPath(CMeshNavigator* path) override;
	void DetermineIdealPostureForPath(const CMeshNavigator* path) override;

private:

};

#endif // !__NAVBOT_INSMICBOT_MOVEMENT_H_
