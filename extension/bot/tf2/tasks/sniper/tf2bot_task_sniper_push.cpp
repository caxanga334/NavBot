#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tf2bot.h>
#include <bot/bot_shared_utils.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <bot/tf2/tasks/tf2bot_find_ammo_task.h>
#include "tf2bot_task_sniper_push.h"

CTF2BotSniperPushTask::CTF2BotSniperPushTask() :
	m_goal(0.0f, 0.0f, 0.0f)
{
}

TaskResult<CTF2Bot> CTF2BotSniperPushTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	m_endpushtimer.StartRandom(90.0f, 180.0f);
	SelectPushGoal(bot);
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotSniperPushTask::OnTaskUpdate(CTF2Bot* bot)
{
	const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat();
	bool move = true;
	constexpr auto SCOPE_IN_TIME = 10.0f;

	const CTF2BotWeapon* weapon = bot->GetInventoryInterface()->GetActiveTFWeapon();

	if (!weapon)
	{
		return Done("No weapon!");
	}

	if (weapon->GetTF2Info()->GetSlot() != static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Primary))
	{
		CBaseEntity* sniper = bot->GetWeaponOfSlot(static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Primary));

		if (!sniper)
		{
			return Done("No weapon!");
		}

		bot->SelectWeapon(sniper);
		return Continue();
	}

	if (weapon->IsAmmoLow(bot, false))
	{
		CBaseEntity* source = nullptr;

		if (CTF2BotFindAmmoTask::IsPossible(bot, &source))
		{
			return SwitchTo(new CTF2BotFindAmmoTask(source), "Need ammo!");
		}
	}

	if (threat)
	{
		if (threat->IsVisibleNow())
		{
			botsharedutils::aiming::SelectDesiredAimSpotForTarget(bot, threat->GetEntity());
			bot->GetControlInterface()->AimAt(threat->GetEntity(), IPlayerController::LOOK_COMBAT, 0.5f, "Looking at visible threat!");

			if (m_firetimer.IsElapsed())
			{
				m_firetimer.StartRandom(1.0f, 2.0f);
				bot->GetControlInterface()->PressAttackButton();
			}

			move = false;
		}
		else
		{
			if (bot->GetSensorInterface()->GetTimeSinceVisibleThreat() <= 1.0f)
			{
				bot->GetControlInterface()->AimAt(threat->GetLastKnownPosition(), IPlayerController::LOOK_ALERT, 1.0f, "Looking at threat LKP!");
			}
		}

		if (threat->IsVisibleNow() || bot->GetSensorInterface()->GetTimeSinceVisibleThreat() <= SCOPE_IN_TIME)
		{
			// scope
			if (!bot->IsScopedIn())
			{
				bot->GetControlInterface()->PressSecondaryAttackButton();
			}
		}
		else
		{
			// unscope
			if (bot->IsScopedIn())
			{
				bot->GetControlInterface()->PressSecondaryAttackButton();
			}
		}
	}
	else
	{
		// unscope
		if (bot->IsScopedIn())
		{
			bot->GetControlInterface()->PressSecondaryAttackButton();
		}
	}

	if (move)
	{
		if (m_repathtimer.IsElapsed())
		{
			m_repathtimer.Start(2.0f);
			CTF2BotPathCost cost(bot);
			m_nav.ComputePathToPosition(bot, m_goal, cost);
		}

		m_nav.Update(bot);
	}


	return Continue();
}

void CTF2BotSniperPushTask::OnTaskEnd(CTF2Bot* bot, AITask<CTF2Bot>* nextTask)
{
	if (bot->IsScopedIn())
	{
		bot->GetControlInterface()->PressSecondaryAttackButton();
	}
}

TaskEventResponseResult<CTF2Bot> CTF2BotSniperPushTask::OnMoveToSuccess(CTF2Bot* bot, CPath* path)
{
	SelectPushGoal(bot);
	return TryContinue();
}

void CTF2BotSniperPushTask::SelectPushGoal(CTF2Bot* bot)
{
	CTeamFortress2Mod* tf2mod = CTeamFortress2Mod::GetTF2Mod();
	TeamFortress2::GameModeType gamemode = tf2mod->GetCurrentGameMode();

	switch (gamemode)
	{
	case TeamFortress2::GameModeType::GM_CTF:
	{
		edict_t* flag = bot->GetFlagToDefend();
		
		if (flag)
		{
			Vector pos = tf2lib::GetFlagPosition(UtilHelpers::EdictToBaseEntity(flag));

			if (bot->GetRangeTo(pos) <= 512.0f)
			{
				break;
			}
			else
			{
				m_goal = pos;
				return;
			}
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

		m_goal = UtilHelpers::getEntityOrigin(randomGoal);
		return;
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
			else
			{
				m_goal = pos;
				return;
			}
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
		m_goal = UtilHelpers::getEntityOrigin(gamehelpers->EdictOfIndex(librandom::utils::GetRandomElementFromVector(players)));
		return;
	}

	botsharedutils::RandomDestinationCollector collector(bot, 8192.0f);
	collector.Execute();
	
	if (collector.IsCollectedAreasEmpty())
	{
		m_goal = bot->GetAbsOrigin();
	}
	else
	{
		CNavArea* area = collector.GetRandomCollectedArea();
		m_goal = area->GetRandomPoint();
	}
}
