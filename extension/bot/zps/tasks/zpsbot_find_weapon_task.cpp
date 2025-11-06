#include NAVBOT_PCH_FILE
#include <bot/zps/zpsbot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <mods/zps/zps_mod.h>
#include <mods/zps/nav/zps_nav_mesh.h>
#include <bot/bot_shared_utils.h>
#include <bot/tasks_shared/bot_shared_drop_weapon.h>
#include "zpsbot_find_weapon_task.h"

class ZPSWeaponSearch : public botsharedutils::search::SearchReachableEntities<CZPSBot, CZPSNavArea>
{
public:
	ZPSWeaponSearch(CZPSBot* bot) :
		botsharedutils::search::SearchReachableEntities<CZPSBot, CZPSNavArea>(bot)
	{
		this->AddSearchPattern("weapon_*");
	}

private:
	bool IsEntityValid(CBaseEntity* entity, CZPSNavArea* area) override
	{
		// must be in-mesh
		if (area)
		{
			CBaseEntity* owner = nullptr;
			entprops->GetEntPropEnt(entity, Prop_Data, "m_hOwner", nullptr, &owner);

			// already being carried by someone else
			if (owner != nullptr)
			{
				return false;
			}

			return true;
		}

		return false;
	}

	bool IsPossible(CBaseEntity* entity, CZPSBot* bot, CZPSNavArea* area) override
	{
		if (!bot->GetMovementInterface()->IsAreaTraversable(area))
		{
			return false;
		}

		const char* classname = gamehelpers->GetEntityClassname(entity);

		if (classname)
		{
			std::string name{ classname };
			const WeaponInfo* info = extmanager->GetMod()->GetWeaponInfoManager()->GetClassnameInfo(name);

			if (!info)
			{
				return false;
			}

			if (!info->IsCombatWeapon())
			{
				return false;
			}

			if (!info->GetAttackInfo(WeaponInfo::PRIMARY_ATTACK).IsMelee())
			{
				int clip = 0;
				entprops->GetEntProp(entity, Prop_Data, "m_iClip1", clip);

				// must have ammo
				if (clip <= 0)
				{
					return false;
				}
			}

			if (bot->GetBehaviorInterface()->ShouldPickup(bot, entity) == ANSWER_NO)
			{
				return false;
			}
		}

		return true;
	}

};

bool CZPSBotFindWeaponTask::IsPossible(CZPSBot* bot, CBaseEntity** outweapon)
{
	ZPSWeaponSearch search{ bot };
	search.DoSearch();

	auto& result = search.GetSearchResult();

	if (result.empty())
	{
		return false;
	}

	if (result.size() == 1U)
	{
		*outweapon = result[0].first;
		return true;
	}

	*outweapon = result[CBaseBot::s_botrng.GetRandomInt<std::size_t>(0U, result.size() - 1U)].first;
	return true;
}

CZPSBotFindWeaponTask::CZPSBotFindWeaponTask(CBaseEntity* weapon) :
	m_weapon(weapon)
{
	m_waitTimer.Invalidate();
	m_needsDrop = false;
	m_dropped = false;
}

TaskResult<CZPSBot> CZPSBotFindWeaponTask::OnTaskStart(CZPSBot* bot, AITask<CZPSBot>* pastTask)
{
	CBaseEntity* weapon = m_weapon.Get();

	if (!CZPSBotFindWeaponTask::IsValidWeapon(weapon))
	{
		return Done("Weapon is invalid!");
	}

	const char* classname = gamehelpers->GetEntityClassname(weapon);
	std::string name{ classname };
	const WeaponInfo* info = extmanager->GetMod()->GetWeaponInfoManager()->GetClassnameInfo(name);

	if (!info || !info->HasSlot())
	{
		return Done("Weapon is invalid!");
	}

	const CBotWeapon* bw = bot->GetInventoryInterface()->FindWeaponBySlot(info->GetSlot());

	if (bw)
	{
		m_needsDrop = true;
	}

	if (bot->IsDebugging(BOTDEBUG_TASKS))
	{
		bot->DebugPrintToConsole(255, 255, 0, "%s FIND WEAPON %s <%s>\n", bot->GetDebugIdentifier(), UtilHelpers::textformat::FormatEntity(weapon), info->GetConfigEntryName());
	}

	return Continue();
}

TaskResult<CZPSBot> CZPSBotFindWeaponTask::OnTaskUpdate(CZPSBot* bot)
{
	if (!m_waitTimer.IsElapsed())
	{
		return Continue();
	}

	if (m_timeout.HasStarted() && m_timeout.IsElapsed())
	{
		return Done("Timed out!");
	}

	CBaseEntity* weapon = m_weapon.Get();

	if (!CZPSBotFindWeaponTask::IsValidWeapon(weapon))
	{
		return Done("Weapon is invalid!");
	}

	const char* classname = gamehelpers->GetEntityClassname(weapon);
	Vector pos = UtilHelpers::getWorldSpaceCenter(weapon);
	const CBotWeapon* activeWeapon = bot->GetInventoryInterface()->GetActiveBotWeapon();

	if (activeWeapon && activeWeapon->GetEntity() == weapon)
	{
		bot->GetInventoryInterface()->StartInventoryUpdateTimer(0.5f);
		return Done("Got weapon!");
	}

	if (m_nav.NeedsRepath())
	{
		CZPSBotPathCost cost{ bot };
		m_nav.ComputePathToPosition(bot, pos, cost, 0.0f, true);
		m_nav.StartRepathTimer();
	}

	Vector eyepos = bot->GetEyeOrigin();
	const float range = (eyepos - pos).Length();

	if (m_needsDrop && !m_dropped && range <= (CBaseExtPlayer::PLAYER_USE_RADIUS * 2.5f))
	{
		std::string name{ classname };
		const WeaponInfo* info = extmanager->GetMod()->GetWeaponInfoManager()->GetClassnameInfo(name);

		if (!info || !info->HasSlot())
		{
			return Done("Weapon is invalid!");
		}

		const CBotWeapon* weaponinslot = bot->GetInventoryInterface()->FindWeaponBySlot(info->GetSlot());

		if (weaponinslot)
		{
			m_dropped = true;
			m_waitTimer.Start(2.0f);
			return PauseFor(new CBotSharedDropWeaponTask<CZPSBot>(weaponinslot), "Dropping weapon!");
		}
	}

	if (range < CBaseExtPlayer::PLAYER_USE_RADIUS)
	{
		bot->GetCombatInterface()->DisableCombat(1.0f);

		if (!m_timeout.HasStarted())
		{
			m_timeout.Start(5.0f);
		}

		bot->GetControlInterface()->AimAt(pos, IPlayerController::LOOK_PRIORITY, 1.0f, "Looking at weapon to pick up!");

		if (bot->GetControlInterface()->IsAimOnTarget())
		{
			bot->GetControlInterface()->PressUseButton();
			bot->GetInventoryInterface()->RequestRefresh();
			return Done("Weapon picked up!");
		}
	}

	m_nav.Update(bot);
	return Continue();
}

bool CZPSBotFindWeaponTask::IsValidWeapon(CBaseEntity* weapon)
{
	if (!weapon)
	{
		return false;
	}

	CBaseEntity* owner = nullptr;
	entprops->GetEntPropEnt(weapon, Prop_Data, "m_hOwner", nullptr, &owner);

	// already being carried by someone else
	if (owner != nullptr)
	{
		return false;
	}

	return true;
}
