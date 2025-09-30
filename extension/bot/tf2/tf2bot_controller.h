#ifndef __NAVBOT_TF2_PLAYER_CONTROLLER_H_
#define __NAVBOT_TF2_PLAYER_CONTROLLER_H_
#pragma once

#include <bot/interfaces/playercontrol.h>

class CTF2BotPlayerController : public IPlayerController
{
public:
	CTF2BotPlayerController(CBaseBot* bot);
	virtual ~CTF2BotPlayerController();
};

#endif // !__NAVBOT_TF2_PLAYER_CONTROLLER_H_
