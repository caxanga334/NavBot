#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/helpers.h>
#include <entities/tf2/tf_entities.h>
#include <util/entprops.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tf2bot.h>
#include <bot/tf2/tasks/tf2bot_find_ammo_task.h>
#include "tf2bot_engineer_repair_object.h"

CTF2BotEngineerRepairObjectTask::CTF2BotEngineerRepairObjectTask(CBaseEntity* object) :
	m_object(object)
{
	m_sentry = false;
	m_useRescueRanger = false;
}

TaskResult<CTF2Bot> CTF2BotEngineerRepairObjectTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	if (m_object.Get() == nullptr)
	{
		return Done("Goal object is no longer valid");
	}

	if (UtilHelpers::FClassnameIs(m_object.Get(), "obj_sentrygun"))
	{
		m_sentry = true;
	}

	const CTF2BotWeapon* rescueranger = bot->GetInventoryInterface()->GetTheRescueRanger();

	if (rescueranger)
	{
		m_useRescueRanger = true;
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotEngineerRepairObjectTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (m_object.Get() == nullptr)
	{
		return Done("Goal object is no longer valid");
	}

	if (bot->GetAmmoOfIndex(TeamFortress2::TF_AMMO_METAL) == 0)
	{
		CBaseEntity* source = nullptr;

		if (CTF2BotFindAmmoTask::IsPossible(bot, &source))
		{
			return PauseFor(new CTF2BotFindAmmoTask(source, 100), "Need more metal!");
		}
	}

	tfentities::HBaseObject object(m_object.Get());

	bool needsRepair = object.GetHealthPercentage() < 0.99f || object.IsSapped();

	if (m_sentry)
	{
		int shells = 999;
		entprops->GetEntProp(object.GetIndex(), Prop_Send, "m_iAmmoShells", shells);
		int rockets = 999;
		entprops->GetEntProp(object.GetIndex(), Prop_Send, "m_iAmmoRockets", rockets);

		if (shells <= 120 || rockets <= 15)
		{
			needsRepair = true;
		}
	}

	if (!needsRepair)
	{
		return Done("Object is repaied!");
	}

	auto myweapon = bot->GetInventoryInterface()->GetActiveTFWeapon();
	Vector center = object.WorldSpaceCenter();
	const float range = bot->GetRangeTo(center);

	if (myweapon)
	{
		if (m_useRescueRanger && range > 256.0f && bot->IsLineOfFireClear(center) && object.GetHealthPercentage() < 0.99f && !object.IsSapped())
		{
			if (!myweapon->IsTheRescueRanger())
			{
				const CTF2BotWeapon* rescueranger = bot->GetInventoryInterface()->GetTheRescueRanger();

				if (rescueranger)
				{
					bot->SelectWeapon(rescueranger->GetEntity());
				}
				else
				{
					m_useRescueRanger = false;
				}
			}
			else if (myweapon->IsLoaded())
			{
				bot->GetControlInterface()->AimAt(m_object.Get(), IPlayerController::LookPriority::LOOK_PRIORITY, 1.0f, "Aiming the rescue ranger!");

				if (bot->GetControlInterface()->IsAimOnTarget())
				{
					bot->GetControlInterface()->PressAttackButton();
				}
			}
		}
		else
		{
			if (myweapon->GetWeaponInfo()->GetSlot() != static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Melee))
			{
				CBaseEntity* wrench = bot->GetWeaponOfSlot(static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Melee));

				if (wrench)
				{
					bot->SelectWeapon(wrench);
				}
			}
		}
	}

	if (range > get_object_melee_range())
	{
		if (!m_nav.IsValid() || m_nav.GetAge() > 3.0f)
		{
			CTF2BotPathCost cost(bot);
			
			if (!m_nav.ComputePathToPosition(bot, object.GetAbsOrigin(), cost))
			{
				return Done("No path to object.");
			}
		}

		m_nav.Update(bot);

		// crouch early for precise movement when going for teleporters
		if (std::strcmp(gamehelpers->GetEntityClassname(m_object.Get()), "obj_teleporter") == 0 && range <= 256.0f)
		{
			bot->GetControlInterface()->PressCrouchButton();
		}
	}
	else
	{
		bot->GetControlInterface()->AimAt(object.WorldSpaceCenter(), IPlayerController::LOOK_PRIORITY, 0.5f, "Looking at object to repair it.");
		bot->GetControlInterface()->PressAttackButton(0.5f);
		bot->GetControlInterface()->PressCrouchButton(0.5f);
	}

	return Continue();
}

QueryAnswerType CTF2BotEngineerRepairObjectTask::ShouldAttack(CBaseBot* me, const CKnownEntity* them)
{
	if (UtilHelpers::IsPlayerIndex(them->GetIndex()))
	{
		if (tf2lib::GetPlayerClassType(them->GetIndex()) == TeamFortress2::TFClass_Spy)
		{
			return ANSWER_YES; // always attack spies
		}
	}

	if (m_sentry)
	{
		return ANSWER_NO; // the sentry will deal with them
	}

	return ANSWER_UNDEFINED;
}
