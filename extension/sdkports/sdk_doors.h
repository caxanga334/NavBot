#ifndef __NAVBOT_SDKPORTS_SDK_DOORS_H_
#define __NAVBOT_SDKPORTS_SDK_DOORS_H_

namespace sdk::doors
{
	// doors spawnflags
	constexpr auto SF_DOOR_ROTATE_YAW = 0; // yaw by default
	constexpr auto SF_DOOR_START_OPEN_OBSOLETE = 1;
	constexpr auto SF_DOOR_ROTATE_BACKWARDS = 2;
	constexpr auto SF_DOOR_NONSOLID_TO_PLAYER = 4;
	constexpr auto SF_DOOR_PASSABLE = 8;
	constexpr auto SF_DOOR_ONEWAY = 16;
	constexpr auto SF_DOOR_NO_AUTO_RETURN = 32;
	constexpr auto SF_DOOR_ROTATE_ROLL = 64;
	constexpr auto SF_DOOR_ROTATE_PITCH = 128;
	constexpr auto SF_DOOR_PUSE = 256;	// door can be opened by player's use button.
	constexpr auto SF_DOOR_NONPCS = 512;	// NPC can't open
	constexpr auto SF_DOOR_PTOUCH = 1024; // player touch opens
	constexpr auto SF_DOOR_LOCKED = 2048;	// Door is initially locked
	constexpr auto SF_DOOR_SILENT = 4096;	// Door plays no audible sound, and does not alert NPCs when opened
	constexpr auto SF_DOOR_USE_CLOSES = 8192;	// Door can be +used to close before its autoreturn delay has expired.
	constexpr auto SF_DOOR_SILENT_TO_NPCS = 16384;	// Does not alert NPC's when opened.
	constexpr auto SF_DOOR_IGNORE_USE = 32768;	// Completely ignores player +use commands.
	constexpr auto SF_DOOR_NEW_USE_RULES = 65536;	// For func_door entities, behave more like prop_door_rotating with respect to +USE (changelist 242482)
}

#endif // !__NAVBOT_SDKPORTS_SDK_DOORS_H_
