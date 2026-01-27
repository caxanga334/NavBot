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

protected:
	void DoJumpAssist() override;

private:

};

#endif // !__NAVBOT_INSMICBOT_MOVEMENT_H_
