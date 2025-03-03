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
	CTFNavArea* FindRandomNavAreaToBuild(CTF2Bot* bot, const float maxRange = 4096.0f, const Vector* searchCenter = nullptr, const bool avoidSlopes = false);
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
	// Determines the search start position for sentry gun build spot search.
	bool GetSentrySearchStartPosition(CTF2Bot* bot, Vector& spot);
	float GetSentrySearchMaxRange(bool isWaypointSearch);
}

#endif // !TF2BOT_UTILS_H_
