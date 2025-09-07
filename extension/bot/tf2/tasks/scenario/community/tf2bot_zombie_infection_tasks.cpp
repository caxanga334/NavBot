#include NAVBOT_PCH_FILE
#include <extension.h>
#include <mods/tf2/tf2lib.h>
#include <mods/tf2/teamfortress2mod.h>
#include <bot/tf2/tf2bot.h>
#include <bot/interfaces/path/chasenavigator.h>
#include <bot/tf2/tasks/tf2bot_scenario_task.h>
#include <bot/tasks_shared/bot_shared_attack_enemy.h>
#include <bot/tasks_shared/bot_shared_go_to_position.h>
#include <bot/tasks_shared/bot_shared_search_area.h>
#include <bot/tasks_shared/bot_shared_roam.h>
#include <bot/tasks_shared/bot_shared_defend_spot.h>
#include <bot/tasks_shared/bot_shared_retreat_from_threat.h>
#include <bot/tasks_shared/bot_shared_seek_and_destroy_entity.h>
#include "tf2bot_zombie_infection_tasks.h"

namespace zi
{
	static bool IsPlayerRevealed(CBaseEntity* player)
	{
		bool result = false;
		entprops->GetEntPropBool(player, Prop_Send, "m_bGlowEnabled", result);
		return result;
	}
}

CTF2BotZIMonitorTask::CTF2BotZIMonitorTask()
{
	m_isUsingClassBehavior = false;
	m_isZombie = false;
}

AITask<CTF2Bot>* CTF2BotZIMonitorTask::InitialNextTask(CTF2Bot* bot)
{
	if (bot->GetMyTFTeam() == TeamFortress2::TFTeam::TFTeam_Red)
	{
		AITask<CTF2Bot>* classBehavior = CTF2BotScenarioTask::SelectClassTask(bot);

		if (classBehavior)
		{
			m_isUsingClassBehavior = true;
			return classBehavior;
		}

		return new CTF2BotZISurvivorBehaviorTask;
	}

	m_isZombie = true;
	return new CTF2BotZIZombieBehaviorTask;
}

TaskResult<CTF2Bot> CTF2BotZIMonitorTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotZIMonitorTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (m_isZombie)
	{
		if (m_zombieThinkTimer.IsElapsed())
		{
			m_zombieThinkTimer.Start(0.5f);

			DetectRevealedPlayers(bot);

			if (m_zombieAbilityCooldown.IsElapsed())
			{
				if (ShouldUseAbility(bot))
				{
					m_zombieAbilityCooldown.Start(12.0f);
					bot->GetControlInterface()->PressSecondaryAttackButton(0.5f);
				}
			}
		}
	}
	else
	{
		if (bot->GetMyTFTeam() == TeamFortress2::TFTeam::TFTeam_Blue)
		{
			return SwitchTo(new CTF2BotZIMonitorTask, "I have become a zombie, restarting ZI behavior!");
		}
	}

	return Continue();
}

void CTF2BotZIMonitorTask::DetectRevealedPlayers(CTF2Bot* me) const
{
	CTF2BotSensor* sensor = me->GetSensorInterface();

	auto func = [&sensor](int client, edict_t* entity, SourceMod::IGamePlayer* player) {
		CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(client);

		if (player->IsInGame() && pEntity)
		{
			if (tf2lib::GetEntityTFTeam(client) == TeamFortress2::TFTeam::TFTeam_Red)
			{
				bool revealed = false;
				entprops->GetEntPropBool(client, Prop_Send, "m_bGlowEnabled", revealed);

				if (revealed)
				{
					CKnownEntity* known = sensor->AddKnownEntity(pEntity);
					known->UpdatePosition();
				}
			}
		}
	};

	UtilHelpers::ForEachPlayer(func);
}

bool CTF2BotZIMonitorTask::ShouldUseAbility(CTF2Bot* me) const
{
	TeamFortress2::TFClassType myclass = me->GetMyClassType();
	const CKnownEntity* threat = me->GetSensorInterface()->GetPrimaryKnownThreat();

	switch (myclass)
	{
	case TeamFortress2::TFClass_Soldier:
		[[fallthrough]];
	case TeamFortress2::TFClass_Sniper:
		[[fallthrough]];
	case TeamFortress2::TFClass_Pyro:
		return (threat && threat->IsVisibleNow());
	case TeamFortress2::TFClass_DemoMan:
		return (threat && threat->IsVisibleNow() && threat->IsPlayer() && me->GetRangeTo(threat->GetLastKnownPosition()) <= 600.0f);
	case TeamFortress2::TFClass_Engineer:
		return (threat && threat->IsVisibleNow() && threat->IsEntityOfClassname("obj_sentrygun"));
	case TeamFortress2::TFClass_Medic:
		return me->GetHealthPercentage() <= me->GetDifficultyProfile()->GetHealthLowThreshold();
	case TeamFortress2::TFClass_Spy:
		return (!threat || !threat->IsVisibleNow());
	default:
		break;
	}

	return false;
}

TaskResult<CTF2Bot> CTF2BotZISurvivorBehaviorTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotZISurvivorBehaviorTask::OnTaskUpdate(CTF2Bot* bot)
{
	CTF2BotSensor* sensor = bot->GetSensorInterface();

	if (sensor->GetVisibleEnemiesCount() > 2 && sensor->GetVisibleEnemiesCount() > sensor->GetVisibleAlliesCount())
	{
		const CKnownEntity* threat = sensor->GetPrimaryKnownThreat();

		if ((bot->GetAbsOrigin() - threat->GetLastKnownPosition()).IsLengthLessThan(512.0f))
		{
			return PauseFor(new CBotSharedRetreatFromThreatTask<CTF2Bot, CTF2BotPathCost>(bot, false), "Retreating!");
		}
	}

	// increase the camp/defend chance on ZI for suvivors
	int defendChance = CTeamFortress2Mod::GetTF2Mod()->GetTF2ModSettings()->GetDefendRate() * 3;
	defendChance = std::clamp(defendChance, 0, 75);

	if (CBaseBot::s_botrng.GetRandomChance(defendChance))
	{
		// Survivor bots will use a random defend waypoint and stay there
		CWaypoint* waypoint = botsharedutils::waypoints::GetRandomDefendWaypoint(bot, nullptr, -1.0f);

		if (waypoint)
		{
			return PauseFor(new CBotSharedDefendSpotTask<CTF2Bot, CTF2BotPathCost>(bot, waypoint, CBaseBot::s_botrng.GetRandomReal<float>(10.0f, 20.0f), true), "Camping");
		}
	}

	// Roam if no defend waypoint is available
	return PauseFor(new CBotSharedRoamTask<CTF2Bot, CTF2BotPathCost>(bot, 10000.0f, false, -1.0f, false), "Roaming!");
}

TaskResult<CTF2Bot> CTF2BotZIZombieBehaviorTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotZIZombieBehaviorTask::OnTaskUpdate(CTF2Bot* bot)
{
	CTF2BotSensor* sensor = bot->GetSensorInterface();

	const CKnownEntity* threat = sensor->GetPrimaryKnownThreat();

	if (threat)
	{
		if (threat->IsPlayer())
		{
			bool glowing = false;
			entprops->GetEntPropBool(threat->GetEntity(), Prop_Send, "m_bGlowEnabled", glowing);

			if (glowing)
			{
				auto task = new CBotSharedSeekAndDestroyEntityTask<CTF2Bot, CTF2BotPathCost>(bot, threat->GetEntity());
				auto func = std::bind(zi::IsPlayerRevealed, std::placeholders::_1);
				task->SetValidatorFunction(func);
				return PauseFor(task, "Chasing releaved survivor!");
			}
		}

		if (threat->IsVisibleNow())
		{
			return PauseFor(new CBotSharedAttackEnemyTask<CTF2Bot, CTF2BotPathCost>(bot, CBaseBot::s_botrng.GetRandomReal<float>(6.0f, 12.0f)), "Attacking visible survivor!");
		}

		return PauseFor(new CBotSharedSearchAreaTask<CTF2Bot, CTF2BotPathCost>(bot, threat->GetLastKnownPosition(), 128.0f, 8000.0f), "Searching for lost enemy!");
	}

	return PauseFor(new CBotSharedRoamTask<CTF2Bot, CTF2BotPathCost>(bot, 10000.0f, true, -1.0f, true), "Roaming!");
}
