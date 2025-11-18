#ifndef __NAVBOT_TF2BOT_SQUAD_INTERFACE_H_
#define __NAVBOT_TF2BOT_SQUAD_INTERFACE_H_

#include <bot/interfaces/squads.h>

class CTF2BotSquad : public ISquad
{
public:
	CTF2BotSquad(CTF2Bot* bot);


	// Returns true if this bot can join squads as a member.
	bool CanJoinSquads() const override;
	// Returns true if this bot can create and lead squads.
	bool CanLeadSquads() const override;
	/**
	 * @brief Invoke to determine if this bot should use squad behavior.
	 * @return Return true to use squad behavior, false to keep the existing behavior.
	 */
	bool UsesSquadBehavior() const override;
};



#endif // !__NAVBOT_TF2BOT_SQUAD_INTERFACE_H_
