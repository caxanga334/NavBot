#ifndef NAVBOT_TEAM_FORTRESS_2_BOT_H_
#define NAVBOT_TEAM_FORTRESS_2_BOT_H_
#pragma once

#include <extplayer.h>
#include <bot/interfaces/playercontrol.h>
#include <bot/interfaces/movement.h>
#include <bot/interfaces/sensor.h>
#include <bot/interfaces/event_listener.h>
#include <bot/interfaces/behavior.h>
#include <bot/interfaces/profile.h>
#include <bot/basebot.h>

struct edict_t;

class CTFBot : public CBaseBot
{
public:
	CTFBot(edict_t* edict);
	virtual ~CTFBot();

	virtual void TryJoinGame() override;
};

#endif // !NAVBOT_TEAM_FORTRESS_2_BOT_H_
