#include NAVBOT_PCH_FILE
#include <algorithm>
#include <extension.h>
#include <util/entprops.h>
#include <entities/tf2/tf_entities.h>
#include <mods/tf2/tf2lib.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/nav/tfnavmesh.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <bot/tf2/tf2bot.h>
#include "tf2bot_mvm_tasks.h"

#if SOURCE_ENGINE == SE_TF2
static ConVar cvar_collect_currency("sm_navbot_tf_mvm_collect_currency", "0", FCVAR_GAMEDLL, "Set to 1 to allow bots to collect MvM currency.");
#endif

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

bool CTF2BotCollectMvMCurrencyTask::IsAllowedToCollectCurrency()
{
#if SOURCE_ENGINE == SE_TF2
	return cvar_collect_currency.GetBool();
#else
	return false;
#endif
}

void CTF2BotCollectMvMCurrencyTask::ScanForDroppedCurrency(std::vector<CHandle<CBaseEntity>>& currencyPacks)
{
	for (int i = gpGlobals->maxClients + 1; i < gpGlobals->maxEntities; i++)
	{
		edict_t* edict = gamehelpers->EdictOfIndex(i);

		if (!UtilHelpers::IsValidEdict(edict))
			continue;

		CBaseEntity* entity = edict->GetIServerEntity()->GetBaseEntity();

		if (UtilHelpers::FClassnameIs(entity, "item_currencypack*"))
		{
			bool distributed = false;
			entprops->GetEntPropBool(i, Prop_Send, "m_bDistributed", distributed);

			if (distributed) // this one doesn't need to be collected (IE: killed by sniper)
				continue;

			currencyPacks.emplace_back(entity);
		}
	}
}

CTF2BotMvMTankBusterTask::CTF2BotMvMTankBusterTask(CBaseEntity* tank) : 
	m_tank(tank)
{
}

TaskResult<CTF2Bot> CTF2BotMvMTankBusterTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	m_rescanTimer.Start(2.0f);

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMvMTankBusterTask::OnTaskUpdate(CTF2Bot* bot)
{
	CBaseEntity* tank = m_tank.Get();

	if (tank == nullptr || !UtilHelpers::IsEntityAlive(tank))
	{
		return Done("Tank destroyed!");
	}

	const Vector& pos1 = UtilHelpers::getEntityOrigin(tank);

	if (m_rescanTimer.IsElapsed())
	{
		m_rescanTimer.Start(2.0f);

		CBaseEntity* othertank = tf2lib::mvm::GetMostDangerousTank();
		CBaseEntity* flag = tf2lib::mvm::GetMostDangerousFlag(true);
		const Vector& hatch = CTeamFortress2Mod::GetTF2Mod()->GetMvMBombHatchPosition();

		if (flag)
		{
			tfentities::HCaptureFlag cf(flag);
			Vector pos2 = cf.GetPosition();

			// bomb is closer to the hatch and the tank
			if ((hatch - pos2).LengthSqr() < (hatch - pos1).LengthSqr())
			{
				CBaseEntity* owner = cf.GetOwnerEntity();

				if (owner)
				{
					auto threat = bot->GetSensorInterface()->AddKnownEntity(owner);

					if (threat)
					{
						threat->UpdatePosition();
					}
				}

				return Done("Priority threat: bomb is closer to the hatch than the tank!");
			}

			if (othertank && othertank != tank)
			{
				const Vector& otherPos = UtilHelpers::getEntityOrigin(othertank);

				// another tank is closer to the hatch!
				if ((hatch - otherPos).LengthSqr() < (hatch - pos1).LengthSqr())
				{
					m_tank = othertank;
					tank = othertank;
				}
			}
		}
	}

	if (m_repathTimer.IsElapsed())
	{
		CTF2BotPathCost cost(bot);
		m_nav.ComputePathToPosition(bot, pos1, cost);
		m_repathTimer.Start(1.0f);
	}

	if (bot->GetRangeTo(pos1) > 500.0f)
	{
		m_nav.Update(bot);
	}

	bot->GetControlInterface()->AimAt(tank, IPlayerController::LOOK_INTERESTING, 1.0f, "Looking at the tank position!");

	return Continue();
}