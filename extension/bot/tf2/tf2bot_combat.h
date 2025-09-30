#ifndef __NAVBOT_TF2BOT_COMBAT_INTERFACE_H_
#define __NAVBOT_TF2BOT_COMBAT_INTERFACE_H_

#include <bot/interfaces/combat.h>

class CTF2BotCombat : public ICombat
{
public:
	CTF2BotCombat(CBaseBot* bot);

protected:
	bool IsAbleToDodgeEnemies(const CKnownEntity* threat, const CBotWeapon* activeWeapon) override;
	bool CanFireWeaponAtEnemy(const CBaseBot* bot, const CKnownEntity* threat, const CBotWeapon* activeWeapon) const override;
	bool HandleWeapon(const CBaseBot* bot, const CBotWeapon* activeWeapon) override;
	void UseSecondaryAbilities(const CKnownEntity* threat, const CBotWeapon* activeWeapon) override;
private:

	void PyroFireCompressionBlast(const CKnownEntity* threat, const CBotWeapon* activeWeapon);
	void DemomanUseShieldCharge(const CKnownEntity* threat);
};

#endif // !__NAVBOT_TF2BOT_COMBAT_INTERFACE_H_
