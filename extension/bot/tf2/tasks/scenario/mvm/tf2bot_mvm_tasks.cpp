#include <algorithm>
#include <extension.h>
#include <util/entprops.h>
#include <mods/tf2/tf2lib.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/nav/tfnavmesh.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <bot/tf2/tf2bot.h>
#include "tf2bot_mvm_tasks.h"

CTF2BotCollectMvMCurrencyTask::CTF2BotCollectMvMCurrencyTask(std::vector<CHandle<CBaseEntity>>& packs)
{
	m_currencypacks = packs;
	m_allowattack = true;
}

TaskResult<CTF2Bot> CTF2BotCollectMvMCurrencyTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	// remove invalid currency entities
	m_currencypacks.erase(std::remove_if(m_currencypacks.begin(), m_currencypacks.end(), [](const CHandle<CBaseEntity>& handle) {
		return handle.Get() == nullptr;
	}), m_currencypacks.end());

	m_it = m_currencypacks.begin();

	if (bot->GetMyClassType() == TeamFortress2::TFClass_Spy)
	{
		if (!bot->IsDisguised())
		{
			bot->DisguiseAs(TeamFortress2::TFClass_Spy, false);
		}

		m_allowattack = false;
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotCollectMvMCurrencyTask::OnTaskUpdate(CTF2Bot* bot)
{
	auto threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

	// these classes gives up if they see an enemy
	if ((bot->GetMyClassType() == TeamFortress2::TFClass_Medic || bot->GetMyClassType() == TeamFortress2::TFClass_Engineer ||
		bot->GetMyClassType() == TeamFortress2::TFClass_Sniper) && threat)
	{
		return Done("Not safe to collect currency!");
	}

	if (bot->GetMyClassType() == TeamFortress2::TFClass_Spy)
	{
		if (bot->GetCloakPercentage() > 30.0f && !bot->IsInvisible())
		{
			// cloak
			bot->GetControlInterface()->PressSecondaryAttackButton();
		}
	}

	CBaseEntity* currency = m_it->Get();

	if (currency == nullptr)
	{
		m_it = std::next(m_it, 1);

		if (m_it == m_currencypacks.end())
		{
			return Done("Done collecting currency packs!");
		}
		else
		{
			m_repathTimer.Invalidate();
			return Continue(); // skip 1 update cycle
		}
	}

	if (m_repathTimer.IsElapsed())
	{
		m_repathTimer.Start(0.8f);
		Vector pos = UtilHelpers::getEntityOrigin(currency);
		CTF2BotPathCost cost(bot);
		
		if (!m_nav.ComputePathToPosition(bot, pos, cost))
		{
			m_it = std::next(m_it, 1);

			if (m_it == m_currencypacks.end())
			{
				return Done("Done collecting currency packs!");
			}

			m_repathTimer.Invalidate();
			return Continue(); // skip 1 update cycle
		}
	}

	m_nav.Update(bot);
	
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMvMCombatTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{

	switch (bot->GetMyClassType())
	{
	case TeamFortress2::TFClass_Heavy:
		[[fallthrough]];
	case TeamFortress2::TFClass_Pyro:
		m_idealCombatRange = 200.0f;
		break;
	default:
		m_idealCombatRange = 600.0f;
		break;
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMvMCombatTask::OnTaskUpdate(CTF2Bot* bot)
{
	auto threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(false);

	if (!threat)
	{
		return Done("No more enemies!");
	}

	if (!threat->IsVisibleNow())
	{
		bot->GetControlInterface()->AimAt(m_moveGoal, IPlayerController::LOOK_ALERT, 0.5f, "Looking at threat position!");
		m_moveGoal = UtilHelpers::getEntityOrigin(threat->GetEntity());
	}
	else
	{
		const Vector myPos = bot->GetAbsOrigin();
		const Vector& theirPos = UtilHelpers::getEntityOrigin(threat->GetEntity());
		Vector to = (theirPos - myPos);
		to.NormalizeInPlace();
		m_moveGoal = theirPos + (to * m_idealCombatRange);
	}

	if (m_repathTimer.IsElapsed())
	{
		m_repathTimer.Start(0.5f);
		CTF2BotPathCost cost(bot);
		
		if (!m_nav.ComputePathToPosition(bot, m_moveGoal, cost))
		{
			if (bot->IsDebugging(BOTDEBUG_TASKS))
			{
				bot->DebugPrintToConsole(255, 0, 0, "%s CTF2BotMvMCombatTask Failed to compute path to threat! <%3.2f, %3.2f, %3.2f> \n", bot->GetDebugIdentifier(),
					m_moveGoal.x, m_moveGoal.y, m_moveGoal.z);
			}

			m_repathTimer.Start(1.0f);
			m_nav.Invalidate();
		}
	}

	m_nav.Update(bot);

	return Continue();
}
