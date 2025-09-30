#ifndef __NAVBOT_DODSBOT_COMBAT_H_
#define __NAVBOT_DODSBOT_COMBAT_H_

#include <bot/interfaces/combat.h>

class CDoDSBotCombat : public ICombat
{
public:
	CDoDSBotCombat(CBaseBot* bot);

protected:
	// Can't fire weapons while jumping
	bool CanJumpOrCrouchToDodge() const override { return false; }

};

#endif // !__NAVBOT_DODSBOT_COMBAT_H_
