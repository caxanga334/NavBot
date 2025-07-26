#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <entities/tf2/tf_entities.h>
#include <bot/interfaces/path/chasenavigator.h>
#include <bot/tasks_shared/bot_shared_attack_enemy.h>
#include <bot/tasks_shared/bot_shared_defend_spot.h>
#include "tf2bot_rd_defend_core_task.h"

bool CTF2BotRDDefendCoreTask::IsPossible(CTF2Bot* bot, CBaseEntity** core)
{
	CBaseEntity* out = nullptr;
	auto functor = [&bot, &out](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			tfentities::HCaptureFlag cf(entity);

			if (!cf.IsDisabled() && cf.GetTFTeam() == bot->GetMyTFTeam() && (cf.IsStolen() || cf.IsDropped()))
			{
				out = entity;
				return false;
			}
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("item_teamflag", functor);

	if (!out)
	{
		return false;
	}

	*core = out;
	return true;

	return false;
}

CTF2BotRDDefendCoreTask::CTF2BotRDDefendCoreTask(CBaseEntity* core) :
	m_flag(core)
{
}

TaskResult<CTF2Bot> CTF2BotRDDefendCoreTask::OnTaskUpdate(CTF2Bot* bot)
{
	CBaseEntity* ent = m_flag.Get();

	if (!ent)
	{
		return Done("Reactor core is NULL!");
	}

	const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

	if (threat)
	{
		return PauseFor(new CBotSharedAttackEnemyTask<CTF2Bot, CTF2BotPathCost>(bot, 7.5f), "Attacking visible threat!");
	}

	tfentities::HCaptureFlag core(ent);

	if (core.IsDisabled() || core.IsHome())
	{
		return Done("Reactor core returned home.");
	}

	Vector goal = core.GetPosition();

	if (core.IsDropped() && bot->GetRangeTo(goal) <= 256.0f)
	{
		return SwitchTo(new CBotSharedDefendSpotTask<CTF2Bot, CTF2BotPathCost>(bot, goal, 20.0f, true), "Defending dropped reactor core!");
	}

	CBaseEntity* carrier = core.GetOwnerEntity();

	if (carrier)
	{
		bot->GetControlInterface()->AimAt(carrier, IPlayerController::LOOK_INTERESTING, 1.0f, "Looking at the reactor core carrier.");
	}

	if (m_nav.NeedsRepath())
	{
		m_nav.StartRepathTimer();
		CTF2BotPathCost cost(bot, FASTEST_ROUTE);
		m_nav.ComputePathToPosition(bot, goal, cost);
	}

	m_nav.Update(bot);

	return Continue();
}
