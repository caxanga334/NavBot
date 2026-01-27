#ifndef __NAVBOT_INSMICBOT_COMBAT_H_
#define __NAVBOT_INSMICBOT_COMBAT_H_

#include <bot/interfaces/combat.h>

class CInsMICBot;

class CInsMICBotCombat : public ICombat
{
public:
	CInsMICBotCombat(CInsMICBot* bot);

protected:
	bool CanJumpOrCrouchToDodge() const override { return false; }
};


#endif // !__NAVBOT_INSMICBOT_COMBAT_H_
