#ifndef __NAVBOT_BASE_BOT_SHARED_UTILS_H_
#define __NAVBOT_BASE_BOT_SHARED_UTILS_H_

#include <vector>
#include <unordered_set>
#include <navmesh/nav_pathfind.h>
#include <sdkports/sdk_traces.h>
#include <bot/interfaces/decisionquery.h>

namespace botsharedutils
{
	class TraceFilterVisibleSimple : public trace::CTraceFilterSimple
	{
	public:
		TraceFilterVisibleSimple();

		bool ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask) override;

	private:
		bool EntityBlocksLOS(CBaseEntity* entity);
	};

	/**
	 * @brief Builds a vector of positions where an enemy might come from, starting from the bot's current position. Doesn't search Nav Elevators by default.
	 */
	class AimSpotCollector : public INavAreaCollector<CNavArea>
	{
	public:
		AimSpotCollector(CBaseBot* bot);
		bool ShouldCollect(CNavArea* area) override;
		void OnDone() override;
		// Swaps the collected aim spots to the given vector
		inline void ExtractAimSpots(std::vector<Vector>& spots) { spots.swap(m_aimSpots); }

	private:
		std::vector<Vector> m_aimSpots;
		std::unordered_set<unsigned int> m_checkedAreas;
		Vector m_aimOrigin;
		Vector m_vecOffset1;
		Vector m_vecOffset2;
		TraceFilterVisibleSimple m_filter;

		bool BlocksLOS(Vector endpos);
	};

	/**
	 * @brief Searches for Nav areas that are visible to a given position.
	 */
	class RandomDefendSpotCollector : public INavAreaCollector<CNavArea>
	{
	public:
		RandomDefendSpotCollector(const Vector& spot, CBaseBot* bot);

		bool ShouldSearch(CNavArea* area) override;
		bool ShouldCollect(CNavArea* area) override;

	private:
		CBaseBot* m_bot;
		Vector m_spot;
		Vector m_vecOffset;
		TraceFilterVisibleSimple m_filter;
	};

	/**
	 * @brief Searches for a random sniper spot.
	 */
	class RandomSnipingSpotCollector : public INavAreaCollector<CNavArea>
	{
	public:
		/**
		 * @brief Constructor
		 * @param spot Position the sniper wants to overwatch.
		 * @param bot The bot that will snipe.
		 * @param maxRange Maximum range allowed.
		 */
		RandomSnipingSpotCollector(const Vector& spot, CBaseBot* bot, const float maxRange = 4096.0f);

		bool ShouldSearch(CNavArea* area) override;
		bool ShouldCollect(CNavArea* area) override;
		void OnDone() override;

		// The snipe area if found
		inline CNavArea* GetSnipingArea() const { return m_snipeArea; }

	private:
		CBaseBot* m_bot;
		Vector m_spot;
		Vector m_vecOffset1;
		Vector m_vecOffset2;
		TraceFilterVisibleSimple m_filter;
		CNavArea* m_snipeArea;
		float m_minRange;

		bool BlockedLOS(Vector endPos);
	};

	/**
	 * @brief Utility class for searching for a cover spot using the Nav Mesh.
	 */
	class FindCoverCollector : public INavAreaCollector<CNavArea>
	{
	public:
		/**
		 * @brief Constructor
		 * @param fromSpot Position to take cover from.
		 * @param radius Cover radius (minimum distance between cover and fromSpot).
		 * @param checkLOS If true, the cover must NOT have LOS to fromSpot.
		 * @param checkCorners If true, LOS will check the nav area's corners. If false, it only checks the center.
		 * @param maxSearchRange Maximum travel distance to search for cover.
		 * @param bot Bot that needs to take cover.
		 */
		FindCoverCollector(const Vector& fromSpot, const float radius, const bool checkLOS, const bool checkCorners, const float maxSearchRange, CBaseBot* bot);
		/**
		 * @brief Constructor
		 * @param fromSpot Position to take cover from.
		 * @param radius Cover radius (minimum distance between cover and fromSpot).
		 * @param checkLOS If true, the cover must NOT have LOS to fromSpot.
		 * @param checkCorners If true, LOS will check the nav area's corners. If false, it only checks the center.
		 * @param maxSearchRange Maximum travel distance to search for cover.
		 * @param origin Position to the entity that wants to find cover.
		 */
		FindCoverCollector(const Vector& fromSpot, const float radius, const bool checkLOS, const bool checkCorners, const float maxSearchRange, const Vector& origin);

		bool ShouldSearch(CNavArea* area) override;
		bool ShouldCollect(CNavArea* area) override;

	private:
		bool m_checkLOS; // if true, the cover spot must also block LOS
		bool m_checkCorners; // if true, check the nav area corners for LOS
		float m_minDistance; // minimum distance to cover spot
		Vector m_coverFrom; // find cover from this
		Vector m_vecOffset;
		Vector m_myPos; // position of the bot/entity that needs cover
		Vector m_dir;
		float m_cos;
		CBaseBot* m_bot;
		CTraceFilterWorldAndPropsOnly m_filter;
	};

	/**
	 * @brief Utility class for selecting random roaming destinations. Filters blocked/unreachable nav areas.
	 */
	class RandomDestinationCollector : public INavAreaCollector<CNavArea>
	{
	public:
		RandomDestinationCollector(CBaseBot* bot, const float travellimit = 4096.0f);
		RandomDestinationCollector(CNavArea* start, const float travellimit = 4096.0f);

		bool ShouldSearch(CNavArea* area) override;

	private:
		CBaseBot* m_bot;
	};
}

namespace botsharedutils::weapons
{
	/**
	 * @brief Gets the max attack range for the current weapon. Determines if the bot used primary or secondary attack last.
	 * @param bot Bot to get the weapon from.
	 * @return Weapon's maximum attack range. Returns a large negative value on failure.
	 */
	float GetMaxAttackRangeForCurrentlyHeldWeapon(CBaseBot* bot);
}

namespace botsharedutils::aiming
{
	// Gets a position to shoot enemy players
	Vector GetAimPositionForPlayers(CBaseBot* bot, CBaseExtPlayer* player, IDecisionQuery::DesiredAimSpot desiredAim, const CBotWeapon* weapon, const char* headbone = nullptr);
	/**
	 * @brief Utility for bots aiming a hitscan weapon on a player.
	 * @param bot Bot
	 * @param target Enemy
	 * @param desiredAim Desired aim spot.
	 * @param headbone (optional) Bone to search when going for headshots.
	 * @return Position to aim at.
	 */
	Vector AimAtPlayerWithHitScan(CBaseBot* bot, CBaseEntity* target, IDecisionQuery::DesiredAimSpot desiredAim, const CBotWeapon* weapon, const char* headbone = nullptr);
	/**
	 * @brief Utility for bots aiming a projectile weapon. (Projectile is not affected by gravity, moves in a constant speed).
	 * @param bot Bot
	 * @param target Enemy
	 * @param desiredAim Desired aim spot.
	 * @param headbone (optional) Bone to search when going for headshots.
	 * @param checkLOS If true, tests for obstructions on the projectile's path.
	 * @return Position to aim at.
	 */
	Vector AimAtPlayerWithProjectile(CBaseBot* bot, CBaseEntity* target, IDecisionQuery::DesiredAimSpot desiredAim, const CBotWeapon* weapon, const char* headbone = nullptr, const bool checkLOS = true);
	/**
	 * @brief Utility for bots aiming a ballistic weapon. (projectile is affected by gravity).
	 * @param bot Bot
	 * @param target Enemy
	 * @param desiredAim Desired aim spot.
	 * @param headbone (optional) Bone to search when going for headshots.
	 * @return Position to aim at.
	 */
	Vector AimAtPlayerWithBallistic(CBaseBot* bot, CBaseEntity* target, IDecisionQuery::DesiredAimSpot desiredAim, const CBotWeapon* weapon, const char* headbone = nullptr);
	// Gets a position to shoot non-player entities
	Vector GetAimPositionForEntities(CBaseBot* bot, CBaseEntity* target, IDecisionQuery::DesiredAimSpot desiredAim, const CBotWeapon* weapon);
	// Same as AimAtPlayerWithProjectile but for non-player entities
	Vector AimAtEntityWithBallistic(CBaseBot* bot, CBaseEntity* target, IDecisionQuery::DesiredAimSpot desiredAim, const CBotWeapon* weapon);
	/**
	 * @brief Basic select between head, center and abs origin for aiming.
	 * @param bot Bot that will aim at this target.
	 * @param target Target to aim.
	 */
	void SelectDesiredAimSpotForTarget(CBaseBot* bot, CBaseEntity* target);
}

#endif // !__NAVBOT_BASE_BOT_SHARED_UTILS_H_
