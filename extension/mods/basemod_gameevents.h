#ifndef SMNAV_BASEMOD_GAME_EVENTS_H_
#define SMNAV_BASEMOD_GAME_EVENTS_H_
#pragma once

#include <core/eventmanager.h>

class CPlayerSpawnEvent : public IEventReceiver
{
public:
	CPlayerSpawnEvent();

	virtual void OnGameEvent(IGameEvent* gameevent) override;
};




#endif // !SMNAV_BASEMOD_GAME_EVENTS_H_

