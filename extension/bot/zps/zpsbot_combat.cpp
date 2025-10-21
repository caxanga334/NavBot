#include NAVBOT_PCH_FILE
#include "zpsbot.h"
#include "zpsbot_combat.h"

CZPSBotCombat::CZPSBotCombat(CZPSBot* bot) :
	ICombat(bot)
{
}

bool CZPSBotCombat::IsAbleToDodgeEnemies(const CKnownEntity* threat, const CBotWeapon* activeWeapon)
{
	// disabled for now in ZPS
	return false;
}
