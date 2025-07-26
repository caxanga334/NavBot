#ifndef __NAVBOT_BOT_SHARED_TAKE_COVER_FROM_SPOT_TASK_H_
#define __NAVBOT_BOT_SHARED_TAKE_COVER_FROM_SPOT_TASK_H_

#include <extension.h>
#include <sdkports/sdk_timers.h>
#include <bot/basebot.h>
#include <bot/basebot_pathcost.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <bot/bot_shared_utils.h>
#include <navmesh/nav_area.h>

template <typename BT, typename CT = CBaseBotPathCost>
class CBotSharedTakeCoverFromSpotTask : public AITask<BT>
{
public:
	CBotSharedTakeCoverFromSpotTask(BT* bot, const Vector& fromSpot, const float radius, const bool checkLOS, const bool checkCorners, const float maxSearchRange, AITask<BT>* nextTask = nullptr) :
		m_collector(fromSpot, radius, checkLOS, checkCorners, maxSearchRange, bot), m_pathcost(bot), m_goal(0.0f, 0.0f, 0.0f)
	{
		m_task = nextTask;
	}

	~CBotSharedTakeCoverFromSpotTask() override
	{
		if (m_task != nullptr)
		{
			delete m_task;
		}
	}

	QueryAnswerType ShouldHurry(CBaseBot* me) override { return ANSWER_YES; }

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override;
	TaskResult<BT> OnTaskUpdate(BT* bot) override;

	TaskEventResponseResult<BT> OnMoveToSuccess(BT* bot, CPath* path) override;

	const char* GetName() const override { return "TakeCoverFromSpot"; }
private:
	botsharedutils::FindCoverCollector m_collector;
	CT m_pathcost;
	AITask<BT>* m_task; // optional task for when the bot reaches cover
	CMeshNavigator m_nav;
	Vector m_goal;
};

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedTakeCoverFromSpotTask<BT, CT>::OnTaskStart(BT* bot, AITask<BT>* pastTask)
{
	m_collector.Execute();

	if (m_collector.IsCollectedAreasEmpty())
	{
		if (m_task)
		{
			AITask<BT>* task = m_task;
			m_task = nullptr;
			return AITask<BT>::SwitchTo(task, "Failed to find cover spot!");
		}
		else
		{
			return AITask<BT>::Done("Failed to find cover spot!");
		}
	}

	CNavArea* area = m_collector.GetRandomCollectedArea();
	m_goal = area->GetCenter();

	m_pathcost.SetRouteType(FASTEST_ROUTE);

	return AITask<BT>::Continue();
}

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedTakeCoverFromSpotTask<BT, CT>::OnTaskUpdate(BT* bot)
{
	if (m_nav.NeedsRepath())
	{
		m_nav.StartRepathTimer();
		m_nav.ComputePathToPosition(bot, m_goal, m_pathcost);
	}

	m_nav.Update(bot);

	return AITask<BT>::Continue();
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedTakeCoverFromSpotTask<BT, CT>::OnMoveToSuccess(BT* bot, CPath* path)
{
	if (m_task)
	{
		AITask<BT>* task = m_task;
		m_task = nullptr;
		return AITask<BT>::TrySwitchTo(task, PRIORITY_HIGH, "Cover spot reached!");
	}

	return AITask<BT>::TryDone(PRIORITY_HIGH, "Cover spot reached!");
}

#endif // !__NAVBOT_BOT_SHARED_TAKE_COVER_FROM_SPOT_TASK_H_

