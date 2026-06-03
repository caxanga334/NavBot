#include NAVBOT_PCH_FILE
#include <mods/blackmesa/blackmesadm_mod.h>
#include <mods/blackmesa/nav/bm_nav_mesh.h>
#include <bot/blackmesa/bmbot.h>
#include <bot/bot_shared_utils.h>
#include "bmbot_find_ammo_task.h"

CBlackMesaBotFindAmmoTask::CBlackMesaBotFindAmmoTask(CBaseEntity* ammo, blackmesa::BMAmmoIndex ammoIndex)
{
	m_item = ammo;
	m_ammoIndex = ammoIndex;
	m_maxCarry = CBlackMesaDeathmatchMod::GetBMMod()->GetMaxCarryForAmmoType(ammoIndex);
}

bool CBlackMesaBotFindAmmoTask::IsPossible(CBlackMesaBot* bot, CBaseEntity** ammo, blackmesa::BMAmmoIndex& ammoIndex)
{
	ammoIndex = bot->GetInventoryInterface()->SelectRandomNeededAmmo();

	if (ammoIndex == blackmesa::BMAmmoIndex::Ammo_Invalid)
	{
		return false;
	}

	const char* itemname = blackmesa::GetItemNameForAmmoIndex(ammoIndex);

	if (itemname == nullptr)
	{
		return false;
	}

	botsharedutils::search::SearchReachableEntities<CBlackMesaBot, CNavArea> collector(bot, CBlackMesaDeathmatchMod::GetBMMod()->GetModSettings()->GetCollectItemMaxDistance());
	collector.SetCheckCanPickup(true);
	collector.AddSearchPattern(itemname);
	collector.DoSearch();
	CBaseEntity* entity = collector.SelectNearest();

	if (!entity)
	{
		return false;
	}

	*ammo = entity;
	return true;;
}

TaskResult<CBlackMesaBot> CBlackMesaBotFindAmmoTask::OnTaskStart(CBlackMesaBot* bot, AITask<CBlackMesaBot>* pastTask)
{
	CBaseEntity* ammo = m_item.Get();

	if (!ammo)
	{
		return Done("Ammo entity is NULL!");
	}

	m_goal = UtilHelpers::getEntityOrigin(ammo);
	return Continue();
}

TaskResult<CBlackMesaBot> CBlackMesaBotFindAmmoTask::OnTaskUpdate(CBlackMesaBot* bot)
{
	CBaseEntity* item = m_item.Get();
	
	if (bot->GetAmmoOfIndex(static_cast<int>(m_ammoIndex)) >= m_maxCarry)
	{
		return Done("Ammo full!");
	}

	if (!IsItemValid(item))
	{
		return Done("No ammo to pickup!");
	}

	if (m_nav.NeedsRepath())
	{
		CBlackMesaBotPathCost cost(bot);
		
		if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
		{
			return Done("Failed to find a path to the ammo entity!");
		}

		m_nav.StartRepathTimer();
	}

	m_nav.Update(bot);

	return Continue();
}

bool CBlackMesaBotFindAmmoTask::IsItemValid(CBaseEntity* item)
{
	if (!item)
	{
		return false;
	}

	int effects = 0;
	entprops->GetEntProp(gamehelpers->EntityToBCompatRef(item), Prop_Send, "m_fEffects", effects);

	if ((effects & EF_NODRAW) != 0)
	{
		return false;
	}

	return true;
}