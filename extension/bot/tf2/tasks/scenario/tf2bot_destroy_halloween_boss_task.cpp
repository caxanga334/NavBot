#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/helpers.h>
#include <sdkports/sdk_traces.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <bot/interfaces/path/meshnavigator.h>
#include "tf2bot_destroy_halloween_boss_task.h"

TaskResult<CTF2Bot> CTF2BotDestroyHalloweenBossTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	if (!FindHalloweenBoss())
	{
		return Done("Failed to find the halloween boss entity!");
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotDestroyHalloweenBossTask::OnTaskUpdate(CTF2Bot* bot)
{
	CBaseEntity* boss = m_boss.Get();

	if (!boss)
	{
		return Done("Boss entity is NULL!");
	}

	CKnownEntity* known = bot->GetSensorInterface()->AddKnownEntity(boss);
	known->UpdatePosition();
	UpdateBossPosition(boss);

	bool moveTowards = true;

	if (bot->GetSensorInterface()->IsAbleToSee(boss))
	{
		auto weapon = bot->GetInventoryInterface()->GetActiveTFWeapon();

		if (weapon)
		{
			if (bot->GetRangeTo(UtilHelpers::getWorldSpaceCenter(boss)) < weapon->GetCurrentMaximumAttackRange(bot))
			{
				// boss is visible and in-range
				moveTowards = false;
			}
		}
	}
	else
	{
		if (bot->GetSensorInterface()->IsLineOfSightClear(boss) && !bot->GetSensorInterface()->IsInFieldOfView(UtilHelpers::getEntityOrigin(boss)))
		{
			bot->GetControlInterface()->AimAt(UtilHelpers::getEntityOrigin(boss), IPlayerController::LOOK_ALERT, 2.0f, "Looking at the boss!");
		}
	}

	if (moveTowards)
	{
		if (m_nav.NeedsRepath())
		{
			m_nav.StartRepathTimer();
			CTF2BotPathCost cost(bot);
			m_nav.ComputePathToPosition(bot, m_goal, cost, 0.0f, true);
		}

		m_nav.Update(bot);
	}

	return Continue();
}

bool CTF2BotDestroyHalloweenBossTask::FindHalloweenBoss()
{
	CBaseEntity* boss = nullptr;
	auto eyeballfunctor = [&boss](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			// ignore spell created Monoculus
			if (tf2lib::GetEntityTFTeam(entity) == TeamFortress2::TFTeam::TFTeam_Halloween)
			{
				boss = entity;
				return false;
			}
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("eyeball_boss", eyeballfunctor);

	auto functor = [&boss](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			boss = entity;
			return false;
		}

		return true;
	};

	if (!boss)
	{
		UtilHelpers::ForEachEntityOfClassname("merasmus", functor);
	}

	if (!boss)
	{
		// base_boss is generally used with Vscript bosses
		UtilHelpers::ForEachEntityOfClassname("base_boss", functor);
	}

	if (!boss)
	{
		UtilHelpers::ForEachEntityOfClassname("headless_hatman", functor);
	}

	if (boss)
	{
		m_boss = boss;
		return true;
	}

	return false;
}

void CTF2BotDestroyHalloweenBossTask::UpdateBossPosition(CBaseEntity* boss)
{
	m_goal = trace::getground(UtilHelpers::getWorldSpaceCenter(boss));
}
