#ifndef __NAVBOT_TF2BOT_AIM_H_
#define __NAVBOT_TF2BOT_AIM_H_

class CTF2Bot;

#include <bot/interfaces/aim.h>

class CTF2BotAimHelper : public IBotAimHelper<CTF2Bot>
{
public:
	Vector SelectAimPosition(CTF2Bot* bot, CBaseEntity* entity, IDecisionQuery::DesiredAimSpot aimspot) override;

};

#endif // !__NAVBOT_TF2BOT_AIM_H_
