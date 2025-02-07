#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/librandom.h>
#include <mods/blackmesa/blackmesadm_mod.h>
#include <bot/blackmesa/bmbot.h>
#include "bmbot_find_ammo_task.h"

CBlackMesaBotFindAmmoTask::CBlackMesaBotFindAmmoTask(blackmesa::BMAmmoIndex ammoType)
{
	const char* itemname = blackmesa::GetItemNameForAmmoIndex(ammoType);

	if (itemname == nullptr)
	{
		m_classname.clear();
	}
	else
	{
		m_classname.assign(itemname);
	}

	m_ammoIndex = ammoType;
	m_maxCarry = CBlackMesaDeathmatchMod::GetBMMod()->GetMaxCarryForAmmoType(m_ammoIndex);
}

bool CBlackMesaBotFindAmmoTask::IsPossible(CBlackMesaBot* bot, blackmesa::BMAmmoIndex& ammoInNeed)
{
	ammoInNeed = bot->GetInventoryInterface()->SelectRandomNeededAmmo();

	if (ammoInNeed != blackmesa::BMAmmoIndex::Ammo_Invalid)
	{
		return true;
	}

	return false;
}

TaskResult<CBlackMesaBot> CBlackMesaBotFindAmmoTask::OnTaskStart(CBlackMesaBot* bot, AITask<CBlackMesaBot>* pastTask)
{
	CBaseEntity* ammo = nullptr;

	if (!FindAmmoToPickup(bot, &ammo, m_classname))
	{
		return Done("No ammo to pickup!");
	}

	SetGoalEntity(ammo);

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
		if (!FindAmmoToPickup(bot, &item, m_classname, 1024.0f, true))
		{
			return Done("No ammo to pickup!");
		}
		else
		{
			SetGoalEntity(item);
		}
	}

	if (m_repathtimer.IsElapsed())
	{
		CBlackMesaBotPathCost cost(bot);
		
		if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
		{
			return Done("Failed to find a path to the ammo entity!");
		}

		m_repathtimer.Start(1.0f);
	}

	m_nav.Update(bot);

	return Continue();
}

void CBlackMesaBotFindAmmoTask::SetGoalEntity(CBaseEntity* goalEnt)
{
	m_item = goalEnt;
	m_goal = UtilHelpers::getEntityOrigin(goalEnt);
	m_repathtimer.Invalidate();
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

bool CBlackMesaBotFindAmmoTask::FindAmmoToPickup(CBlackMesaBot* bot, CBaseEntity** ammo, const std::string& classname, const float maxRange, const bool selectNearest)
{
	Vector start = bot->GetAbsOrigin();
	CBlackMesaBotInventory* inventory = bot->GetInventoryInterface();
	std::vector<CBaseEntity*> nearbyammo;

	if (classname.empty())
	{
		return false;
	}

	UtilHelpers::ForEachEntityOfClassname(classname.c_str(), [&nearbyammo, &start, &maxRange](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			int effects = 0;
			entprops->GetEntProp(index, Prop_Send, "m_fEffects", effects);

			if ((effects & EF_NODRAW) != 0)
			{
				return true; // keep loop
			}

			const Vector& end = UtilHelpers::getEntityOrigin(entity);

			const float range = (start - end).Length();

			if (range <= maxRange)
			{
				nearbyammo.push_back(entity);
			}
		}

		return true;
	});



	if (nearbyammo.empty())
	{
		return false;
	}

	if (nearbyammo.size() == 1)
	{
		*ammo = nearbyammo[0];
		return true;
	}

	if (selectNearest)
	{
		CBaseEntity* best = nullptr;
		float nearest = maxRange * maxRange;

		for (auto entity : nearbyammo)
		{
			const Vector& end = UtilHelpers::getEntityOrigin(entity);

			float range = (start - end).LengthSqr();

			if (range < nearest)
			{
				nearest = range;
				best = entity;
			}
		}

		if (best == nullptr)
		{
			return false;
		}

		*ammo = best;
		return true;
	}

	*ammo = librandom::utils::GetRandomElementFromVector<CBaseEntity*>(nearbyammo);
	return true;
}
