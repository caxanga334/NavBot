#ifndef __NAVBOT_CSS_BOT_COMBAT_INTERFACE_H_
#define __NAVBOT_CSS_BOT_COMBAT_INTERFACE_H_

#include "../interfaces/combat.h"

class CCSSBot;

class CCSSBotCombat : public ICombat
{
public:
	CCSSBotCombat(CCSSBot* bot);

	void Reset() override;

	/**
	 * @brief Marks the bot as flashbanged.
	 * @param time Amount of time in seconds for the bot to be flashbanged.
	 */
	void SetFlashbangedTime(float time) { m_flashbangedTimer.Start(time); }
	// Returns true if the bot is flashbanged.
	bool IsFlashbanged() const { return !m_flashbangedTimer.IsElapsed(); }

protected:
	bool CanFireWeaponAtEnemy(const CBaseBot* bot, const CKnownEntity* threat, const CBotWeapon* activeWeapon) const override;

private:
	CountdownTimer m_flashbangedTimer;
};

#endif // !__NAVBOT_CSS_BOT_COMBAT_INTERFACE_H_
