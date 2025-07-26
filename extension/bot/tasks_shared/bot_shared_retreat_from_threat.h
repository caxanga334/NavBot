#ifndef __NAVBOT_BOT_TASK_SHARED_RETREAT_FROM_THREAT_H_
#define __NAVBOT_BOT_TASK_SHARED_RETREAT_FROM_THREAT_H_

#include <bot/basebot.h>
#include <bot/basebot_pathcost.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <sdkports/sdk_timers.h>
#include <bot/bot_shared_utils.h>

template <typename BT, typename CT = CBaseBotPathCost>
class CBotSharedRetreatFromThreatTask : public AITask<BT>
{
public:
	CBotSharedRetreatFromThreatTask(BT* bot, botsharedutils::SelectRetreatArea::RetreatAreaPreference preference, const float noThreatTime = 3.0f, const float timeout = 90.0f, const float minDistance = 512.0f, const float maxRetreatDistance = 8192.0f) :
		m_pathcost(bot)
	{
		m_pathcost.SetRouteType(FASTEST_ROUTE);
		m_escapeTime = noThreatTime;
		m_timeoutTimer.Start(timeout);
		m_preference = preference;
		m_minDistance = minDistance;
		m_maxRetreatDistance = maxRetreatDistance;
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override;
	TaskResult<BT> OnTaskUpdate(BT* bot) override;
	TaskEventResponseResult<BT> OnMoveToSuccess(BT* bot, CPath* path) override;

	const char* GetName() const override { return "RetreatFromThreat"; }
private:
	CT m_pathcost;
	CMeshNavigatorAutoRepath m_nav;
	Vector m_goal;
	float m_escapeTime;
	float m_minDistance;
	float m_maxRetreatDistance;
	IntervalTimer m_threatVisibleTimer;
	CountdownTimer m_timeoutTimer;
	botsharedutils::SelectRetreatArea::RetreatAreaPreference m_preference;
};

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedRetreatFromThreatTask<BT, CT>::OnTaskStart(BT* bot, AITask<BT>* pastTask)
{
	botsharedutils::SelectRetreatArea collector(bot, m_preference, m_minDistance, m_maxRetreatDistance);
	collector.Execute();

	if (!collector.HasFoundRetreatArea())
	{
		return AITask<BT>::Done("Failed to find a spot to retreat!");
	}

	m_goal = collector.GetRetreatToArea()->GetRandomPoint();
	m_timeoutTimer.Reset();
	m_threatVisibleTimer.Start();
	return AITask<BT>::Continue();
}

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedRetreatFromThreatTask<BT, CT>::OnTaskUpdate(BT* bot)
{
	const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(false);

	if (threat)
	{
		if (threat->IsVisibleNow())
		{
			m_threatVisibleTimer.Start();
		}
		else if (threat->GetTimeSinceLastInfo() < m_escapeTime)
		{
			// aim at the last known position if I saw it for less than the escape time
			bot->GetControlInterface()->AimAt(threat->GetLastKnownPosition(), IPlayerController::LOOK_ALERT, 1.0f, "Looking at primary threat last known position!");
		}
	}

	if (m_threatVisibleTimer.IsGreaterThen(m_escapeTime))
	{
		return AITask<BT>::Done("I have escaped!");
	}

	m_nav.Update(bot, m_goal, m_pathcost);

	return AITask<BT>::Continue();
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedRetreatFromThreatTask<BT, CT>::OnMoveToSuccess(BT* bot, CPath* path)
{
	botsharedutils::SelectRetreatArea collector(bot, m_preference, m_minDistance, m_maxRetreatDistance);
	collector.Execute();

	if (collector.HasFoundRetreatArea())
	{
		m_goal = collector.GetRetreatToArea()->GetRandomPoint();
	}

	return AITask<BT>::TryContinue();
}

#endif // !__NAVBOT_BOT_TASK_SHARED_RETREAT_FROM_THREAT_H_
