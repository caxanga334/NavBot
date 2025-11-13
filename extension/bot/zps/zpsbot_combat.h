#ifndef __NAVBOT_ZPSBOT_COMBAT_H_
#define __NAVBOT_ZPSBOT_COMBAT_H_

#include <bot/interfaces/combat.h>

class CZPSBot;

class CZPSBotCombat : public ICombat
{
public:
	CZPSBotCombat(CZPSBot* bot);

	void OnDifficultyProfileChanged() override;

	void Update() override;
	void Reset() override;

	bool IsAbleToDodgeEnemies(const CKnownEntity* threat, const CBotWeapon* activeWeapon) override;

protected:
	void DodgeEnemies(const CKnownEntity* threat, const CBotWeapon* activeWeapon) override;

private:
	bool m_shouldwalk;
};

#endif