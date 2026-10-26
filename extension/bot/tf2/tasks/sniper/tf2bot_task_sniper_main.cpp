#include NAVBOT_PCH_FILE
#include <cstring>
#include <extension.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tf2bot.h>
#include <bot/tf2/tf2bot_utils.h>
#include <bot/tf2/tasks/tf2bot_scenario_task.h>
#include <bot/tf2/tasks/scenario/tf2bot_map_ctf.h>
#include <bot/tf2/tasks/scenario/specialdelivery/tf2bot_sd_deliver_flag.h>
#include <bot/tf2/tasks/scenario/controlpoints/tf2bot_attack_controlpoint.h>
#include "tf2bot_task_sniper_move_to_sniper_spot.h"
#include <bot/tasks_shared/bot_shared_sniper_rush.h>
#include "tf2bot_task_sniper_main.h"


CTF2BotSniperMainTask::CTF2BotSniperMainTask()
{
}

TaskResult<CTF2Bot> CTF2BotSniperMainTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotSniperMainTask::OnTaskUpdate(CTF2Bot* bot)
{
	// aggressive sniper bots have 50% to push towards the objective
	if (bot->GetDifficultyProfile()->GetAggressiveness() >= 50 && CBaseBot::s_botrng.GetRandomChance())
	{
		return PauseFor(new CBotSharedSniperRushTask<CTF2Bot, CTF2BotPathCost>(GetRushGoal(bot)), "Pushing to the objective!");
	}

	auto tf2mod = CTeamFortress2Mod::GetTF2Mod();
	auto gm = tf2mod->GetCurrentGameMode();

	if (gm == TeamFortress2::GameModeType::GM_CP || gm == TeamFortress2::GameModeType::GM_ADCP)
	{
		if (bot->GetTimeLeftToCapture() < 30.0f) /* team is about to lose, don't snipe, go try capture the control point */
		{
			std::vector<CBaseEntity*> attackPoints;
			tf2mod->CollectControlPointsToAttack(bot->GetMyTFTeam(), attackPoints);

			if (!attackPoints.empty())
			{
				CBaseEntity* target = librandom::utils::GetRandomElementFromVector<CBaseEntity*>(attackPoints);
				return PauseFor(new CTF2BotAttackControlPointTask(target), "Attacking control point!");
			}
		}
	}

	/* time to go sniping */
	return PauseFor(new CTF2BotSniperMoveToSnipingSpotTask, "Sniping!");
}

TaskEventResponseResult<CTF2Bot> CTF2BotSniperMainTask::OnFlagTaken(CTF2Bot* bot, CBaseEntity* player)
{
	if (bot->GetEntity() == player)
	{
		switch (CTeamFortress2Mod::GetTF2Mod()->GetCurrentGameMode())
		{
		case TeamFortress2::GameModeType::GM_CTF:
		{
			return TryPauseFor(new CTF2BotCTFDeliverFlagTask, PRIORITY_HIGH, "I have the flag!");
		}
		case TeamFortress2::GameModeType::GM_SD:
		{
			return TryPauseFor(new CTF2BotSDDeliverFlag, PRIORITY_HIGH, "I have the australium!");
		}
		default:
			break;
		}
	}

	return TryContinue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotSniperMainTask::OnWeaponEquip(CTF2Bot* bot, CBaseEntity* weapon)
{
	if (weapon && UtilHelpers::FClassnameIs(weapon, "tf_weapon_compound_bow"))
	{
		AITask<CTF2Bot>* task = CTF2BotScenarioTask::SelectScenarioTask(bot, true);
		return TrySwitchTo(task, PRIORITY_MANDATORY, "Bow equipped, existing sniper behavior.");
	}

	return TryContinue();
}

Vector CTF2BotSniperMainTask::GetRushGoal(CTF2Bot* bot) const
{
	CTeamFortress2Mod* tf2mod = CTeamFortress2Mod::GetTF2Mod();
	TeamFortress2::GameModeType gamemode = tf2mod->GetCurrentGameMode();

	switch (gamemode)
	{
	case TeamFortress2::GameModeType::GM_CTF:
	{
		if (CBaseBot::s_botrng.GetRandomChance())
		{
			edict_t* flag = bot->GetFlagToFetch();

			if (flag)
			{
				return tf2lib::GetFlagPosition(UtilHelpers::EdictToBaseEntity(flag));
			}
		}

		edict_t* flag = bot->GetFlagToDefend();

		if (flag)
		{
			Vector pos = tf2lib::GetFlagPosition(UtilHelpers::EdictToBaseEntity(flag));

			if (bot->GetRangeTo(pos) <= 512.0f)
			{
				break;
			}

			return pos;
		}

		break;
	}
	case TeamFortress2::GameModeType::GM_CP:
		[[fallthrough]];
	case TeamFortress2::GameModeType::GM_ADCP:
		[[fallthrough]];
	case TeamFortress2::GameModeType::GM_KOTH:
		[[fallthrough]];
	case TeamFortress2::GameModeType::GM_ARENA:
		[[fallthrough]];
	case TeamFortress2::GameModeType::GM_PL:
		[[fallthrough]];
	case TeamFortress2::GameModeType::GM_PL_RACE:
		[[fallthrough]];
	case TeamFortress2::GameModeType::GM_TC:
	{
		// control point based game modes
		std::vector<CBaseEntity*> points;
		points.reserve(8);

		tf2mod->CollectControlPointsToAttack(bot->GetMyTFTeam(), points);

		if (points.empty())
		{
			tf2mod->CollectControlPointsToDefend(bot->GetMyTFTeam(), points);
		}

		if (points.empty())
		{
			break;
		}

		CBaseEntity* randomGoal = librandom::utils::GetRandomElementFromVector(points);

		// already near it
		if (bot->GetRangeTo(randomGoal) <= 512.0f)
		{
			break;
		}

		return UtilHelpers::getEntityOrigin(randomGoal);
	}
	case TeamFortress2::GameModeType::GM_PD:
		break;
	case TeamFortress2::GameModeType::GM_SD:
		break;
	case TeamFortress2::GameModeType::GM_MVM:
	{
		CBaseEntity* bomb = tf2lib::mvm::GetMostDangerousFlag();

		if (bomb)
		{
			Vector pos = tf2lib::GetFlagPosition(bomb);

			if (bot->GetRangeTo(pos) <= 256.0f)
			{
				break;
			}
			
			return pos;
		}

		break;
	}
	default:
		break;
	}

	std::vector<int> players;
	players.reserve(128);
	UtilHelpers::CollectPlayers(players, [&bot](int client, edict_t* entity, SourceMod::IGamePlayer* player) {
		if (client == bot->GetIndex())
		{
			return false;
		}

		int teamnum = -2;
		entprops->GetEntProp(client, Prop_Send, "m_iTeamNum", teamnum);

		if (teamnum == static_cast<int>(bot->GetMyTFTeam()))
		{
			return true;
		}

		return false;
	});

	// move to a random teammate
	if (!players.empty() && CBaseBot::s_botrng.GetRandomInt<int>(1, 100) >= bot->GetDifficultyProfile()->GetTeamwork())
	{
		return UtilHelpers::getEntityOrigin(gamehelpers->EdictOfIndex(librandom::utils::GetRandomElementFromVector(players)));
	}

	botsharedutils::RandomDestinationCollector collector(bot, 8192.0f);
	collector.Execute();

	if (collector.IsCollectedAreasEmpty())
	{
		return bot->GetAbsOrigin();
	}

	CNavArea* area = collector.GetRandomCollectedArea();
	return area->GetRandomPoint();
}
