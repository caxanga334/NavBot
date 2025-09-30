#include NAVBOT_PCH_FILE
#include <bot/interfaces/path/meshnavigator.h>
#include <bot/tf2/tf2bot.h>
#include <bot/bot_shared_utils.h>
#include <mods/tf2/tf2lib.h>
#include <util/entprops.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_area.h>
#include <sdkports/sdk_takedamageinfo.h>
#include <bot/tasks_shared/bot_shared_attack_enemy.h>
#include "tf2bot_demoman_lay_sticky_trap_task.h"

class ShouldDetonateStickiesFunctor
{
public:
	ShouldDetonateStickiesFunctor(const Vector& trap) :
		m_trap(trap)
	{
		m_detonate = false;
	}

	void operator()(const CKnownEntity* known)
	{
		constexpr float BLAST_RADIUS = 200.0f;

		if ((m_trap - known->GetLastKnownPosition()).IsLengthLessThan(BLAST_RADIUS))
		{
			m_detonate = true;
		}
	}

	Vector m_trap;
	bool m_detonate;
};

bool CTF2BotDemomanLayStickyTrapTask::IsPossible(CTF2Bot* bot)
{
	if (bot->GetHealthState() != CBaseBot::HealthState::HEALTH_OK)
	{
		return false;
	}

	const CBotWeapon* stickybomblauncher = bot->GetInventoryInterface()->FindWeaponByClassname("tf_weapon_pipebomblauncher");

	if (!stickybomblauncher)
	{
		return false;
	}

	const int econindex = stickybomblauncher->GetWeaponEconIndex();

	if (econindex == 265)
	{
		return false; // sticky jumper
	}

	if (stickybomblauncher->GetPrimaryAmmoLeft(bot) < 16)
	{
		return false;
	}

	return true;
}

CTF2BotDemomanLayStickyTrapTask::CTF2BotDemomanLayStickyTrapTask(const Vector& trapPos) :
	m_trapLocation(trapPos), m_hideSpot(0.0f, 0.0f, 0.0f), m_aimPos(trapPos)
{
	m_justFired = false;
	m_trapDeployed = false;
	m_maxBombs = 0;
}

TaskResult<CTF2Bot> CTF2BotDemomanLayStickyTrapTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	// detonate any existing stickies before laying another trap
	bot->GetControlInterface()->PressSecondaryAttackButton();

	const CBotWeapon* stickybomblauncher = bot->GetInventoryInterface()->FindWeaponByClassname("tf_weapon_pipebomblauncher");

	if (!stickybomblauncher)
	{
		return Done("I don't own an sticky bomb launcher!");
	}

	// 130 The Scottish Resistance
	if (stickybomblauncher->GetWeaponEconIndex() == 130)
	{
		m_maxBombs = 14;
	}
	else
	{
		m_maxBombs = 8;
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotDemomanLayStickyTrapTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (!m_reloadWaitTimer.IsElapsed())
	{
		return Continue();
	}

	const CBotWeapon* stickybomblauncher = bot->GetInventoryInterface()->FindWeaponByClassname("tf_weapon_pipebomblauncher");

	if (!stickybomblauncher)
	{
		return Done("I don't own an sticky bomb launcher!");
	}

	const CBotWeapon* activeWeapon = bot->GetInventoryInterface()->GetActiveBotWeapon();

	if (activeWeapon != stickybomblauncher)
	{
		bot->GetInventoryInterface()->EquipWeapon(stickybomblauncher);
		return Continue();
	}

	auto input = bot->GetControlInterface();

	if (!stickybomblauncher->IsLoaded())
	{
		input->PressReloadButton();
		m_reloadWaitTimer.Start(4.0f);
	}

	int pipecount = 0;
	entprops->GetEntProp(stickybomblauncher->GetEntity(), Prop_Send, "m_iPipebombCount", pipecount);

	if (m_trapDeployed)
	{
		if (m_trapEndTimer.IsElapsed())
		{
			return Done("Bored!");
		}

		if (bot->GetRangeTo(m_hideSpot) >= 128.0f)
		{
			if (m_nav.NeedsRepath())
			{
				m_nav.StartRepathTimer();
				CTF2BotPathCost cost(bot);
				m_nav.ComputePathToPosition(bot, m_hideSpot, cost);
			}

			m_nav.Update(bot);
		}
		else
		{
			Vector lookat = m_trapLocation + Vector(0.0f, 0.0f, IMovement::s_playerhull.crouch_height);
			input->AimAt(lookat, IPlayerController::LOOK_PRIORITY, 1.0f, "Overwatching sticky trap!");
			ShouldDetonateStickiesFunctor func{ m_trapLocation };

			bot->GetSensorInterface()->ForEveryKnownEnemy(func);

			if (func.m_detonate || pipecount == 0)
			{
				input->PressSecondaryAttackButton();
				return Done("Stickies detonated!");
			}
		}
	}
	else
	{

		if (pipecount >= m_maxBombs)
		{
			m_trapDeployed = true;
			m_hideSpot = SelectHidingSpot(bot);
			m_nav.InvalidateAndRepath();
			m_trapEndTimer.StartRandom(30.0f, 90.0f);
			return Continue();
		}

		if (bot->GetRangeTo(m_trapLocation) >= 256.0f)
		{
			const CKnownEntity* known = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

			if (known)
			{
				return PauseFor(new CBotSharedAttackEnemyTask<CTF2Bot, CTF2BotPathCost>(bot), "Attacking visible enemy!");
			}

			if (m_nav.NeedsRepath())
			{
				m_nav.StartRepathTimer();
				CTF2BotPathCost cost(bot);
				m_nav.ComputePathToPosition(bot, m_trapLocation, cost);
			}

			m_nav.Update(bot);
		}
		else
		{
			// aim and shoot

			if (m_justFired)
			{
				m_justFired = false;
				input->ReleaseAllAttackButtons();
				RandomAimSpot();
				return Continue();
			}

			input->AimAt(m_aimPos, IPlayerController::LOOK_PRIORITY, 0.5f, "Deploying sticky trap!");

			if (input->IsAimOnTarget())
			{
				input->PressAttackButton();
				m_justFired = true;
			}
		}
	}

	return TaskResult<CTF2Bot>();
}

TaskEventResponseResult<CTF2Bot> CTF2BotDemomanLayStickyTrapTask::OnInjured(CTF2Bot* bot, const CTakeDamageInfo& info)
{
	if (m_trapDeployed)
	{
		CBaseEntity* attacker = info.GetAttacker();

		if (attacker && !bot->GetSensorInterface()->IsIgnored(attacker) && bot->GetSensorInterface()->IsEnemy(attacker))
		{
			bot->GetControlInterface()->PressSecondaryAttackButton();
			return TryDone(PRIORITY_MEDIUM, "I've been spotted!");
		}
	}

	return TryContinue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotDemomanLayStickyTrapTask::OnMoveToSuccess(CTF2Bot* bot, CPath* path)
{
	return TryContinue();
}

void CTF2BotDemomanLayStickyTrapTask::RandomAimSpot()
{
	constexpr float RAND_HORIZ = 48.0f;
	constexpr float RAND_VERT = 20.0f;

	m_aimPos.x = m_trapLocation.x + randomgen->GetRandomReal<float>(-RAND_HORIZ, RAND_HORIZ);
	m_aimPos.y = m_trapLocation.y + randomgen->GetRandomReal<float>(-RAND_HORIZ, RAND_HORIZ);
	m_aimPos.z = m_trapLocation.z + randomgen->GetRandomReal<float>(-RAND_VERT, RAND_VERT);

}

Vector CTF2BotDemomanLayStickyTrapTask::SelectHidingSpot(CTF2Bot* bot) const
{
	const float maxvisionrange = bot->GetSensorInterface()->GetMaxVisionRange();

	botsharedutils::IsReachableAreas collector{ bot, maxvisionrange + 300.0f };
	CNavArea* start = TheNavMesh->GetNearestNavArea(m_trapLocation, CPath::PATH_GOAL_MAX_DISTANCE_TO_AREA * 4.0f);

	if (!start)
	{
		return bot->GetAbsOrigin();
	}

	collector.SetStartArea(start);
	collector.Execute();

	if (collector.IsCollectedAreasEmpty())
	{
		return bot->GetAbsOrigin();
	}

	auto& vec = collector.GetCollectedAreas();

	std::vector<CNavArea*> areasvec;

	for (CNavArea* area : vec)
	{
		if (area == start)
		{
			continue;
		}

		// too close
		if ((area->GetCenter() - m_trapLocation).IsLengthLessThan(512.0f))
		{
			continue;
		}

		if (!start->IsPartiallyVisible(area))
		{
			continue;
		}

		areasvec.push_back(area);
	}

	if (!areasvec.empty())
	{
		CNavArea* area = librandom::utils::GetRandomElementFromVector(areasvec);

		Vector spot;
		
		// this should always be true
		if (area->IsVisible(m_trapLocation + Vector(0.0f, 0.0f, navgenparams->human_eye_height), &spot))
		{
			return spot;
		}

		return area->GetRandomPoint();
	}

	return bot->GetAbsOrigin();
}
