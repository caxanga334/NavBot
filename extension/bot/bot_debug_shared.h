#ifndef NAVBOT_BOT_DEBUG_SHARED_H_
#define NAVBOT_BOT_DEBUG_SHARED_H_
#pragma once

enum BotDebugModes
{
	BOTDEBUG_NONE = 0,
	BOTDEBUG_SENSOR = (1 << 0), // bot sensors, vision & hearing
	BOTDEBUG_TASKS = (1 << 1),
	BOTDEBUG_LOOK = (1 << 2),
	BOTDEBUG_PATH = (1 << 3),
	BOTDEBUG_EVENTS = (1 << 4),
	BOTDEBUG_MOVEMENT = (1 << 5),
	BOTDEBUG_ERRORS = (1 << 6),
	BOTDEBUG_SQUADS = (1 << 7),
};

#endif // !NAVBOT_BOT_DEBUG_SHARED_H_


