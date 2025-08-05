#ifndef TF2BOT_UTILS_H_
#define TF2BOT_UTILS_H_

#include <vector>

class CTF2Bot;
class CTFWaypoint;
class CTFNavArea;

namespace tf2botutils
{
	/**
	 * @brief Selects a random waypoint for building a sentry gun.
	 * @param bot Bot that will build the sentry.
	 * @param maxRange Maximum range. Negative for no limit.
	 * @param searchCenter Optional center point for range calculation. Uses the bot position if not specified.
	 * @return Waypoint to build or NULL if none is found.
	 */
	CTFWaypoint* SelectWaypointForSentryGun(CTF2Bot* bot, const float maxRange = -1.0f, const Vector* searchCenter = nullptr);
	/**
	 * @brief Searches for a random nav area to build stuff.
	 * @param bot Bot that will build.
	 * @param maxRange Maximum range.
	 * @param searchCenter Optional position to determine the starting nav area.
	 * @param avoidSlopes Should skip areas with non flat ground?
	 * @return Nav area pointer or NULL if not found.
	 */
	CTFNavArea* FindRandomNavAreaToBuild(CTF2Bot* bot, const float maxRange = 4096.0f, const Vector* searchCenter = nullptr, const bool avoidSlopes = false, const bool checkVis = false);
	/**
	 * @brief Selects a random waypoint for buildng a dispenser.
	 * @param bot Bot that will build the dispenser.
	 * @param maxRange Maximum range. Negative for no limit.
	 * @param searchCenter Optional center point for range calculation. Uses the bot position if not specified.
	 * @param sentryWaypoint Optional waypoint to search for connected waypoints.
	 * @return Waypoint to build or NULL on failure.
	 */
	CTFWaypoint* SelectWaypointForDispenser(CTF2Bot* bot, const float maxRange = -1.0f, const Vector* searchCenter = nullptr, const CTFWaypoint* sentryWaypoint = nullptr);
	/**
	 * @brief Selects a random waypoint for buildng a teleporter exit.
	 * @param bot Bot that will build the teleporter exit.
	 * @param maxRange Maximum range. Negative for no limit.
	 * @param searchCenter Optional center point for range calculation. Uses the bot position if not specified.
	 * @param sentryWaypoint Optional waypoint to search for connected waypoints.
	 * @return Waypoint to build or NULL on failure.
	 */
	CTFWaypoint* SelectWaypointForTeleExit(CTF2Bot* bot, const float maxRange = -1.0f, const Vector* searchCenter = nullptr, const CTFWaypoint* sentryWaypoint = nullptr);
	/**
	 * @brief Selects a random waypoint for building a teleporter entrance.
	 * @param bot Bot that will build the teleporter.
	 * @param maxRange Maximum range. Negative for no limit.
	 * @param searchCenter Optional center point for range calculation. Uses the bot position if not specified.
	 * @return Waypoint to build or NULL if none is found.
	 */
	CTFWaypoint* SelectWaypointForTeleEntrance(CTF2Bot* bot, const float maxRange = -1.0f, const Vector* searchCenter = nullptr);
	// Finds a good area to build a tele entrance for when the bot is inside their spawnroom
	CTFNavArea* FindTeleEntranceNavAreaFromSpawnRoom(CTF2Bot* bot, const Vector& spawnPointPos);
	/**
	 * @brief Finds an area to build near the sentry gun.
	 * @param bot Bot that will build.
	 * @param sentryArea Nav area the sentry gun is at.
	 * @param maxRange Maximum search range.
	 * @param minRangeScale Minimum distance is max range multiplied by this.
	 * @return Nearest nav area or NULL if none found.
	 */
	CTFNavArea* FindAreaNearSentryGun(CTF2Bot* bot, CTFNavArea* sentryArea, const float maxRange, const float minRangeScale = 0.2f);
	// Determines the search start position for sentry gun build spot search.
	bool GetSentrySearchStartPosition(CTF2Bot* bot, Vector& spot);
	// Determines the maximum search range for sentry gun spots.
	float GetSentrySearchMaxRange(bool isWaypointSearch);
	/**
	 * @brief Finds a spot to build a sentry gun.
	 * @param bot Engineer bot.
	 * @param out Waypoint to build if found.
	 * @param area Nav area to build if found.
	 * @return true if a spot is found. false otherwise.
	 */
	bool FindSpotToBuildSentryGun(CTF2Bot* bot, CTFWaypoint** waypoint, CTFNavArea** area);
	/**
	 * @brief Check if it's possible to build a dispenser behind the sentry gun.
	 * @param bot Engineer bot. 
	 * @param spot Build position.
	 * @param distance Distance from the sentry gun. (MUST BE A POSITIVE NUMBER)
	 * @return true if possible. false otherwise.
	 */
	bool FindSpotToBuildDispenserBehindSentry(CTF2Bot* bot, Vector& spot, const float distance = 128.0f);
	/**
	 * @brief Finds a spot to build a dispenser gun.
	 * @param bot Engineer bot.
	 * @param out Waypoint to build if found.
	 * @param area Nav area to build if found.
	 * @param sentryGunWaypoint Waypoint to search for connections.
	 * @param sentryGunPos Optional position of where the sentry would be in case it's not built yet.
	 * @return true if a spot is found. false otherwise.
	 */
	bool FindSpotToBuildDispenser(CTF2Bot* bot, CTFWaypoint** waypoint, CTFNavArea** area, const CTFWaypoint* sentryGunWaypoint, const Vector* sentryGunPos = nullptr);
	/**
	 * @brief Finds a spot to build a teleporter entrance.
	 * @param bot Engineer bot.
	 * @param out Waypoint to build if found.
	 * @param area Nav area to build if found.
	 * @return true if a spot is found. false otherwise.
	 */
	bool FindSpotToBuildTeleEntrance(CTF2Bot* bot, CTFWaypoint** waypoint, CTFNavArea** area);
	/**
	 * @brief Finds a spot to build a teleporter exit.
	 * @param bot Engineer bot.
	 * @param out Waypoint to build if found.
	 * @param area Nav area to build if found.
	 * @param sentryGunWaypoint Waypoint to search for connections.
	 * @param sentryGunPos Optional position of where the sentry would be in case it's not built yet.
	 * @return true if a spot is found. false otherwise.
	 */
	bool FindSpotToBuildTeleExit(CTF2Bot* bot, CTFWaypoint** waypoint, CTFNavArea** area, const CTFWaypoint* sentryGunWaypoint, const Vector* sentryGunPos = nullptr);
	/**
	 * @brief Finds a player for medic bots to heal.
	 * @param bot The medic bot.
	 * @param maxSearchRange Maximum search range.
	 * @return Player entity to heal or NULL if no player is suitable.
	 */
	CBaseEntity* MedicSelectBestPatientToHeal(CTF2Bot* bot, const float maxSearchRange);
}

#endif // !TF2BOT_UTILS_H_
