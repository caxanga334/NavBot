#include <extension.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <navmesh/nav_pathfind.h>
#include "tf2bot_task_sniper_snipe_area.h"

class SnipingAreaCollector : public INavAreaCollector<CTFNavArea>
{
public:
	SnipingAreaCollector(CTF2Bot* bot) :
		INavAreaCollector<CTFNavArea>(),
		m_offset(0.0f, 0.0f, 36.0f),
		m_filter(bot->GetEntity(), COLLISION_GROUP_NONE)
	{
		m_me = bot;
		m_origin = bot->GetEyeOrigin();
		SetStartArea(static_cast<CTFNavArea*>(bot->GetLastKnownNavArea()));
		SetTravelLimit(bot->GetDifficultyProfile().GetMaxVisionRange());
	}

	bool ShouldCollect(CTFNavArea* area) override;

private:
	CTF2Bot* m_me;
	Vector m_origin;
	Vector m_offset;
	trace::CTraceFilterNoNPCsOrPlayers m_filter;
	trace_t m_tr;
};

bool SnipingAreaCollector::ShouldCollect(CTFNavArea* area)
{
	trace::line(m_origin, area->GetCenter() + m_offset, MASK_SHOT, &m_filter, m_tr);

	if (m_tr.fraction < 1.0f)
	{
		return false; // area not visible
	}

	// if the current Area is visible but any connectedArea, this is an edge area.
	bool isEdgeArea = false;

	area->ForEachConnectedArea([this, &isEdgeArea](CNavArea* connectedArea) {
		if (isEdgeArea)
		{
			return; // skip trace if edge id found
		}

		trace::line(m_origin, connectedArea->GetCenter() + m_offset, MASK_SHOT, &m_filter, m_tr);

		if (m_tr.fraction < 1.0f)
		{
			isEdgeArea = true; // not visible, mark as edge
		}
	});

	return isEdgeArea;
}

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
	const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat();
	constexpr auto SNIPER_FIRE_DOT_TOLERANCE = 0.98f;

	EquipAndScope(bot);

	if (threat != nullptr)
	{
		if (threat->IsVisibleNow())
		{
			m_boredTimer.Reset();

			bot->GetControlInterface()->AimAt(threat->GetEdict(), IPlayerController::LOOK_COMBAT, 0.5f, "Looking at visible threat!");

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
		Vector start = me->GetEyeOrigin();

		m_waypoint->ForEveryAngle([this, &me, &start](const QAngle& angle) {
			Vector forward;
			AngleVectors(angle, &forward);
			forward.NormalizeInPlace();

			Vector end = start + (forward * 512.0f);
			m_lookPoints.push_back(end);
		});

		return;
	}

	SnipingAreaCollector collector(me);
	collector.Execute();

	auto& allareas = collector.GetCollectedAreas();
	Vector offset(0.0f, 0.0f, 70.0f);


	for (auto area : allareas)
	{
		m_lookPoints.push_back(area->GetCenter() + offset);
	}
}

void CTF2BotSniperSnipeAreaTask::EquipAndScope(CTF2Bot* me)
{
	me->SelectWeapon(me->GetWeaponOfSlot(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Primary));

	if (!me->IsUsingSniperScope())
	{
		me->GetControlInterface()->PressSecondaryAttackButton();
	}
}