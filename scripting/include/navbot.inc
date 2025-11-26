/**
 * Navbot's SourcePawn include file
 */
 
#if defined _navbot_included
 #endinput
#endif
#define _navbot_included

#include <core>

#include "navbot/nav_mesh.inc"
#include "navbot/bots.inc"
#include "navbot/mod.inc"

enum NavBotAISoundType
{
	NAVBOT_SOUND_INVALID = -1,		// Invalid sound (don't use it)
	NAVBOT_SOUND_GENERIC = 0, 		// generic sounds
	NAVBOT_SOUND_WEAPON, 			// weapon sound (except firing)
	NAVBOT_SOUND_GUNFIRE, 			// gun fire
	NAVBOT_SOUND_PLAYER, 			// player sounds
	NAVBOT_SOUND_FOOTSTEP, 			// NPC/player footstep
	NAVBOT_SOUND_VOICE, 			// NPC/player voice
	NAVBOT_SOUND_WORLD, 			// world emitted sounds
	NAVBOT_SOUND_DOOR, 				// door opening/closing
	NAVBOT_SOUND_TURRET, 			// turret/sentry gun sound
	NAVBOT_SOUND_DANGER, 			// something dangerous

	NAVBOT_MAX_SOUND_TYPES
};

/**
 * Fires an AI sound event for all NavBot's bots.
 *
 * @param entity			The entity that made the sound.
 * @param origin			The sound position uses the entity's WorldSpaceCenter by default, you can pass a position here to override it.
 * @param soundType			Sound type made by the entity.
 * @param maxRadius			Maximum distance from origin the sound can be heard. Negative values means it's global.
 * @error					Invalid entity, invalid sound type or volume value given.
 */
native void FireNavBotSoundEvent(int entity, const float origin[3] = NULL_VECTOR, NavBotAISoundType soundType, const float maxRadius);

/**
 * Called when a new NavBot instance is about to be created.
 * 
 * @return Plugin_Continue to allow the bot to be added. Anything else to prevent the bot from being added.
 */
forward Action OnPreNavBotAdd();

/**
 * Called when a NavBot was just added to the game.
 * 
 * @param bot Client index of the bot that was just added.
 */
forward void OnNavBotAdded(int bot);

/**
 * Builds a path from the start position to the end position.
 * 
 * @param vStart				Path start position.
 * @param vEnd					Path goal position.
 * @param maxJumpHeight			Maximum jump height used for path finding. Set to 0 if you only want paths that doesn't require jumps.
 * @param maxGapJumpDistance	Maximum gap distance used for path finding. Set to half the hull width if you only want paths that doesn't require gap jumps.
 * @param maxDropHeight			Maximum drop height used for path finding.
 * @return						True if a path was found, false otherwise.
 */
native bool BuildPathSimple(const float vStart[3], const float vEnd[3], const float maxJumpHeight = 57.0, const float maxGapJumpDistance = 200.0, const float maxDropHeight = 300.0);

/**
 * Retrieves a path segment.
 * A path must be built first using BuildPathSimple.
 * Indexes starts at 0 and ends at GetPathSegmentCount() - 1
 * 
 * @param index			Path segment index.
 * @param position		Vector to store the path segment position at.
 * 
 * @error				Path segment index out of bounds.
 */
native void GetPathSegment(const int index, float position[3]);

// Returns the number of stored path segments.
native int GetPathSegmentCount();

methodmap NavBotManager
{
	/**
	 * Checks if the given client is a NavBot managed fake client.
	 *
	 * @param client        	Client index.
	 * @return					True if the given client is a NavBot, false otherwise.
	 * @error               	Invalid client index
	 */
	public static native bool IsNavBot(int client);
	/**
	 * Returns the number of Navbots in-game.
	 */
	public static native int GetNavBotCount();
	/**
	 * Gets a NavBot instance by client index.
	 * @param client  Client index
	 * @return  NavBot instance or NULL_NAVBOT is a navbot doesn't exists for the given client index.
	 */
	public static native NavBot GetNavBotByIndex(int client);
	/**
	 * Returns true if the extension's bot feature is supported.
	 */
	public static native bool AreBotsSupported();
}

/**
 * Do not edit below this line!
 */
public Extension __ext_navbot =
{
	name = "navbot",
	file = "navbot.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};

#if !defined REQUIRE_EXTENSIONS
public __ext_navbot_SetNTVOptional()
{
	MarkNativeAsOptional("FireNavBotSoundEvent");
	MarkNativeAsOptional("BuildPathSimple");
	MarkNativeAsOptional("GetPathSegment");
	MarkNativeAsOptional("GetPathSegmentCount");

	/* Manager */
	MarkNativeAsOptional("NavBotManager.IsNavBot");
	MarkNativeAsOptional("NavBotManager.GetNavBotCount");
	MarkNativeAsOptional("NavBotManager.GetNavBotByIndex");
	MarkNativeAsOptional("NavBotManager.AreBotsSupported");

	/* Nav Area */
	MarkNativeAsOptional("NavBotNavArea.GetID");
	MarkNativeAsOptional("NavBotNavArea.GetCenter");

	/* Nav Mesh */
	MarkNativeAsOptional("NavBotNavMesh.IsLoaded");
	MarkNativeAsOptional("NavBotNavMesh.GetNearestNavArea");
	MarkNativeAsOptional("NavBotNavMesh.GetNavAreaByID");
	MarkNativeAsOptional("NavBotNavMesh.GetAuthor");
	MarkNativeAsOptional("NavBotNavMesh.GetEditorCount");
	MarkNativeAsOptional("NavBotNavMesh.GetEditor");

	/* Mod Interface */
	MarkNativeAsOptional("NavBotModInterface.GetModName");
	MarkNativeAsOptional("NavBotModInterface.ParseModSettingsFile");
	MarkNativeAsOptional("NavBotModInterface.ParseDifficultyProfileFile");

	/* Navigator */
	MarkNativeAsOptional("MeshNavigator.MeshNavigator");
	MarkNativeAsOptional("MeshNavigator.Update");
	MarkNativeAsOptional("MeshNavigator.IsValid");
	MarkNativeAsOptional("MeshNavigator.ComputeToPos");
	MarkNativeAsOptional("MeshNavigator.GetMoveGoal");
	MarkNativeAsOptional("MeshNavigator.StartRepathTimer");
	MarkNativeAsOptional("MeshNavigator.NeedsRepath");
	MarkNativeAsOptional("MeshNavigator.Invalidate");

	/* Bots */
	MarkNativeAsOptional("NavBot.NavBot");
	MarkNativeAsOptional("NavBot.SetSkillLevel");
	MarkNativeAsOptional("NavBot.DelayedFakeClientCommand");
	MarkNativeAsOptional("NavBot.GetInventoryInterface");

	/* Base Interface */
	MarkNativeAsOptional("NavBotBotInterface.Reset");

	/* Inventory Interface */
	MarkNativeAsOptional("NavBotInventoryInterface.EquipWeapon");
	MarkNativeAsOptional("NavBotInventoryInterface.RegisterWeapon");
	MarkNativeAsOptional("NavBotInventoryInterface.RequestUpdate");
}
#endif