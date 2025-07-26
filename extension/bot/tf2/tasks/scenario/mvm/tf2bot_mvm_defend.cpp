#include NAVBOT_PCH_FILE
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
#include "tf2bot_mvm_guard_dropped_bomb_task.h"

CTF2BotMvMDefendTask::CTF2BotMvMDefendTask() :
	m_targetPos()
{
}

TaskResult<CTF2Bot> CTF2BotMvMDefendTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	m_updateTargetTimer.Invalidate();
	UpdateTarget();

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMvMDefendTask::OnTaskUpdate(CTF2Bot* bot)
{
	UpdateTarget();

	CBaseEntity* target = m_target.Get();

	if (!target)
	{
		auto threat = bot->GetSensorInterface()->GetPrimaryKnownThreat();

		if (!threat)
		{
			return Continue();
		}

		target = threat->GetEntity();
	}

	TeamFortress2::TFClassType myclass = bot->GetMyClassType();

	// soldiers, pyros and demos go after the tank
	if ((myclass == TeamFortress2::TFClass_Soldier || myclass == TeamFortress2::TFClass_DemoMan || myclass == TeamFortress2::TFClass_Pyro) && UtilHelpers::FClassnameIs(target, "tank_boss"))
	{
		return PauseFor(new CTF2BotMvMTankBusterTask(target), "Seek and destroy MvM tank!");
	}

	bool isFlag = UtilHelpers::FClassnameIs(target, "item_teamflag");

	if (!m_nav.IsValid() || m_nav.NeedsRepath())
	{
		if (isFlag)
		{
			m_targetPos = tf2lib::GetFlagPosition(target);
		}
		else
		{
			m_targetPos = UtilHelpers::getEntityOrigin(target);
		}

		m_nav.StartRepathTimer();
		CTF2BotPathCost cost(bot);
		m_nav.ComputePathToPosition(bot, m_targetPos, cost);
	}
	
	const CTF2BotWeapon* weapon = bot->GetInventoryInterface()->GetActiveTFWeapon();
	float maxrange = weapon ? weapon->GetTF2Info()->GetAttackRange() : 512.0f;
	float range = bot->GetRangeTo(m_targetPos);

	if (range > 128.0f && m_lookAtTimer.IsElapsed())
	{
		m_lookAtTimer.Start(5.0f);
		bot->GetControlInterface()->AimAt(m_targetPos, IPlayerController::LOOK_ALERT, 1.0f, "Looking at mvm target!");
	}

	if (range > maxrange || !bot->GetSensorInterface()->IsLineOfSightClear(m_targetPos))
	{
		m_nav.Update(bot);
	}
	else if (isFlag)
	{
		tfentities::HCaptureFlag cf(target);

		if (cf.IsDropped())
		{
			return PauseFor(new CTF2BotMvMGuardDroppedBombTask(target), "Guarding dropped bomb!");
		}
	}

	return Continue();
}

void CTF2BotMvMDefendTask::UpdateTarget()
{
	if (m_updateTargetTimer.IsElapsed())
	{
		m_updateTargetTimer.Start(CBaseBot::s_botrng.GetRandomReal<float>(2.0f, 5.0f));
		const Vector& hatchPos = CTeamFortress2Mod::GetTF2Mod()->GetMvMBombHatchPosition();

		CBaseEntity* tank = tf2lib::mvm::GetMostDangerousTank();

		if (tank)
		{
			// a tank is present, see if the bomb carrier is closer to the hatch
			CBaseEntity* flag = tf2lib::mvm::GetMostDangerousFlag(true);

			if (!flag)
			{
				m_target = tank;
			}
			else
			{
				const Vector& tankPos = UtilHelpers::getEntityOrigin(tank);
				const Vector& flagPos = tf2lib::GetFlagPosition(flag);

				if ((hatchPos - tankPos).LengthSqr() < (hatchPos - flagPos).LengthSqr())
				{
					m_target = tank;
				}
				else
				{
					m_target = flag;
				}
			}
		}
		else
		{
			CBaseEntity* flag = tf2lib::mvm::GetMostDangerousFlag(false);

			if (flag)
			{
				m_target = flag;
			}
		}
	}
}
