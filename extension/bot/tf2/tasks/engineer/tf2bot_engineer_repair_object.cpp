#include <extension.h>
#include <util/helpers.h>
#include <entities/tf2/tf_entities.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tf2bot.h>
#include <bot/tf2/tasks/tf2bot_find_ammo_task.h>
#include "tf2bot_engineer_repair_object.h"

CTF2BotEngineerRepairObjectTask::CTF2BotEngineerRepairObjectTask(CBaseEntity* object) :
	m_object(object)
{
	m_sentry = false;
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
		return PauseFor(new CTF2BotFindAmmoTask, "Need more metal!");
	}

	tfentities::HBaseObject object(m_object.Get());

	const bool needsRepair = object.GetHealthPercentage() < 0.9999f || object.IsSapped();

	if (!needsRepair)
	{
		return Done("Object is repaied!");
	}

	/* TODO position behind sentry if under attack by enemy */

	auto myweapon = bot->GetActiveBotWeapon();

	if (myweapon && myweapon->GetModWeaponID<TeamFortress2::TFWeaponID>() != TeamFortress2::TFWeaponID::TF_WEAPON_WRENCH)
	{
		CBaseEntity* wrench = bot->GetWeaponOfSlot(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Melee);

		if (wrench)
		{
			bot->SelectWeapon(wrench);
		}
	}

	if (bot->GetRangeTo(object.WorldSpaceCenter()) > get_object_melee_range())
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
	}
	else
	{
		bot->GetControlInterface()->AimAt(object.WorldSpaceCenter(), IPlayerController::LOOK_VERY_IMPORTANT, 0.5f, "Looking at object to repair it.");
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
