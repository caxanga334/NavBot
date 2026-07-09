/**
 * Navbot's SourcePawn include file
 */
 
#if defined _navbot_included
 #endinput
#endif
#define _navbot_included

#include <sourcemod>

#include "navbot/nav_mesh.inc"
#include "navbot/baseplayer.inc"
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
 * Called when running the bot's update cycle.
 */
forward void OnPreNavBotUpdate(NavBot bot);

/**
 * Called when NavBot's navigation mesh is loaded.
 */
forward void OnNavBotNavMeshLoaded();

/**
 * Called when NavBot's navigation mesh is destroyed (unloaded).
 */
forward void OnNavBotNavMeshDestroyed();

/**
 * Called when a bot is stuck.
 * 
 * @param bot		NavBot instance that got stuck.
 * @param count		Count of consecutive stuck events.
 */
forward void OnNavBotStuck(NavBot bot, int count);

/**
 * Called when the mod interface detects a round restart.
 */
forward void OnNavBotModRoundRestart();

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
	 * Gets a BasePlayer instance by client index.
	 * @parma client		Index of the client to get the base player instance of.
	 * @return				BasePlayer instance. Use BasePlayer.IsNull to check if it's valid.
	 */
	public static native NavBotBasePlayer GetBasePlayer(int client);
	/**
	 * Returns true if the extension's bot feature is supported.
	 */
	public static native bool AreBotsSupported();
	// Runs the round restart routines.
	public static native void NotifyRoundRestart();
	// Returns true if the current mod uses workshop maps.
	public static native bool ModUsesWorkshopMaps();
}

/**
 * Do not edit below this line!
 */
public Extension __ext_navbot =
{
	name = "navbot",
	file = "navbot.ext",
/*
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
*/
	// NavBot ships a .autoload file
	autoload = 0,
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
	MarkNativeAsOptional("NavBotManager.GetBasePlayer");
	MarkNativeAsOptional("NavBotManager.AreBotsSupported");
	MarkNativeAsOptional("NavBotManager.NotifyRoundRestart");
	MarkNativeAsOptional("NavBotManager.ModUsesWorkshopMaps");

	/* Base Player */
	MarkNativeAsOptional("NavBotBasePlayer.IsNavBot.get");
	MarkNativeAsOptional("NavBotBasePlayer.GetLastKnownNavArea");
	MarkNativeAsOptional("NavBotBasePlayer.ForceUpdateLastKnownNavArea");
	MarkNativeAsOptional("NavBotBasePlayer.GetLocalEyeAngles");
	MarkNativeAsOptional("NavBotBasePlayer.WorldSpaceCenter");
	MarkNativeAsOptional("NavBotBasePlayer.SnapEyeAngles");

	/* Nav Area */
	MarkNativeAsOptional("NavBotNavArea.GetID");
	MarkNativeAsOptional("NavBotNavArea.GetCenter");
	MarkNativeAsOptional("NavBotNavArea.GetCorner");
	MarkNativeAsOptional("NavBotNavArea.GetExtent");
	MarkNativeAsOptional("NavBotNavArea.IsVisible");
	MarkNativeAsOptional("NavBotNavArea.IsBlocked");
	MarkNativeAsOptional("NavBotNavArea.GetClosestPointOnArea");

	/* Nav Mesh */
	MarkNativeAsOptional("NavBotNavMesh.IsLoaded");
	MarkNativeAsOptional("NavBotNavMesh.GetNearestNavArea");
	MarkNativeAsOptional("NavBotNavMesh.GetNavAreaByID");
	MarkNativeAsOptional("NavBotNavMesh.GetAuthor");
	MarkNativeAsOptional("NavBotNavMesh.GetEditorCount");
	MarkNativeAsOptional("NavBotNavMesh.GetEditor");
	MarkNativeAsOptional("NavBotNavMesh.GetLastLoadResult");
	MarkNativeAsOptional("NavBotNavMesh.Generate");
	MarkNativeAsOptional("NavBotNavMesh.Load");
	MarkNativeAsOptional("NavBotNavMesh.GetFullPathToNavMeshFile");

	/* Nav Area Vector */
	MarkNativeAsOptional("NavBotNavAreaVector.NavBotNavAreaVector");
	MarkNativeAsOptional("NavBotNavAreaVector.Push");
	MarkNativeAsOptional("NavBotNavAreaVector.Contains");
	MarkNativeAsOptional("NavBotNavAreaVector.Erase");
	MarkNativeAsOptional("NavBotNavAreaVector.Clear");
	MarkNativeAsOptional("NavBotNavAreaVector.At");
	MarkNativeAsOptional("NavBotNavAreaVector.Size.get");

	/* Nav Area Collector */
	MarkNativeAsOptional("NavBotNavAreaVector.NavBotNavAreaVector");
	MarkNativeAsOptional("NavBotNavAreaVector.SetSearchStartArea");
	MarkNativeAsOptional("NavBotNavAreaVector.ShouldSearch.set");
	MarkNativeAsOptional("NavBotNavAreaVector.ShouldCollect.set");
	MarkNativeAsOptional("NavBotNavAreaVector.TravelLimit.set");
	MarkNativeAsOptional("NavBotNavAreaVector.TravelLimit.get");
	MarkNativeAsOptional("NavBotNavAreaVector.SearchLadder.set");
	MarkNativeAsOptional("NavBotNavAreaVector.SearchOffMeshLinks.set");
	MarkNativeAsOptional("NavBotNavAreaVector.SearchElevators.set");
	MarkNativeAsOptional("NavBotNavAreaVector.SearchIncoming.set");
	MarkNativeAsOptional("NavBotNavAreaVector.Execute");
	MarkNativeAsOptional("NavBotNavAreaVector.Reset");
	MarkNativeAsOptional("NavBotNavAreaVector.EndSearch");
	MarkNativeAsOptional("NavBotNavAreaVector.GetSearchNodeForArea");
	MarkNativeAsOptional("NavBotNavAreaVector.CollectedAreaSize.get");
	MarkNativeAsOptional("NavBotNavAreaVector.GetRandomCollectedArea");
	MarkNativeAsOptional("NavBotNavAreaVector.GetFarthestCollectedArea");
	MarkNativeAsOptional("NavBotNavAreaVector.GetNearestCollectedArea");
	MarkNativeAsOptional("NavBotNavAreaVector.GetCollectedAreas");

	/* Nav Blocker */
	MarkNativeAsOptional("NavBotNavBlocker.NavBotNavBlocker");
	MarkNativeAsOptional("NavBotNavBlocker.AddAreas");
	MarkNativeAsOptional("NavBotNavBlocker.UpdateBlockedStatus");
	MarkNativeAsOptional("NavBotNavBlocker.OnRoundRestart.set");
	MarkNativeAsOptional("NavBotNavBlocker.OnRecomputeInternalData.set");
	MarkNativeAsOptional("NavBotNavBlocker.Entity.set");
	MarkNativeAsOptional("NavBotNavBlocker.Entity.get");

	/* Mod Interface */
	MarkNativeAsOptional("NavBotModInterface.GetModName");
	MarkNativeAsOptional("NavBotModInterface.ParseModSettingsFile");
	MarkNativeAsOptional("NavBotModInterface.ParseDifficultyProfileFile");
	MarkNativeAsOptional("NavBotModInterface.ParseWeaponConfigFile");
	MarkNativeAsOptional("NavBotModInterface.GetCurrentMapName");
	MarkNativeAsOptional("NavBotModInterface.GetModFolder");
	MarkNativeAsOptional("NavBotModInterface.GetFallbackModFolder");
	MarkNativeAsOptional("NavBotModInterface.AddClassnameToNPCScanList");
	MarkNativeAsOptional("NavBotModInterface.RemoveClassnameFromNPCScanList");

	/* ZPS Mod Interface */
	MarkNativeAsOptional("NavBotZPSModInterface.SetCurrentObjective");
	MarkNativeAsOptional("NavBotZPSModInterface.SetObjectiveMoveGoal");
	MarkNativeAsOptional("NavBotZPSModInterface.SetObjectiveUseButton");
	MarkNativeAsOptional("NavBotZPSModInterface.SetObjectiveItemSearchID");
	MarkNativeAsOptional("NavBotZPSModInterface.SetObjectiveItemUseTarget");
	MarkNativeAsOptional("NavBotZPSModInterface.SetObjectiveDetectionRadius");
	MarkNativeAsOptional("NavBotZPSModInterface.ResetObjective");

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
	MarkNativeAsOptional("NavBot.Reset");
	MarkNativeAsOptional("NavBot.DelayedFakeClientCommand");
	MarkNativeAsOptional("NavBot.GetInventoryInterface");
	MarkNativeAsOptional("NavBot.GetMovementInterface");
	MarkNativeAsOptional("NavBot.GetSensorInterface");
	MarkNativeAsOptional("NavBot.GetPlayerControllerInterface");
	MarkNativeAsOptional("NavBot.IsLineOfFireClear");
	MarkNativeAsOptional("NavBot.SendImpulse");
	MarkNativeAsOptional("NavBot.GetDebugIdentifier");

	/* Base Interface */
	MarkNativeAsOptional("NavBotBotInterface.Reset");

	/* Shared Bot Memory Interface  */
	MarkNativeAsOptional("NavBotSharedBotMemory.ReportEntity");
	MarkNativeAsOptional("NavBotSharedBotMemory.ForgetEntity");

	/* Inventory Interface */
	MarkNativeAsOptional("NavBotInventoryInterface.EquipWeapon");
	MarkNativeAsOptional("NavBotInventoryInterface.RegisterWeapon");
	MarkNativeAsOptional("NavBotInventoryInterface.RequestUpdate");
	MarkNativeAsOptional("NavBotInventoryInterface.SelectFirstWeaponWithTag");
	MarkNativeAsOptional("NavBotInventoryInterface.SelectBestWeapon");

	/* Movement Interface */
	MarkNativeAsOptional("NavBotMovementInterface.IsPotentiallyTraversable");
	MarkNativeAsOptional("NavBotMovementInterface.IsStuck");
	MarkNativeAsOptional("NavBotMovementInterface.GetStuckDuration");
	MarkNativeAsOptional("NavBotMovementInterface.IsAreaTraversable");
	MarkNativeAsOptional("NavBotMovementInterface.ClearStuckStatus");

	/* Sensor Interface */
	MarkNativeAsOptional("NavBotSensorInterface.IsIgnored");
	MarkNativeAsOptional("NavBotSensorInterface.IsFriendly");
	MarkNativeAsOptional("NavBotSensorInterface.IsEnemy");
	MarkNativeAsOptional("NavBotSensorInterface.IsAbleToSeePos");
	MarkNativeAsOptional("NavBotSensorInterface.IsAbleToSeeEntity");
	MarkNativeAsOptional("NavBotSensorInterface.IsLineOfSightClearPos");
	MarkNativeAsOptional("NavBotSensorInterface.IsLineOfSightClearEntity");
	MarkNativeAsOptional("NavBotSensorInterface.IsInFieldOfView");
	MarkNativeAsOptional("NavBotSensorInterface.SetPrimaryThreatOverride");
	MarkNativeAsOptional("NavBotSensorInterface.GetPrimaryKnownThreatIndex");

	/* Player Controller Interface */
	MarkNativeAsOptional("NavBotPlayerControllerInterface.AimAtEntity");
	MarkNativeAsOptional("NavBotPlayerControllerInterface.AimAtPos");
	MarkNativeAsOptional("NavBotPlayerControllerInterface.IsAimOnTarget");
	MarkNativeAsOptional("NavBotPlayerControllerInterface.PressButtonByID");
	MarkNativeAsOptional("NavBotPlayerControllerInterface.ReleaseButtonByID");
}

#endif