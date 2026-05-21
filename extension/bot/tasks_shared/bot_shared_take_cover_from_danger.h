#ifndef __NAVBOT_BOT_SHARED_TAKE_COVER_FROM_DANGER_TASK_H_
#define __NAVBOT_BOT_SHARED_TAKE_COVER_FROM_DANGER_TASK_H_

#include <extension.h>
#include <sdkports/sdk_timers.h>
#include <bot/basebot.h>
#include <bot/basebot_pathcost.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <bot/bot_shared_utils.h>
#include <navmesh/nav_area.h>

template <typename BT, typename CT = CBaseBotPathCost>
class CBotSharedTakeCoverFromDangerTask : public AITask<BT>
{
public:
	/**
	 * @brief Constructor.
	 * @param danger Danger entity.
	 * @param pos Position the projectile will hit (used to determine where to hide from).
	 * @param useLOS If true, check for LOS when searching for cover.
	 */
	CBotSharedTakeCoverFromDangerTask(CBaseEntity* danger, const Vector& pos, const bool useLOS = false) :
		m_goal(0.0f, 0.0f, 0.0f), m_danger(danger), m_source(pos), m_useLOS(useLOS)
	{
	}

	QueryAnswerType ShouldHurry(CBaseBot* me) override { return ANSWER_YES; }
	QueryAnswerType ShouldRetreat(CBaseBot* me) override { return ANSWER_NO; }
	QueryAnswerType ShouldPickup(CBaseBot* me, CBaseEntity* item) override { return ANSWER_NO; }
	QueryAnswerType ShouldSeekAndDestroy(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_NO; }
	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_YES; }
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override { return ANSWER_YES; }

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override
	{
		const float blastradius = extmanager->GetMod()->GetModSettings()->GetDefaultBlastRadius();
		const float maxradius = std::clamp(blastradius * 12.0f, 1024.0f, 16384.0f);
		botsharedutils::FindCoverCollector collector(m_source, blastradius, m_useLOS, true, maxradius, bot);
		collector.Execute();

		if (collector.IsCollectedAreasEmpty())
		{
			return AITask<BT>::Done("Failed to find a cover position!");
		}

		CNavArea* coverArea = collector.GetNearestCollectedArea();
		m_goal = coverArea->GetRandomPoint();

		return AITask<BT>::Continue();
	}

	TaskResult<BT> OnTaskUpdate(BT* bot) override
	{
		CBaseEntity* danger = m_danger.Get();

		if (!danger)
		{
			return AITask<BT>::Done("Danger entity is NULL!");
		}

		if (!m_nav.IsValid() || m_nav.NeedsRepath())
		{
			CT cost(bot);
			cost.SetRouteType(RouteType::FASTEST_ROUTE);
			m_nav.ComputePathToPosition(bot, m_goal, cost);
			m_nav.StartRepathTimer();
		}

		m_nav.Update(bot);
		return AITask<BT>::Continue();
	}

	TaskEventResponseResult<BT> OnMoveToSuccess(BT* bot, CPath* path) override
	{
		return AITask<BT>::TryDone(PRIORITY_CRITICAL, "Reached cover.");
	}

	TaskEventResponseResult<BT> OnDangerousEntityChanged(BT* bot, CBaseEntity* newent, CBaseEntity* oldent) override
	{
		// don't take cover again
		return AITask<BT>::TryToMaintain(PRIORITY_CRITICAL);
	}

	const char* GetName() const override { return "TakeCoverFromDanger"; }
private:
	CMeshNavigator m_nav;
	Vector m_goal;
	CHandle<CBaseEntity> m_danger;
	Vector m_source; // danger hit pos
	bool m_useLOS;
};

#endif // !__NAVBOT_BOT_SHARED_TAKE_COVER_FROM_DANGER_TASK_H_
