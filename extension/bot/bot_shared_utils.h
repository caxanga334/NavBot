#ifndef __NAVBOT_BASE_BOT_SHARED_UTILS_H_
#define __NAVBOT_BASE_BOT_SHARED_UTILS_H_

#include <vector>
#include <unordered_set>
#include <navmesh/nav_pathfind.h>
#include <sdkports/sdk_traces.h>

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
		Vector m_vecOffset;
		TraceFilterVisibleSimple m_filter;
	};

	/**
	 * @brief Searches for Nav areas that are visible to a given position.
	 */
	class RandomDefendSpotCollector : public INavAreaCollector<CNavArea>
	{
	public:
		RandomDefendSpotCollector(const Vector& spot, CBaseBot* bot);

		bool ShouldCollect(CNavArea* area) override;

	private:
		CBaseBot* m_bot;
		Vector m_spot;
		Vector m_vecOffset;
		TraceFilterVisibleSimple m_filter;
	};
}

#endif // !__NAVBOT_BASE_BOT_SHARED_UTILS_H_
