#include NAVBOT_PCH_FILE
#include <mods/zps/zps_lib.h>
#include <mods/zps/zps_mod.h>
#include <bot/zps/zpsbot.h>
#include <bot/tasks_shared/bot_shared_roam.h>
#include <bot/tasks_shared/bot_shared_drop_weapon.h>
#include <bot/tasks_shared/bot_shared_collect_items.h>
#include <bot/tasks_shared/bot_shared_escort_entity.h>
#include "zpsbot_human_objectives_task.h"

AITask<CZPSBot>* CZPSBotObjectiveFindItemTask::InitialNextTask(CZPSBot* bot)
{
	// roam is used to simulate searching
	return new CBotSharedRoamTask<CZPSBot, CZPSBotPathCost>(bot, 8192.0f);
}

TaskResult<CZPSBot> CZPSBotObjectiveFindItemTask::OnTaskStart(CZPSBot* bot, AITask<CZPSBot>* pastTask)
{
	const CZPSObjectiveManager& mgr = CZombiePanicSourceMod::GetZPSMod()->GetObjectiveManager();
	const std::string& name = mgr.GetItemSearchID();

	if (bot->GetInventoryInterface()->FindItemDeliver(name) != nullptr)
	{
		return PauseFor(new CBotSharedRoamTask<CZPSBot, CZPSBotPathCost>(bot, 8192.0f), "Already have required item, roaming!");
	}

	return Continue();
}

TaskResult<CZPSBot> CZPSBotObjectiveFindItemTask::OnTaskUpdate(CZPSBot* bot)
{
	if (GetNextTask() == nullptr)
	{
		StartNewNextTask(bot, new CBotSharedRoamTask<CZPSBot, CZPSBotPathCost>(bot, 8192.0f), "Restarting roam task!");
	}

	if (m_nextScanTimer.IsElapsed())
	{
		m_nextScanTimer.StartRandom(1.0f, 2.0f);
		const CZPSObjectiveManager& mgr = CZombiePanicSourceMod::GetZPSMod()->GetObjectiveManager();
		const std::string& name = mgr.GetItemSearchID();

		if (bot->GetInventoryInterface()->FindItemDeliver(name))
		{
			return Done("I've got the item!");
		}

		const float detectionRadius = mgr.GetDetectionRadius();
		Vector eyePos = bot->GetEyeOrigin();
		CBaseEntity* found = nullptr;

		auto func = [&name, &detectionRadius, &found, &eyePos](int index, edict_t* edict, CBaseEntity* entity) {
			if (entity)
			{
				CBaseEntity* owner = nullptr;
				entprops->GetEntPropEnt(entity, Prop_Send, "m_hOwner", nullptr, &owner);

				if (owner)
				{
					return true;
				}

				bool cantequip = false;
				entprops->GetEntPropBool(entity, Prop_Send, "m_bCantEquip", cantequip);

				if (cantequip)
				{
					return true;
				}

				const Vector& pos = UtilHelpers::getEntityOrigin(entity);

				if ((pos - eyePos).IsLengthGreaterThan(detectionRadius))
				{
					return true;
				}

				std::string itemid = entprops->GetEntPropString(entity, Prop_Data, "m_strItemID");
				
				if (ke::StrCaseCmp(itemid.c_str(), name.c_str()) == 0)
				{
					found = entity;
					return false;
				}
			}

			return true;
		};

		UtilHelpers::ForEachEntityOfClassname("item_deliver", func);

		if (found)
		{
			m_nextScanTimer.Invalidate();
			
			if (!HasSpaceInInventory(bot))
			{
				const CBotWeapon* todrop = bot->GetInventoryInterface()->FindWeaponToDrop();

				if (todrop)
				{
					return PauseFor(new CBotSharedDropWeaponTask<CZPSBot>(todrop, 2.0f), "Dropping weapon to make space in my inventory!");
				}
			}

			auto task = new CBotSharedCollectItemsTask<CZPSBot, CZPSBotPathCost>(bot, found, NBotSharedCollectItemTask::COLLECT_PRESS_USE);
			auto func = std::bind(NBotSharedCollectItemTask::CommonWeaponValidator<CZPSBot>, std::placeholders::_1, std::placeholders::_2);
			task->SetValidationFunction(func);
			return SwitchTo(task, "Collecting item!");
		}
	}

	return Continue();
}

QueryAnswerType CZPSBotObjectiveFindItemTask::ShouldPickup(CBaseBot* me, CBaseEntity* item)
{
	if (item)
	{
		// don't pick weapons
		if (UtilHelpers::FClassnameIs(item, "weapon_*"))
		{
			return ANSWER_NO;
		}
	}

	return ANSWER_UNDEFINED;
}

bool CZPSBotObjectiveFindItemTask::HasSpaceInInventory(CZPSBot* bot) const
{
	const WeaponInfo* info = extmanager->GetMod()->GetWeaponInfoManager()->GetClassnameInfo("item_deliver");

	if (!info) { return true; }

	int size = static_cast<const ZPSWeaponInfo*>(info)->GetInventorySize();

	if (bot->GetInventoryInterface()->GetInventorySize() + size >= CZPSBotInventory::MAX_INVENTORY_SIZE)
	{
		return false;
	}

	return true;
}

bool CZPSBotObjectiveUseItemTask::IsPossible(CZPSBot* bot)
{
	const CZPSObjectiveManager& mgr = CZombiePanicSourceMod::GetZPSMod()->GetObjectiveManager();
	const std::string& name = mgr.GetItemSearchID();
	CBaseEntity* specificitem = mgr.GetGenericTargetEntity();

	// target not set
	if (mgr.GetUseItemTarget() == nullptr)
	{
		return false;
	}

	if (specificitem)
	{
		// bot got the specific item needed
		if (bot->GetInventoryInterface()->HasWeapon(specificitem))
		{
			return true;
		}
	}

	// we have the item in our inventory
	if (bot->GetInventoryInterface()->FindItemDeliver(name) != nullptr)
	{
		return true;
	}

	return false;
}

TaskResult<CZPSBot> CZPSBotObjectiveUseItemTask::OnTaskStart(CZPSBot* bot, AITask<CZPSBot>* pastTask)
{
	m_usingItem = false;
	return Continue();
}

TaskResult<CZPSBot> CZPSBotObjectiveUseItemTask::OnTaskUpdate(CZPSBot* bot)
{
	// waiting for the weapon switch cooldown
	if (!m_switchDelay.IsElapsed())
	{
		return Continue();
	}

	const CZPSObjectiveManager& mgr = CZombiePanicSourceMod::GetZPSMod()->GetObjectiveManager();
	CBaseEntity* entity = mgr.GetUseItemTarget();

	if (!entity)
	{
		return Done("Use target is NULL!");
	}

	Vector eyePos = bot->GetEyeOrigin();
	Vector center = UtilHelpers::getWorldSpaceCenter(entity);

	// item_deliver use radius appears to be smaller than the default use radius
	if ((eyePos - center).IsLengthLessThan((CBaseExtPlayer::PLAYER_USE_RADIUS * 0.7f)))
	{
		m_usingItem = true;

		if (!IsItemEquipped(bot))
		{
			EquipRequiredItem(bot);
			return Continue();
		}

		bot->GetControlInterface()->AimAt(center, IPlayerController::LOOK_PRIORITY, 0.5f, "Looking at use entity!");

		if (bot->GetControlInterface()->IsAimOnTarget())
		{
			bot->GetControlInterface()->PressAttackButton(0.2f);
			return Done("Item used!");
		}
	}

	if (m_nav.NeedsRepath())
	{
		CZPSBotPathCost cost(bot, RouteType::FASTEST_ROUTE);
		Vector pos = trace::getground(center);
		m_nav.ComputePathToPosition(bot, pos, cost);
		m_nav.StartRepathTimer();
	}

	m_nav.Update(bot);
	return Continue();
}

QueryAnswerType CZPSBotObjectiveUseItemTask::ShouldPickup(CBaseBot* me, CBaseEntity* item)
{
	const CZPSObjectiveManager& mgr = CZombiePanicSourceMod::GetZPSMod()->GetObjectiveManager();
	CBaseEntity* entity = mgr.GetUseItemTarget();

	if (!entity)
	{
		return ANSWER_UNDEFINED;
	}

	Vector eyePos = me->GetEyeOrigin();
	Vector p1 = UtilHelpers::getWorldSpaceCenter(entity);
	Vector p2 = UtilHelpers::getWorldSpaceCenter(item);
	float d1 = (eyePos - p1).LengthSqr();
	float d2 = (eyePos - p2).LengthSqr();

	// do not pick up items if the use target is closer than the item
	if (d1 <= d2)
	{
		return ANSWER_NO;
	}

	return m_usingItem ? ANSWER_UNDEFINED : ANSWER_NO;
}

bool CZPSBotObjectiveUseItemTask::IsItemEquipped(CZPSBot* bot) const
{
	const CZPSObjectiveManager& mgr = CZombiePanicSourceMod::GetZPSMod()->GetObjectiveManager();
	CBaseEntity* specificitem = mgr.GetGenericTargetEntity();
	const CBotWeapon* activeWeapon = bot->GetInventoryInterface()->GetActiveBotWeapon();

	if (!activeWeapon)
	{
		return false;
	}

	if (activeWeapon->GetEntity() == specificitem)
	{
		return true;
	}

	const CBotWeapon* requiredItem = bot->GetInventoryInterface()->FindItemDeliver(mgr.GetItemSearchID());

	if (activeWeapon == requiredItem)
	{
		return true;
	}

	return false;
}

void CZPSBotObjectiveUseItemTask::EquipRequiredItem(CZPSBot* bot)
{
	const CZPSObjectiveManager& mgr = CZombiePanicSourceMod::GetZPSMod()->GetObjectiveManager();
	CBaseEntity* specificitem = mgr.GetGenericTargetEntity();

	const CBotWeapon* weapon = bot->GetInventoryInterface()->GetWeaponOfEntity(specificitem);

	if (weapon)
	{
		m_switchDelay.Start(3.0f);
		bot->GetInventoryInterface()->EquipWeapon(weapon);
		return;
	}

	weapon = bot->GetInventoryInterface()->FindItemDeliver(mgr.GetItemSearchID());

	if (weapon)
	{
		m_switchDelay.Start(3.0f);
		bot->GetInventoryInterface()->EquipWeapon(weapon);
	}
}

bool CZPSBotObjectiveFollowItemCarrierTask::IsPossible(CZPSBot* bot, CBaseEntity** carrier)
{
	CBaseEntity* pPlayer = nullptr;
	const CZPSObjectiveManager& mgr = CZombiePanicSourceMod::GetZPSMod()->GetObjectiveManager();

	auto func = [&pPlayer, &mgr](int client, edict_t* entity, SourceMod::IGamePlayer* player) {
		CBaseEntity* pEntity = UtilHelpers::EdictToBaseEntity(entity);

		if (pEntity)
		{
			if (modhelpers->GetEntityTeamNumber(pEntity) != static_cast<int>(zps::ZPSTeam::ZPS_TEAM_SURVIVORS))
			{
				return;
			}

			if (modhelpers->IsDead(pEntity))
			{
				return;
			}

			if (zpslib::PlayerHasNamedItem(pEntity, mgr.GetItemSearchID().c_str()))
			{
				pPlayer = pEntity;
				return;
			}
		}
	};

	UtilHelpers::ForEachPlayer(func);

	if (pPlayer)
	{
		if (pPlayer == bot->GetEntity())
		{
			return false;
		}

		*carrier = pPlayer;
		return true;
	}

	return false;
}

AITask<CZPSBot>* CZPSBotObjectiveFollowItemCarrierTask::InitialNextTask(CZPSBot* bot)
{
	CBaseEntity* carrier = m_carrier.Get();

	if (carrier)
	{
		return new CBotSharedEscortEntityTask<CZPSBot, CZPSBotPathCost>(bot, carrier, 1e10f, 450.0f);
	}

	return nullptr;
}

TaskResult<CZPSBot> CZPSBotObjectiveFollowItemCarrierTask::OnTaskUpdate(CZPSBot* bot)
{
	CBaseEntity* carrier = m_carrier.Get();

	if (!carrier)
	{
		return Done("Item carrier disconnected!");
	}

	if (GetNextTask() == nullptr)
	{
		return Done("No longer following the item carrier!");
	}

	if (m_checkInventory.IsElapsed())
	{
		m_checkInventory.Start(5.0f);
	}

	return Continue();
}
