#include NAVBOT_PCH_FILE
#include <mods/blackmesa/blackmesadm_mod.h>
#include <mods/blackmesa/nav/bm_nav_mesh.h>
#include <bot/blackmesa/bmbot.h>
#include <bot/bot_shared_utils.h>
#include "bmbot_find_weapon_task.h"

bool CBlackMesaBotFindWeaponTask::IsPossible(CBlackMesaBot* bot, CBaseEntity** weapon)
{
	botsharedutils::search::SearchReachableEntities<CBlackMesaBot, CNavArea> collector(bot);
	collector.SetCheckCanPickup(true); // query the behavior to see if we should pick up items
	collector.AddSearchPattern("item_weapon_*");
	collector.DoSearch();
	CBaseEntity* item = nullptr;

	// randomize a bit
	if (CBaseBot::s_botrng.GetRandomChance(33))
	{
		item = collector.SelectFarthest();
	}
	else
	{
		item = collector.SelectNearest();
	}

	if (!item)
	{
		return false;
	}

	*weapon = item;
	return true;
}

TaskResult<CBlackMesaBot> CBlackMesaBotFindWeaponTask::OnTaskStart(CBlackMesaBot* bot, AITask<CBlackMesaBot>* pastTask)
{
	if (!IsWeaponValid())
	{
		return Done("Weapon is no longer valid!");
	}

	m_goal = UtilHelpers::getEntityOrigin(m_weapon.Get());
	return Continue();
}

TaskResult<CBlackMesaBot> CBlackMesaBotFindWeaponTask::OnTaskUpdate(CBlackMesaBot* bot)
{
	if (!IsWeaponValid())
	{
		return Done("Weapon is invalid/taken!");
	}

	if (m_nav.NeedsRepath())
	{
		m_nav.StartRepathTimer();
		CBlackMesaBotPathCost cost(bot);
		
		if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
		{
			return Done("No path to weapon!");
		}
	}

	m_nav.Update(bot);

	return Continue();
}

void CBlackMesaBotFindWeaponTask::OnTaskEnd(CBlackMesaBot* bot, AITask<CBlackMesaBot>* nextTask)
{
	bot->GetInventoryInterface()->SelectBestWeapon();
}

TaskEventResponseResult<CBlackMesaBot> CBlackMesaBotFindWeaponTask::OnMoveToSuccess(CBlackMesaBot* bot, CPath* path)
{
	return TryDone(PRIORITY_HIGH, "Goal reached.");
}

TaskEventResponseResult<CBlackMesaBot> CBlackMesaBotFindWeaponTask::OnWeaponEquip(CBlackMesaBot* bot, CBaseEntity* weapon)
{
	if (weapon == m_weapon.Get())
	{
		return TryDone(PRIORITY_HIGH, "Goal reached.");
	}

	return TryContinue();
}

bool CBlackMesaBotFindWeaponTask::IsWeaponValid()
{
	CBaseEntity* source = m_weapon.Get();

	if (source == nullptr)
	{
		return false;
	}

	int effects = 0;
	entprops->GetEntProp(m_weapon.GetEntryIndex(), Prop_Send, "m_fEffects", effects);

	if ((effects & EF_NODRAW) != 0)
	{
		return false;
	}

	return true;
}
