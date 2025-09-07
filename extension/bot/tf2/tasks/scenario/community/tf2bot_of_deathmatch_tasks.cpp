#include NAVBOT_PCH_FILE
#include <extension.h>
#include <mods/tf2/tf2lib.h>
#include <mods/tf2/teamfortress2mod.h>
#include <bot/tf2/tf2bot.h>
#include <bot/tasks_shared/bot_shared_simple_deathmatch_behavior.h>
#include <bot/tasks_shared/bot_shared_go_to_position.h>
#include "tf2bot_of_deathmatch_tasks.h"

AITask<CTF2Bot>* CTF2BotOFDMMonitorTask::InitialNextTask(CTF2Bot* bot)
{
	/* Use simple DM for now */
	return new CBotSharedSimpleDMBehaviorTask<CTF2Bot, CTF2BotPathCost>();
}

TaskResult<CTF2Bot> CTF2BotOFDMMonitorTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotOFDMMonitorTask::OnTaskUpdate(CTF2Bot* bot)
{
	const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(ISensor::ONLY_VISIBLE_THREATS);

	if (threat)
	{
		SelectRandomWeapon(bot);
	}

	if (m_collectWeaponTimer.IsElapsed())
	{
		m_collectWeaponTimer.StartRandom(60.0f, 90.0f);

		CBaseEntity* weapon = CollectRandomWeapon(bot);

		if (weapon)
		{
			Vector pos = UtilHelpers::getWorldSpaceCenter(weapon);
			pos = trace::getground(pos);

			return PauseFor(new CBotSharedGoToPositionTask<CTF2Bot, CTF2BotPathCost>(bot, pos, "PickupItem", true, true, true), "Collecting random weapon!");
		}
	}

	return Continue();
}

void CTF2BotOFDMMonitorTask::SelectRandomWeapon(CTF2Bot* bot)
{
	if (!m_weaponSelectCooldownTimer.IsElapsed())
		return;

	m_weaponSelectCooldownTimer.StartRandom(3.0f, 8.0f);

	std::vector<CBaseEntity*> guns;

	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		CBaseEntity* weapon = nullptr;
		entprops->GetEntPropEnt(bot->GetEntity(), Prop_Send, "m_hMyWeapons", nullptr, &weapon, i);

		if (!weapon) { continue; }

		char targetname[256];
		std::size_t length = 0U;
		entprops->GetEntPropString(weapon, Prop_Data, "m_iName", targetname, sizeof(targetname), length);

		if (length == 0U) { continue; }

		int slotNumber = -1;

		// parse the targetname
		int c = sscanf(targetname, "slot %i", &slotNumber);

		// parse failed
		if (c <= 0) { continue; }

		if (slotNumber < 0) { continue; }

		guns.push_back(weapon);
	}

	if (guns.empty()) { return; }

	CBaseEntity* wep = librandom::utils::GetRandomElementFromVector(guns);
	bot->SelectWeapon(wep);
}

CBaseEntity* CTF2BotOFDMMonitorTask::CollectRandomWeapon(CTF2Bot* bot) const
{
	std::vector<CBaseEntity*> weapons;
	const Vector origin = bot->GetAbsOrigin();
	const float maxRange = CTeamFortress2Mod::GetTF2Mod()->GetModSettings()->GetCollectItemMaxDistance();

	auto func = [&weapons, origin, maxRange](int index, edict_t* edict, CBaseEntity* entity) {

		if (entity)
		{
			const float range = (origin - UtilHelpers::getEntityOrigin(entity)).Length();

			if (range <= maxRange)
			{
				char targetname[256];
				std::size_t length = 0U;
				entprops->GetEntPropString(entity, Prop_Data, "m_iName", targetname, sizeof(targetname), length);

				if (length > 1)
				{
					if (std::strcmp(targetname, "tiny_pill") == 0)
					{
						return true;
					}

					if (std::strstr(targetname, "ammo_") != nullptr)
					{
						return true;
					}

					weapons.push_back(entity);
				}
			}
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("item_armor", func);

	if (weapons.empty())
	{
		return nullptr;
	}

	return librandom::utils::GetRandomElementFromVector(weapons);
}
