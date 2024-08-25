#include <extension.h>
#include <util/helpers.h>
#include <entities/tf2/tf_entities.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tf2bot.h>
#include "tf2bot_engineer_speedup_object.h"

CTF2BotEngineerSpeedUpObjectTask::CTF2BotEngineerSpeedUpObjectTask(CBaseEntity* object) :
	m_object(object)
{
}

TaskResult<CTF2Bot> CTF2BotEngineerSpeedUpObjectTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	if (m_object.Get() == nullptr)
	{
		return Done("Goal object is no longer valid");
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotEngineerSpeedUpObjectTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (m_object.Get() == nullptr)
	{
		return Done("Goal object is no longer valid");
	}

	tfentities::HBaseObject object(m_object.Get());

	if (object.GetPercentageConstructed() > 0.99f)
	{
		return Done("Object is fully constructed!");
	}

	/* TODO position behind sentry if under attack by enemy */

	auto myweapon = bot->GetInventoryInterface()->GetActiveBotWeapon();

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
		bot->GetControlInterface()->AimAt(object.WorldSpaceCenter(), IPlayerController::LOOK_VERY_IMPORTANT, 0.5f, "Looking at object to construct it.");
		bot->GetControlInterface()->PressAttackButton(0.5f);
		bot->GetControlInterface()->PressCrouchButton(0.5f);
	}

	return Continue();
}
