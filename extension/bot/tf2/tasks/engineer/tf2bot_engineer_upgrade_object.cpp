#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/helpers.h>
#include <entities/tf2/tf_entities.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tf2bot.h>
#include <bot/tf2/tasks/tf2bot_find_ammo_task.h>
#include "tf2bot_engineer_upgrade_object.h"

CTF2BotEngineerUpgradeObjectTask::CTF2BotEngineerUpgradeObjectTask(CBaseEntity* object) :
	m_object(object)
{
}

TaskResult<CTF2Bot> CTF2BotEngineerUpgradeObjectTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	if (m_object.Get() == nullptr)
	{
		return Done("Goal object is no longer valid");
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotEngineerUpgradeObjectTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (m_object.Get() == nullptr)
	{
		return Done("Goal object is no longer valid");
	}

	tfentities::HBaseObject object(m_object.Get());

	if (object.GetLevel() == object.GetMaxLevel())
	{
		return Done("Object is at max level!");
	}

	if (bot->GetAmmoOfIndex(TeamFortress2::TF_AMMO_METAL) == 0)
	{
		CBaseEntity* source = nullptr;

		if (CTF2BotFindAmmoTask::IsPossible(bot, &source))
		{
			return PauseFor(new CTF2BotFindAmmoTask(source, object.GetMetalNeededToUpgrade()), "Need more metal!");
		}
		else
		{
			if (bot->IsDebugging(BOTDEBUG_ERRORS))
			{
				const Vector origin = bot->GetAbsOrigin();

				bot->DebugPrintToConsole(255, 0, 0, "%s ERROR: UpgradeObjectTask bot needs more metal to build but CTF2BotFindAmmoTask::IsPossible returned false! Team: %i Position: %g %g %g", bot->GetDebugIdentifier(), bot->GetCurrentTeamIndex(), origin.x, origin.y, origin.z);
			}

			return Done("Out of metal and no ammo source nearby!");
		}
	}

	auto myweapon = bot->GetInventoryInterface()->GetActiveBotWeapon();

	if (myweapon && myweapon->GetWeaponInfo()->GetSlot() != static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Melee))
	{
		CBaseEntity* wrench = bot->GetWeaponOfSlot(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Melee);

		if (wrench)
		{
			bot->SelectWeapon(wrench);
		}
	}

	const float range = bot->GetRangeTo(object.WorldSpaceCenter());

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
		bot->GetControlInterface()->AimAt(object.WorldSpaceCenter(), IPlayerController::LOOK_PRIORITY, 0.5f, "Looking at object to upgrade it.");
		bot->GetControlInterface()->PressAttackButton(0.5f);
		bot->GetControlInterface()->PressCrouchButton(0.5f);
	}

	return Continue();
}
