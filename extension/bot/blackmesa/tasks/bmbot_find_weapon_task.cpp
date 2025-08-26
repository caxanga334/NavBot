#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/librandom.h>
#include <bot/blackmesa/bmbot.h>
#include "bmbot_find_weapon_task.h"

bool CBlackMesaBotFindWeaponTask::IsPossible(CBlackMesaBot* bot)
{
	// bot doesn't own every weapon type
	if (bot->GetInventoryInterface()->GetOwnedWeaponCount() < 14)
	{
		return true;
	}

	return false;
}

TaskResult<CBlackMesaBot> CBlackMesaBotFindWeaponTask::OnTaskStart(CBlackMesaBot* bot, AITask<CBlackMesaBot>* pastTask)
{
	CBaseEntity* weapon = nullptr;

	if (!CBlackMesaBotFindWeaponTask::FindWeaponToPickup(bot, &weapon))
	{
		return Done("No weapon to pick up!");
	}

	m_weapon = weapon;
	m_goal = UtilHelpers::getEntityOrigin(weapon);

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

bool CBlackMesaBotFindWeaponTask::FindWeaponToPickup(CBlackMesaBot* bot, CBaseEntity** weapon, const float maxRange)
{
	Vector start = bot->GetAbsOrigin();
	CBlackMesaBotInventory* inventory = bot->GetInventoryInterface();
	std::vector<CBaseEntity*> nearbyweapons;

	auto functor = [&nearbyweapons, &inventory](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			const char* classname = entityprops::GetEntityClassname(entity);

			if (!classname)
			{
				return true; // keep loop
			}

			std::string clname{ classname };

			if (clname.find("item_weapon") != std::string::npos)
			{
				int effects = 0;
				entprops->GetEntProp(index, Prop_Send, "m_fEffects", effects);

				if ((effects & EF_NODRAW) != 0)
				{
					return true; // keep loop
				}


				if (!inventory->OwnsThisWeapon(clname))
				{
					nearbyweapons.push_back(entity);
				}
			}
		}

		return true;
	};

	UtilHelpers::ForEachEntityInSphere(start, maxRange, functor);

	if (nearbyweapons.empty())
	{
		return false;
	}

	if (nearbyweapons.size() == 1)
	{
		*weapon = nearbyweapons[0];
		return true;
	}

	*weapon = librandom::utils::GetRandomElementFromVector<CBaseEntity*>(nearbyweapons);
	return true;
}
