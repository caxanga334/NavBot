#ifndef __NAVBOT_BOT_SHARED_SNIPE_TASK_
#define __NAVBOT_BOT_SHARED_SNIPE_TASK_

#include <vector>
#include <functional>
#include <extension.h>
#include <sdkports/sdk_ehandle.h>
#include <sdkports/sdk_timers.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_waypoint.h>
#include <bot/basebot.h>
#include <bot/basebot_pathcost.h>
#include <bot/bot_shared_utils.h>
#include <bot/interfaces/path/meshnavigator.h>

/**
 * @brief A task for snipers to snipe. Can be given a position to overwatch or a waypoint to snipe from.
 * 
 * Assumes the sniper needs to be scoped in and does so with the 'mouse2' (secondary attack/right mouse button).
 * 
 * Uses CBaseBot::IsScopedIn() virtual function to determine if the bot is scoped in.
 * @tparam BT Bot class.
 * @tparam CT Bot path cost functor class.
 */
template <typename BT, typename CT = CBaseBotPathCost>
class CBotSharedSnipeTask : public AITask<BT>
{
public:
	/**
	 * @brief Constructor
	 * @param bot Bot that will snipe
	 * @param overwatchPos Position the sniper will overwatch.
	 * @param waypoint Waypoint to snipe from. If a waypoint is passed, overwatchPos is ignored and can be NULL.
	 * @param sniper The weapon to use. Bots will switch to it automatically when they reach their destination.
	 * @param duration How long to stay sniping.
	 */
	CBotSharedSnipeTask(BT* bot, const Vector* overwatchPos, CWaypoint* waypoint, CBaseEntity* sniper, const float duration);

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override;
	TaskResult<BT> OnTaskUpdate(BT* bot) override;
	void OnTaskEnd(BT* bot, AITask<BT>* nextTask) override;

	TaskEventResponseResult<BT> OnMoveToSuccess(BT* bot, CPath* path) override;

	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override;

	const char* GetName() const override { return "Snipe"; }
private:
	CT m_pathcost;
	CMeshNavigator m_nav;
	CountdownTimer m_repathtimer;
	float m_snipeduration;
	CountdownTimer m_timeout;
	CHandle<CBaseEntity> m_sniperweapon;
	Vector m_overwatchpos;
	Vector m_goal;
	std::vector<Vector> m_aimvec;
	CountdownTimer m_aimtimer;
	CountdownTimer m_scopeintimer;
	CWaypoint* m_waypoint;
	bool m_atgoal;
};

template<typename BT, typename CT>
inline CBotSharedSnipeTask<BT, CT>::CBotSharedSnipeTask(BT* bot, const Vector* overwatchPos, CWaypoint* waypoint, CBaseEntity* sniper, const float duration) :
	m_pathcost(bot), m_sniperweapon(sniper), m_goal(0.0f, 0.0f, 0.0f)
{
	if (overwatchPos)
	{
		m_overwatchpos = *overwatchPos;
		m_waypoint = nullptr;
	}
	else if (waypoint)
	{
		m_overwatchpos = vec3_origin;
		m_waypoint = waypoint;
	}

	m_snipeduration = duration;
	m_atgoal = false;
	m_aimvec.reserve(64);
}

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedSnipeTask<BT, CT>::OnTaskStart(BT* bot, AITask<BT>* pastTask)
{
	if (m_waypoint)
	{
		m_waypoint->ForEveryAngle([this](const QAngle& angle) {
			Vector dir;
			AngleVectors(angle, &dir);
			dir.NormalizeInPlace();
			Vector& aim = m_aimvec.emplace_back();
			aim = (m_waypoint->GetOrigin() + Vector(0.0f, 0.0f, 64.0f)) + (dir * 1024.0f);
		});

		m_goal = m_waypoint->GetRandomPoint();
		m_waypoint->Use(bot, m_snipeduration + 20.0f);

		return AITask<BT>::Continue();
	}

	botsharedutils::RandomSnipingSpotCollector collector{ m_overwatchpos, bot, 4096.0f };

	collector.Execute();
	
	CNavArea* snipeArea = collector.GetSnipingArea();

	if (!snipeArea)
	{
		return AITask<BT>::Done("Failed to find a suitable spot to snipe from!");
	}

	m_goal = snipeArea->GetCenter();

	return AITask<BT>::Continue();
}

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedSnipeTask<BT, CT>::OnTaskUpdate(BT* bot)
{
	CBaseEntity* weapon = m_sniperweapon.Get();

	if (!weapon)
	{
		return AITask<BT>::Done("My weapon is gone!");
	}

	if (m_atgoal)
	{
		if (m_timeout.IsElapsed())
		{
			return AITask<BT>::Done("Done sniping!");
		}

		if (bot->GetActiveWeapon() && bot->GetActiveWeapon() != weapon)
		{
			bot->SelectWeapon(weapon);
		}
		else
		{
			if (m_scopeintimer.IsElapsed())
			{
				m_scopeintimer.Start(1.0f);

				if (!bot->IsScopedIn())
				{
					bot->GetControlInterface()->PressSecondaryAttackButton();
				}
			}
		}
		
		if (m_waypoint && m_waypoint->HasFlags(CWaypoint::BaseFlags::BASEFLAGS_CROUCH))
		{
			bot->GetControlInterface()->PressCrouchButton();
		}

		if (m_aimtimer.IsElapsed())
		{
			m_aimtimer.StartRandom(2.0f, 5.0f);
			const Vector& aimAt = m_aimvec[CBaseBot::s_botrng.GetRandomInt<std::size_t>(0U, m_aimvec.size() - 1U)];
			bot->GetControlInterface()->AimAt(aimAt, IPlayerController::LOOK_INTERESTING, 1.5f, "Looking at snipe aim pos!");
		}
	}
	else
	{
		if (m_repathtimer.IsElapsed())
		{
			m_repathtimer.Start(2.0f);
			m_nav.ComputePathToPosition(bot, m_goal, m_pathcost);
		}

		m_nav.Update(bot);
	}

	return AITask<BT>::Continue();
}

template<typename BT, typename CT>
inline void CBotSharedSnipeTask<BT, CT>::OnTaskEnd(BT* bot, AITask<BT>* nextTask)
{
	if (bot->IsScopedIn())
	{
		bot->GetControlInterface()->PressSecondaryAttackButton();
	}

	if (m_waypoint)
	{
		m_waypoint->StopUsing(bot);
	}
}

template<typename BT, typename CT>
inline TaskEventResponseResult<BT> CBotSharedSnipeTask<BT, CT>::OnMoveToSuccess(BT* bot, CPath* path)
{
	if (!m_atgoal)
	{
		m_atgoal = true;
		m_timeout.Start(m_snipeduration);
		m_scopeintimer.Start(1.0f);

		if (m_aimvec.empty())
		{
			botsharedutils::AimSpotCollector collector{ bot };

			collector.Execute();
			collector.ExtractAimSpots(m_aimvec);
		}

		bot->GetMovementInterface()->DoCounterStrafe();
	}

	return AITask<BT>::TryContinue();
}

template<typename BT, typename CT>
inline QueryAnswerType CBotSharedSnipeTask<BT, CT>::ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon)
{
	if (m_atgoal)
	{
		return ANSWER_NO; // Restrict to the sniper rifle only
	}

	return ANSWER_UNDEFINED;
}

#endif // !__NAVBOT_BOT_SHARED_SNIPE_TASK_
