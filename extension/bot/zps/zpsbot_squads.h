#ifndef __NAVBOT_ZPSBOT_SQUAD_INTERFACE_H_
#define __NAVBOT_ZPSBOT_SQUAD_INTERFACE_H_

#include <bot/interfaces/squads.h>

class CZPSBotSquad : public ISquad
{
public:
	CZPSBotSquad(CZPSBot* bot);

	bool CanLeadSquads() const override;

	static CZPSBotSquad* GetFirstCarrierSquadInterface(CZPSBot* bot);

private:

};

#endif