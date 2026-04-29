#include NAVBOT_PCH_FILE
#include "cssbot.h"
#include "cssbot_combat.h"

CCSSBotCombat::CCSSBotCombat(CCSSBot* bot) :
	ICombat(bot)
{
}

void CCSSBotCombat::Reset()
{
	m_flashbangedTimer.Invalidate();
	ICombat::Reset();
}

bool CCSSBotCombat::CanFireWeaponAtEnemy(const CBaseBot* bot, const CKnownEntity* threat, const CBotWeapon* activeWeapon) const
{
	if (IsFlashbanged()) { return false; }

	return ICombat::CanFireWeaponAtEnemy(bot, threat, activeWeapon);
}
