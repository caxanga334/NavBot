#ifndef __NAVBOT_ZPSBOT_COMBAT_H_
#define __NAVBOT_ZPSBOT_COMBAT_H_

#include <bot/interfaces/combat.h>

class CZPSBot;

class CZPSBotCombat : public ICombat
{
public:
	CZPSBotCombat(CZPSBot* bot);

	bool IsAbleToDodgeEnemies(const CKnownEntity* threat, const CBotWeapon* activeWeapon) override;

private:

};

#endif