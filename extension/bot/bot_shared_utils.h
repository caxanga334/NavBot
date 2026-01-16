#ifndef __NAVBOT_BASE_BOT_SHARED_UTILS_H_
#define __NAVBOT_BASE_BOT_SHARED_UTILS_H_

#include <vector>
#include <unordered_set>
#include <navmesh/nav_mesh.h>
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
		CNavArea* m_area;
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
		CNavArea* m_area;
		CNavArea* m_snipeArea;
		float m_minRange;
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

	/**
	 * @brief Utility class for collecting reachable areas around the bot.
	 */
	class IsReachableAreas : public INavAreaCollector<CNavArea>
	{
	public:
		/**
		 * @brief Constructor.
		 * @param bot Bot
		 * @param limit Travel limit.
		 * @param searchLadders Allow searching ladders?
		 * @param searchLinks Allow searching off-mesh links?
		 * @param searchElevators Allow searching elevators?
		 */
		IsReachableAreas(CBaseBot* bot, const float limit, const bool searchLadders = true, const bool searchLinks = true, const bool searchElevators = true);

		bool ShouldSearch(CNavArea* area) override;

		// returns true if this area can be reached.
		const bool IsReachable(CNavArea* area, float* cost = nullptr) const;

		CBaseBot* GetBot() const { return m_bot; }

	private:
		CBaseBot* m_bot;
	};

	/**
	 * @brief Utility class for spreading danger around nearby areas.
	 */
	class SpreadDangerToNearbyAreas : public INavAreaCollector<CNavArea>
	{
	public:
		/**
		 * @brief Constructor.
		 * @param start Starting nav area.
		 * @param teamid Team Index.
		 * @param travellimit Max travel to spread danger.
		 * @param danger Danger amount to add.
		 * @param maxdanger Max danger.
		 */
		SpreadDangerToNearbyAreas(CNavArea* start, int teamid, const float travellimit, const float danger, const float maxdanger);

		bool ShouldSearch(CNavArea* area) override;
		bool ShouldCollect(CNavArea* area) override;
		void OnDone() override;

	private:
		float m_danger;
		float m_limit;
		int m_teamid;
	};

	/**
	 * @brief Utility area collector for finding an area to retreat from current known threats
	 */
	class SelectRetreatArea : public INavAreaCollector<CNavArea>
	{
	public:
		enum class RetreatAreaPreference : int
		{
			NEAREST = 0, // Retreat to the nearest available area
			FURTHEST, // Retreat to the furthest available area
			RANDOM, // Retreat to a random available area

			MAX_RETREAT_AREA_TYPES
		};

		/**
		 * @brief Constructor.
		 * @param bot Bot that will be retreating.
		 * @param preference Retreat area selection preference type.
		 * @param minDistFromThreat Areas must be at least this far from enemies to be considered a valid retreat area.
		 * @param maxRetreatDist Maximum distance to search for a retreat area.
		 */
		SelectRetreatArea(CBaseBot* bot, RetreatAreaPreference preference = RetreatAreaPreference::RANDOM, const float minDistFromThreat = 512.0f, const float maxRetreatDist = 8192.0f);


		bool ShouldSearch(CNavArea* area) override;
		bool ShouldCollect(CNavArea* area) override;
		void OnDone() override;

		bool HasFoundRetreatArea() const { return m_retreatArea != nullptr; }
		const CNavArea* GetRetreatToArea() const { return m_retreatArea; }

	private:
		CBaseBot* m_bot;
		float m_mindistance; // minimum distance from threat
		RetreatAreaPreference m_preference;
		CNavArea* m_retreatArea;
	};

	class CollectPatrolAreas : public INavAreaCollector<CNavArea>
	{
	public:
		CollectPatrolAreas(CBaseBot* bot, const Vector& start, const float minDistanceFromStart, const float maxSearchDistance = 4096.0f);

		bool ShouldSearch(CNavArea* area) override;
		bool ShouldCollect(CNavArea* area) override;

	private:
		CBaseBot* m_bot;
		Vector m_vStart;
		Vector m_vViewOffset;
		float m_minDistance;
		std::vector<Vector> m_points;

		static constexpr float MAX_RANGE_FOR_VIS_CHECKS = 1024.0f;
	};

	/**
	 * @brief Utility nav area collector for seaching for reachable nav areas with hiding spots.
	 */
	class HidingSpotCollector : public IsReachableAreas
	{
	public:
		HidingSpotCollector(CBaseBot* bot, float maxDistance) :
			IsReachableAreas(bot, maxDistance)
		{
		}

		void OnDone() override;
		CNavArea* GetRandomHidingArea() const;

	private:
		std::vector<CNavArea*> m_hidingareas; // areas with hiding spots
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
	/**
	 * @brief Basic select between head, center and abs origin for aiming.
	 * @param bot Bot that will aim at this target.
	 * @param target Target to aim.
	 */
	void SelectDesiredAimSpotForTarget(CBaseBot* bot, CBaseEntity* target);
}

namespace botsharedutils::waypoints
{
	/**
	 * @brief Gets a random waypoint with the "roam" flag.
	 * @param bot Bot that will be using this waypoint.
	 * @param maxRange Maximum range between the bot and the waypoint. Negative for no limit.
	 * @return Random waypoint or NULL if none found.
	 */
	CWaypoint* GetRandomRoamWaypoint(CBaseBot* bot, const float maxRange = -1.0f);
	/**
	 * @brief Gets a random waypoint with the "defend" flag.
	 * @param bot Bot that will be using this waypoint.
	 * @param defendSpot Position the bot will defend, used in range calculations. If NULL, the bot position is used instead.
	 * @param maxRange Maximum range between the bot and the waypoint. Negative for no limit.
	 * @return Random waypoint or NULL if none found.
	 */
	CWaypoint* GetRandomDefendWaypoint(CBaseBot* bot, const Vector* defendSpot, const float maxRange = -1.0f);
	/**
	 * @brief Gets a random waypoint with the "sniper" flag.
	 * @param bot Bot that will be using this waypoint.
	 * @param defendSpot Position the bot will overwatch, used in range calculations. If NULL, the bot position is used instead.
	 * @param maxRange Maximum range between the bot and the waypoint. Negative for no limit.
	 * @return Random waypoint or NULL if none found.
	 */
	CWaypoint* GetRandomSniperWaypoint(CBaseBot* bot, const Vector* defendSpot, const float maxRange = -1.0f);
}

/**
 * @brief Threat selection
 */
namespace botsharedutils::threat
{
	const CKnownEntity* DefaultThreatSelection(CBaseBot* bot, const CKnownEntity* first, const CKnownEntity* second);
	const CKnownEntity* SelectNearestThreat(CBaseBot* bot, const CKnownEntity* first, const CKnownEntity* second);
	bool IsImmediateThreat(CBaseBot* bot, const CKnownEntity* threat);
}

namespace botsharedutils::search
{
	// Entity pointer, path finding cost
	using EntitySearchData = std::pair<CBaseEntity*, float>;
	using SearchData = std::vector<EntitySearchData>;
	using SearchPatterns = std::vector<std::string>;

	/**
	 * @brief Utility class for searching and collecting reachable entities.
	 * 
	 * This does a breadth-first search.
	 * @tparam BotClass Bot class.
	 * @tparam NavClass Nav area class.
	 */
	template <typename BotClass, typename NavClass>
	class SearchReachableEntities
	{
	public:
		using Collector = botsharedutils::IsReachableAreas;

		SearchReachableEntities(BotClass* bot, const float maxdist = -1.0f) :
			m_position(0.0f, 0.0f, 0.0f)
		{
			m_bot = bot;
			m_maxdist = maxdist;
			m_area = nullptr;
		}

		SearchReachableEntities(BotClass* bot, SearchPatterns&& patterns, const float maxdist = -1.0f) :
			m_patterns(patterns), m_position(0.0f, 0.0f, 0.0f)
		{
			m_bot = bot;
			m_maxdist = maxdist;
			m_area = nullptr;
		}

		virtual ~SearchReachableEntities() = default;

		// Adds an entity classname pattern to search. Example: item_healthkit, item_armor*
		void AddSearchPattern(const char* pattern) { m_patterns.emplace_back(pattern); }
		void AssignCollector(Collector* collector) { m_collector.reset(collector); }

		BotClass* GetBot() const { return m_bot; }
		void SetMaximumSearchDistance(float maxdist) { m_maxdist = maxdist; }
		float GetMaximumSearchDistance() const { return m_maxdist; }
		// Vector of search results containing reachable entities and the travel cost to reach it
		const SearchData& GetSearchResult() const { return m_data; }

		void DoSearch()
		{
			if (!m_collector)
			{
				m_collector = std::make_unique<Collector>(static_cast<CBaseBot*>(m_bot), GetMaximumSearchDistance());
			}
			
			m_collector->Execute();

			for (const std::string& pattern : m_patterns)
			{
				UtilHelpers::ForEachEntityOfClassname(pattern.c_str(), *this);
			}
		}

		bool operator()(int index, edict_t* edict, CBaseEntity* entity)
		{
			if (entity && IsSelected(entity))
			{
				float cost = -1.0f;

				if (m_collector->IsReachable(static_cast<CNavArea*>(GetEntityNavArea()), &cost))
				{
					m_data.emplace_back(entity, cost);
				}
			}

			return true;
		}

	protected:
		virtual bool IsSelected(CBaseEntity* entity)
		{
			m_position = UtilHelpers::getWorldSpaceCenter(entity);
			m_area = static_cast<NavClass*>(TheNavMesh->GetNearestNavArea(m_position, 512.0f));

			if (IsEntityValid(entity, m_area) && IsPossible(entity, m_bot, m_area))
			{
				return true;
			}

			return false;
		}

		// checks if the entity is valid
		virtual bool IsEntityValid(CBaseEntity* entity, NavClass* area)
		{
			if (!area)
			{
				return false;
			}

			return true;
		}

		// checks if it's possible for the given bot
		virtual bool IsPossible(CBaseEntity* entity, BotClass* bot, NavClass* area)
		{
			if (!bot->GetMovementInterface()->IsAreaTraversable(static_cast<CNavArea*>(area)))
			{
				return false;
			}

			return true;
		}

		NavClass* GetEntityNavArea() const { return m_area; }
		const Vector& GetEntityPosition() const { return m_position; }
		void SetEntityNavArea(NavClass* area) { m_area = area; }
		void SetEntityPosition(const Vector& vec) { m_position = vec; }

	private:
		BotClass* m_bot;
		SearchData m_data;
		SearchPatterns m_patterns;
		float m_maxdist;
		NavClass* m_area;
		Vector m_position;
		std::unique_ptr<Collector> m_collector;
	};
}

#endif // !__NAVBOT_BASE_BOT_SHARED_UTILS_H_
