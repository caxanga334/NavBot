#include <array>
#include <string_view>
#include <limits>
#include <extension.h>
#include <util/helpers.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tf2bot.h>
#include "tf2bot_engineer_repair_object.h"
#include "tf2bot_engineer_upgrade_object.h"
#include "tf2bot_engineer_nest.h"
#include "tf2bot_engineer_main.h"

#undef clamp
#undef max
#undef min

CTF2BotEngineerMainTask::CTF2BotEngineerMainTask()
{
	m_friendlyBuildingScan.StartRandom();
}

AITask<CTF2Bot>* CTF2BotEngineerMainTask::InitialNextTask(CTF2Bot* bot)
{
	return new CTF2BotEngineerNestTask;
}

TaskResult<CTF2Bot> CTF2BotEngineerMainTask::OnTaskUpdate(CTF2Bot* bot)
{
	/*
	* @TODO
	* - Help allied engineers
	* - Listen and respond to help requests from other engineers
	*/

	bool upgrade = false;
	CBaseEntity* allyBuilding = ScanAllyBuildings(bot, upgrade);

	if (allyBuilding != nullptr)
	{
		if (upgrade)
		{
			return PauseFor(new CTF2BotEngineerUpgradeObjectTask(allyBuilding), "Upgrading ally building!");
		}
		else
		{
			return PauseFor(new CTF2BotEngineerRepairObjectTask(allyBuilding), "Repairing ally building!");
		}
	}

	return Continue();
}

CBaseEntity* CTF2BotEngineerMainTask::ScanAllyBuildings(CTF2Bot* me, bool& is_upgrade)
{
	using namespace std::literals::string_view_literals;

	constexpr std::array objects_classnames = {
		"obj_sentrygun"sv,
		"obj_dispenser"sv,
		"obj_teleporter"sv
	};

	if (m_friendlyBuildingScan.IsElapsed())
	{
		m_friendlyBuildingScan.StartRandom();

		CBaseEntity* building = nullptr;
		float best = std::numeric_limits<float>::max();
		bool upgrade_or_repair = false; // repair on false, upgrade on true
		TeamFortress2::TFTeam myteam = me->GetMyTFTeam();
		auto functor = [&me, &building, &best, &myteam, &upgrade_or_repair](int index, edict_t* edict, CBaseEntity* entity) {
			if (entity != nullptr && tf2lib::GetEntityTFTeam(index) == myteam &&
				tf2lib::GetBuildingBuilder(entity) != me->GetEntity() && tf2lib::IsBuildingPlaced(entity))
			{
				if (tf2lib::BuildingNeedsToBeRepaired(entity))
				{
					float distance = me->GetRangeToSqr(entity);

					if (distance <= CTF2BotEngineerMainTask::HELP_ALLY_BUILDING_MAX_RANGE_SQR && distance < best)
					{
						best = distance;
						building = entity;
						upgrade_or_repair = false; // repair
					}
				}
				else if (!tf2lib::IsBuildingAtMaxUpgradeLevel(entity))
				{
					float distance = me->GetRangeToSqr(entity);

					if (distance <= CTF2BotEngineerMainTask::HELP_ALLY_BUILDING_MAX_RANGE_SQR && distance < best)
					{
						best = distance;
						building = entity;
						upgrade_or_repair = true; // upgrade
					}
				}
			}

			return true;
		};

		for (auto& classname : objects_classnames)
		{
			UtilHelpers::ForEachEntityOfClassname(classname.data(), functor);
		}

		is_upgrade = upgrade_or_repair;
		return building;
	}

	return nullptr;
}
