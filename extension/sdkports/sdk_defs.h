#ifndef __NAVBOT_SDK_DEFS_H_
#define __NAVBOT_SDK_DEFS_H_

namespace sdkdefs
{

	// prop_door_rotating states, from game\server\BasePropDoor.h
	enum DoorState_t
	{
		DOOR_STATE_CLOSED = 0,
		DOOR_STATE_OPENING,
		DOOR_STATE_OPEN,
		DOOR_STATE_CLOSING,
		DOOR_STATE_AJAR,
	};

	// Check directions for door movement
	enum doorCheck_e
	{
		DOOR_CHECK_FORWARD,		// Door's forward opening direction
		DOOR_CHECK_BACKWARD,	// Door's backward opening direction
		DOOR_CHECK_FULL,		// Door's complete movement volume
	};

	enum PropDoorRotatingSpawnPos_t
	{
		DOOR_SPAWN_CLOSED = 0,
		DOOR_SPAWN_OPEN_FORWARD,
		DOOR_SPAWN_OPEN_BACK,
		DOOR_SPAWN_AJAR,
	};

	enum PropDoorRotatingOpenDirection_e
	{
		DOOR_ROTATING_OPEN_BOTH_WAYS = 0,
		DOOR_ROTATING_OPEN_FORWARD,
		DOOR_ROTATING_OPEN_BACKWARD,
	};
}

#endif // !__NAVBOT_SDK_DEFS_H_
