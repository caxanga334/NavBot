#ifndef __NAVBOT_BOT_SHARED_DEFEND_SPOT_TASK_H_
#define __NAVBOT_BOT_SHARED_DEFEND_SPOT_TASK_H_

#include <vector>
#include <extension.h>
#include <sdkports/sdk_timers.h>
#include <sdkports/debugoverlay_shared.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_waypoint.h>
#include <bot/basebot.h>
#include <bot/basebot_pathcost.h>
#include <bot/bot_shared_utils.h>
#include <bot/interfaces/path/meshnavigator.h>
#include "bot_shared_pursue_and_destroy.h"

template <typename BT, typename CT = CBaseBotPathCost>
class CBotSharedDefendSpotTask : public AITask<BT>
{
public:
	/**
	 * @brief Constructor
	 * @param bot Bot.
	 * @param spot Position the bot will defend.
	 * @param maxtime Maximum defend time.
	 * @param endifenemy If true, the task will end early if an enemy is VISIBLE.
	 */
	CBotSharedDefendSpotTask(BT* bot, const Vector& spot, const float maxtime, const bool endifenemy) :
		m_pathCost(bot), m_watchSpot(0.0f, 0.0f, 0.0f)
	{
		m_defendSpot = spot;
		m_maxtime = maxtime;
		m_endonthreat = endifenemy;
		m_reached = false;
		m_waypoint = nullptr;
	}

	CBotSharedDefendSpotTask(BT* bot, CWaypoint* waypoint, const float maxtime, const bool endifenemy) :
		m_pathCost(bot), m_defendSpot(0.0f, 0.0f, 0.0f)
	{
		m_watchSpot = waypoint->GetRandomPoint();
		m_maxtime = maxtime;
		m_endonthreat = endifenemy;
		m_reached = false;
		m_waypoint = waypoint;
		std::vector<Vector> spots;

		auto func = [&waypoint, &spots](const QAngle& angle) {
			Vector dir;
			AngleVectors(angle, &dir);
			dir.NormalizeInPlace();
			Vector aimAt = (waypoint->GetOrigin() + Vector(0.0f, 0.0f, CWaypoint::WAYPOINT_AIM_HEIGHT)) + (dir * 1024.0f);
			spots.push_back(aimAt);
		};

		waypoint->ForEveryAngle(func);

		m_aimSpots.swap(spots);
		waypoint->Use(bot, maxtime + 10.0f);
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override;
	TaskResult<BT> OnTaskUpdate(BT* bot) override;

	TaskEventResponseResult<BT> OnMoveToSuccess(BT* bot, CPath* path) override;

	const char* GetName() const override { return "DefendSpot"; }
private:
	CT m_pathCost;
	Vector m_defendSpot; // position the bot wants to defend
	Vector m_watchSpot; // position the bot will defend from
	float m_maxtime;
	CMeshNavigator m_nav;
	CountdownTimer m_timeout;
	CountdownTimer m_aimtimer;
	std::vector<Vector> m_aimSpots;
	CWaypoint* m_waypoint;
	bool m_endonthreat;
	bool m_reached;

	void OnReachDestination(BT* bot);
};

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedDefendSpotTask<BT, CT>::OnTaskStart(BT* bot, AITask<BT>* pastTask)
{
	botsharedutils::RandomDefendSpotCollector collector(m_defendSpot, bot);
	collector.Execute();

	if (collector.IsCollectedAreasEmpty())
	{
		return AITask<BT>::Done("Failed to find a suitable spot to defend from!");
	}

	CNavArea* area = collector.GetRandomCollectedArea();
	m_watchSpot = area->GetCenter();

	if (!m_nav.ComputePathToPosition(bot, m_watchSpot, m_pathCost, 0.0f, true))
	{
		return AITask<BT>::Done("No path to defend spot!");
	}

	return AITask<BT>::Continue();
}

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedDefendSpotTask<BT, CT>::OnTaskUpdate(BT* bot)
{
	if (m_reached)
	{
		if (m_endonthreat)
		{
			const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

			if (threat)
			{
				return AITask<BT>::SwitchTo(new CBotSharedPursueAndDestroyTask<BT, CT>(bot, threat->GetEntity()), "Visible threat!");
			}
		}

		if (m_timeout.IsElapsed())
		{
			return AITask<BT>::Done("Defend timer elapsed!");
		}

		if (m_aimtimer.IsElapsed())
		{
			m_aimtimer.Start(CBaseBot::s_botrng.GetRandomReal<float>(2.0f, 3.6f));

			if (m_aimSpots.empty())
			{
				bot->GetControlInterface()->AimAt(m_defendSpot, IPlayerController::LOOK_INTERESTING, 1.5f, "Looking at defend spot!");
			}
			else
			{
				const Vector& point = m_aimSpots[CBaseBot::s_botrng.GetRandomInt<std::size_t>(0U, m_aimSpots.size() - 1U)];
				bot->GetControlInterface()->AimAt(point, IPlayerController::LOOK_INTERESTING, 1.5f, "Looking at potential enemy area!");
			}
		}
	}
	else
	{
		if (m_nav.NeedsRepath())
		{
			m_nav.StartRepathTimer();
			m_nav.ComputePathToPosition(bot, m_watchSpot, m_pathCost, 0.0f, true);
		}

		m_nav.Update(bot);
	}

	return AITask<BT>::Continue();
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedDefendSpotTask<BT, CT>::OnMoveToSuccess(BT* bot, CPath* path)
{
	OnReachDestination(bot);
	bot->GetMovementInterface()->DoCounterStrafe();
	return AITask<BT>::TryContinue(PRIORITY_LOW);
}

template<typename BT, typename CT>
inline void CBotSharedDefendSpotTask<BT, CT>::OnReachDestination(BT* bot)
{
	if (m_aimSpots.empty())
	{
		botsharedutils::AimSpotCollector collector(bot);
		collector.Execute();
		collector.ExtractAimSpots(m_aimSpots);

		if (bot->IsDebugging(BOTDEBUG_TASKS))
		{
			for (const Vector& spot : m_aimSpots)
			{
				NDebugOverlay::Sphere(spot, 8.0f, 255, 0, 0, true, m_maxtime + 2.0f);
				NDebugOverlay::Text(spot, "AimSpot", false, m_maxtime + 2.0f);
			}
		}
	}

	m_timeout.Start(m_maxtime);
	m_reached = true;
}

#endif // !__NAVBOT_BOT_SHARED_DEFEND_SPOT_TASK_H_