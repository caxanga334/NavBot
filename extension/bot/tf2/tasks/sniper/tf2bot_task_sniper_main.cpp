#include NAVBOT_PCH_FILE
#include <cstring>
#include <extension.h>
#include <mods/tf2/teamfortress2mod.h>
#include <bot/tf2/tf2bot.h>
#include <bot/tf2/tasks/tf2bot_scenario_task.h>
#include <bot/tf2/tasks/scenario/tf2bot_map_ctf.h>
#include <bot/tf2/tasks/scenario/specialdelivery/tf2bot_sd_deliver_flag.h>
#include <bot/tf2/tasks/scenario/controlpoints/tf2bot_attack_controlpoint.h>
#include "tf2bot_task_sniper_main.h"
#include "tf2bot_task_sniper_move_to_sniper_spot.h"
#include "tf2bot_task_sniper_push.h"

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
	if (bot->GetDifficultyProfile()->GetAggressiveness() >= 50 && CBaseBot::s_botrng.GetRandomInt<int>(0, 1) == 1)
	{
		return PauseFor(new CTF2BotSniperPushTask, "Pushing to the objective!");
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
