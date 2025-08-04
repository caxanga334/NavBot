#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/entprops.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <navmesh/nav_pathfind.h>
#include <bot/bot_shared_utils.h>
#include <bot/tasks_shared/bot_shared_retreat_from_threat.h>
#include "tf2bot_task_sniper_snipe_area.h"

constexpr auto SNIPER_THREAT_TOO_CLOSE_FOR_CONFORT_RANGE = 600.0f;

TaskResult<CTF2Bot> CTF2BotSniperSnipeAreaTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	m_boredTimer.StartRandom(15.0f, 60.0f);
	m_changeAnglesTimer.StartRandom(5.0f, 10.0f);
	m_fireWeaponDelay.Start(0.1f);

	BuildLookPoints(bot);

	if (!m_lookPoints.empty())
	{
		const Vector& lookat = m_lookPoints[randomgen->GetRandomInt<size_t>(0, m_lookPoints.size() - 1)];
		bot->GetControlInterface()->AimAt(lookat, IPlayerController::LOOK_ALERT, 5.0f, "Sniper: Looking at snipe angles.");
	}

	if (m_waypoint != nullptr)
	{
		m_waypoint->Use(bot, m_boredTimer.GetRemainingTime() + 1.0f);
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotSniperSnipeAreaTask::OnTaskUpdate(CTF2Bot* bot)
{
	auto threat = bot->GetSensorInterface()->GetPrimaryKnownThreat();

	EquipAndScope(bot);

	if (m_waypoint && m_waypoint->HasFlags(CWaypoint::BaseFlags::BASEFLAGS_CROUCH))
	{
		bot->GetControlInterface()->PressCrouchButton();
	}

	if (threat)
	{
		if (threat->IsVisibleNow())
		{
			// Retreat if an enemy is too close
			if (bot->GetDifficultyProfile()->GetAggressiveness() < 75 && bot->GetRangeTo(threat->GetLastKnownPosition()) <= SNIPER_THREAT_TOO_CLOSE_FOR_CONFORT_RANGE)
			{
				return SwitchTo(new CBotSharedRetreatFromThreatTask<CTF2Bot, CTF2BotPathCost>(bot, botsharedutils::SelectRetreatArea::RetreatAreaPreference::FURTHEST), "Threat too close, backing off!");
			}

			m_boredTimer.Reset();

			botsharedutils::aiming::SelectDesiredAimSpotForTarget(bot, threat->GetEntity());
			bot->GetControlInterface()->AimAt(threat->GetEntity(), IPlayerController::LOOK_COMBAT, 0.5f, "Looking at visible threat!");

			if (m_fireWeaponDelay.IsElapsed() && bot->GetControlInterface()->IsAimOnTarget())
			{
				m_fireWeaponDelay.StartRandom(1.5f, 2.3f);
				bot->GetControlInterface()->PressAttackButton();
			}
		}
		else if (threat->GetTimeSinceLastVisible() < 3.0f)
		{
			bot->GetControlInterface()->AimAt(threat->GetLastKnownPosition(), IPlayerController::LOOK_DANGER, 0.5f, "Looking at threat last know position!");
		}
	}
	else
	{
		if (m_boredTimer.IsElapsed())
		{
			return Done("Bored timer expired, no visible threat!");
		}

		if (m_changeAnglesTimer.IsElapsed())
		{
			m_changeAnglesTimer.StartRandom(5.0f, 10.0f);

			if (m_lookPoints.empty())
			{
				QAngle currentAngles = bot->GetEyeAngles();
				float y = randomgen->GetRandomReal<float>(-25.0f, 25.0f);
				currentAngles[YAW] += y;
				Vector forward;
				AngleVectors(currentAngles, &forward);
				Vector lookat = bot->GetEyeOrigin() + (forward * 512.0f);
				bot->GetControlInterface()->AimAt(lookat, IPlayerController::LOOK_ALERT, 5.0f, "Sniper: Looking at snipe angles.");
			}
			else
			{
				const Vector& lookat = m_lookPoints[randomgen->GetRandomInt<size_t>(0, m_lookPoints.size() - 1)];
				bot->GetControlInterface()->AimAt(lookat, IPlayerController::LOOK_ALERT, 5.0f, "Sniper: Looking at snipe angles.");
			}
		}
	}

	return Continue();
}

void CTF2BotSniperSnipeAreaTask::OnTaskEnd(CTF2Bot* bot, AITask<CTF2Bot>* nextTask)
{
	if (bot->IsUsingSniperScope())
	{
		bot->GetControlInterface()->PressSecondaryAttackButton();
	}

	if (m_waypoint != nullptr)
	{
		m_waypoint->StopUsing(bot);
	}
}

bool CTF2BotSniperSnipeAreaTask::OnTaskPause(CTF2Bot* bot, AITask<CTF2Bot>* nextTask)
{
	// destroy on pause
	return false;
}

QueryAnswerType CTF2BotSniperSnipeAreaTask::ShouldAttack(CBaseBot* me, const CKnownEntity* them)
{
	// This task will take over the main task control of weapon firing
	return ANSWER_NO;
}

QueryAnswerType CTF2BotSniperSnipeAreaTask::ShouldRetreat(CBaseBot* me)
{
	// Snipers don't retreat unless low on health or low on ammo
	if (me->GetHealthState() == CBaseBot::HealthState::HEALTH_OK && !me->GetInventoryInterface()->IsAmmoLow(false))
	{
		return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}

void CTF2BotSniperSnipeAreaTask::BuildLookPoints(CTF2Bot* me)
{
	if (m_waypoint != nullptr)
	{
		// Use waypoint origin but the eye's Z coordinate.
		Vector eyePos = me->GetEyeOrigin();
		Vector start = m_waypoint->GetOrigin();
		start.z = eyePos.z;
		CWaypoint::BuildAimSpotFunctor func{ start, &m_lookPoints };
		m_waypoint->ForEveryAngle(func);
		return;
	}

	botsharedutils::AimSpotCollector collector(me);
	collector.Execute();
	collector.ExtractAimSpots(m_lookPoints);
}

void CTF2BotSniperSnipeAreaTask::EquipAndScope(CTF2Bot* me)
{
	me->SelectWeapon(me->GetWeaponOfSlot(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Primary));

	if (!me->IsUsingSniperScope())
	{
		me->GetControlInterface()->PressSecondaryAttackButton();
	}
}