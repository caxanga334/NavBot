#include <extension.h>
#include <util/entprops.h>
#include <mods/tf2/tf2lib.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/nav/tfnavmesh.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <bot/tf2/tf2bot.h>
#include <entities/tf2/tf_entities.h>
#include "tf2bot_mvm_tasks.h"
#include "tf2bot_mvm_defend.h"

TaskResult<CTF2Bot> CTF2BotMvMDefendTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	CBaseEntity* flag = tf2lib::mvm::GetMostDangerousFlag(false);

	m_flag = flag;

	if (flag == nullptr)
	{
		m_updateGoalTimer.Invalidate();
	}
	else
	{
		m_updateGoalTimer.StartRandom(2.0f, 5.0f);
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMvMDefendTask::OnTaskUpdate(CTF2Bot* bot)
{
	auto threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);
	CBaseEntity* tank = tf2lib::mvm::GetMostDangerousTank();

	if (tank)
	{
		auto myclass = bot->GetMyClassType();

		switch (myclass)
		{
		case TeamFortress2::TFClass_Soldier:
			[[fallthrough]];
		case TeamFortress2::TFClass_DemoMan:
			[[fallthrough]];
		case TeamFortress2::TFClass_Pyro:
		{
			return PauseFor(new CTF2BotMvMTankBusterTask(tank), "Destroying tanks!");
		}
		default:
			break;
		}
	}

	if (threat)
	{
		return PauseFor(new CTF2BotMvMCombatTask, "Attacking enemies!");
	}

	CBaseEntity* ent = m_flag.Get();

	if (m_updateGoalTimer.IsElapsed())
	{
		m_updateGoalTimer.StartRandom(2.0f, 5.0f);
		ent = tf2lib::mvm::GetMostDangerousFlag(false);
		m_flag = ent;
	}

	if (ent == nullptr)
	{
		return Continue();
	}

	tfentities::HCaptureFlag flag(ent);
	Vector pos = flag.GetPosition();

	if (!m_nav.IsValid() || m_repathTimer.IsElapsed())
	{
		m_repathTimer.Start(0.5f);
		
		CTF2BotPathCost cost(bot);
		m_nav.ComputePathToPosition(bot, pos, cost);
	}

	float range = bot->GetRangeTo(pos);

	if (range > 300.0f || !bot->GetSensorInterface()->IsLineOfSightClear(pos))
	{
		m_nav.Update(bot);
	}

	if (range < 512.0f)
	{
		bot->GetControlInterface()->AimAt(pos, IPlayerController::LOOK_INTERESTING, 0.2f, "Looking at flag position!");
	}

	return Continue();
}
