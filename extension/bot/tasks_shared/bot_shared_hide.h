#ifndef __NAVBOT_BOT_SHARED_HIDE_TASK_H_
#define __NAVBOT_BOT_SHARED_HIDE_TASK_H_

#include <vector>
#include <extension.h>
#include <sdkports/sdk_timers.h>
#include <navmesh/nav_mesh.h>
#include <bot/basebot.h>
#include <bot/basebot_pathcost.h>
#include <bot/bot_shared_utils.h>
#include <bot/interfaces/path/meshnavigator.h>

template <typename BT, typename CT = CBaseBotPathCost>
class CBotSharedHideTask : public AITask<BT>
{
public:
	CBotSharedHideTask(BT* bot, const HidingSpot* hidingSpot, const float maxHideTime) :
		m_pathcost(bot)
	{
		m_hidingspot = hidingSpot;
		m_hidelength = maxHideTime;
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override
	{
		m_pathcost.SetRouteType(RouteType::SAFEST_ROUTE);
		const Vector& pos = m_hidingspot->GetPosition();

		if (!m_nav.ComputePathToPosition(bot, pos, m_pathcost))
		{
			return AITask<BT>::Done("No path found to hiding spot!");
		}
		else
		{
			m_nav.StartRepathTimer();
		}

		return AITask<BT>::Continue();
	}

	TaskResult<BT> OnTaskUpdate(BT* __bot) override
	{
		CBaseBot* bot = __bot;

		if (m_hideTimer.HasStarted())
		{
			if (m_aimTimer.IsElapsed() && !m_aimSpots.empty())
			{
				m_aimTimer.StartRandom(3.0f, 5.0f);
				const Vector& spot = m_aimSpots[CBaseBot::s_botrng.GetRandomInt<std::size_t>(0U, m_aimSpots.size() - 1U)];
				bot->GetControlInterface()->AimAt(spot, IPlayerController::LOOK_SEARCH, 2.5f, "Looking at potential enemy position!");
				bot->GetCombatInterface()->StopLookingAround();
			}

			if (m_hideTimer.IsElapsed())
			{
				return AITask<BT>::Done("Hiding timer elapsed!");
			}

			return AITask<BT>::Continue();
		}

		if (m_nav.NeedsRepath())
		{
			m_nav.ComputePathToPosition(bot, m_hidingspot->GetPosition(), m_pathcost);
			m_nav.StartRepathTimer();
		}

		m_nav.Update(bot);

		return AITask<BT>::Continue();
	}

	TaskEventResponseResult<BT> OnMoveToSuccess(BT* bot, CPath* path) override
	{

	}


private:
	CT m_pathcost;
	CMeshNavigator m_nav;
	const HidingSpot* m_hidingspot;
	std::vector<Vector> m_aimSpots;
	CountdownTimer m_aimTimer;
	float m_hidelength;
	CountdownTimer m_hideTimer;
};


#endif // !__NAVBOT_BOT_SHARED_HIDE_TASK_H_
