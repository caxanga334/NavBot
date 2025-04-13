#include <extension.h>
#include <util/entprops.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <navmesh/nav_pathfind.h>
#include <bot/bot_shared_utils.h>
#include "tf2bot_task_sniper_snipe_area.h"

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
	constexpr auto SNIPER_FIRE_DOT_TOLERANCE = 0.98f;

	EquipAndScope(bot);

	if (threat)
	{
		if (threat->IsVisibleNow())
		{
			m_boredTimer.Reset();

			botsharedutils::aiming::SelectDesiredAimSpotForTarget(bot, threat->GetEntity());
			bot->GetControlInterface()->AimAt(threat->GetEntity(), IPlayerController::LOOK_COMBAT, 0.5f, "Looking at visible threat!");

			if (m_fireWeaponDelay.IsElapsed() && bot->IsLookingTowards(threat->GetEntity(), SNIPER_FIRE_DOT_TOLERANCE))
			{
				m_fireWeaponDelay.StartRandom(0.7f, 1.6f);
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

void CTF2BotSniperSnipeAreaTask::BuildLookPoints(CTF2Bot* me)
{
	if (m_waypoint != nullptr)
	{
		// Use waypoint origin but the eye's Z coordinate.
		Vector eyePos = me->GetEyeOrigin();
		Vector start = m_waypoint->GetOrigin();
		start.z = eyePos.z;

		m_waypoint->ForEveryAngle([this, &me, &start](const QAngle& angle) {
			Vector forward;
			AngleVectors(angle, &forward);
			forward.NormalizeInPlace();

			Vector end = start + (forward * 512.0f);
			m_lookPoints.push_back(end);
		});

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