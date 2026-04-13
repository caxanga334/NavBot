#ifndef __NAVBOT_HL1MPBOT_SQUAD_H_
#define __NAVBOT_HL1MPBOT_SQUAD_H_

#include <bot/interfaces/squads.h>

class CHL1MPBot;

class CHL1MPBotSquad : public ISquad
{
public:
	CHL1MPBotSquad(CHL1MPBot* bot);

	bool CanJoinSquads() const override { return false; }
	bool CanLeadSquads() const override { return false; }
};

#endif // !__NAVBOT_HL1MPBOT_SQUAD_H_
