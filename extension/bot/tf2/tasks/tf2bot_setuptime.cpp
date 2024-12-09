#include <extension.h>
#include <util/helpers.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tf2bot.h>
#include "tf2bot_taunting.h"
#include "tf2bot_maintask.h"
#include "tf2bot_setuptime.h"

TaskResult<CTF2Bot> CTF2BotSetupTimeTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	m_nextActionTimer.StartRandom(3.0f, 10.0f);
	m_action = IDLE;
	m_target = nullptr;

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotSetupTimeTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (!CTeamFortress2Mod::GetTF2Mod()->IsInSetup())
	{
		return SwitchTo(new CTF2BotMainTask, "Setup time over, returning to main behavior!");
	}

	if (tf2lib::IsPlayerInCondition(bot->GetIndex(), TeamFortress2::TFCond::TFCond_Taunting))
	{
		return PauseFor(new CTF2BotTauntingTask, "Taunting!");
	}

	if (m_nextActionTimer.IsElapsed())
	{
		m_action = static_cast<SetupActions>(randomgen->GetRandomInt<int>(0, static_cast<int>(MAX_ACTIONS) - 1));

		m_nextActionTimer.StartRandom(3.0f, 10.0f);

		if (m_action == STARE || m_action == MELEE)
		{
			SelectRandomTarget(bot);

			if (m_target.Get() == nullptr)
			{
				m_action = IDLE;
				return Continue();
			}
		}
	}

	switch (m_action)
	{
	case CTF2BotSetupTimeTask::TAUNT:
		bot->DelayedFakeClientCommand("taunt");
		m_action = IDLE;
		break;
	case CTF2BotSetupTimeTask::LOOK:
	{
		Vector randomPoint;
		randomPoint.x = randomgen->GetRandomReal<float>(-2048.0f, 2048.0f);
		randomPoint.y = randomgen->GetRandomReal<float>(-2048.0f, 2048.0f);
		randomPoint.z = randomgen->GetRandomReal<float>(-2048.0f, 2048.0f);

		bot->GetControlInterface()->AimAt(randomPoint, IPlayerController::LOOK_INTERESTING, 0.5f, "Setup time random look!");
		break;
	}
	case CTF2BotSetupTimeTask::STARE:
	{
		CBaseEntity* pTarget = m_target.Get();

		if (pTarget != nullptr)
		{
			bot->GetControlInterface()->AimAt(pTarget, IPlayerController::LOOK_INTERESTING, 0.5f, "Staring at my target!");
		}

		break;
	}
	case CTF2BotSetupTimeTask::MELEE:
	{
		CBaseEntity* pTarget = m_target.Get();

		if (pTarget != nullptr)
		{
			bot->SelectWeapon(bot->GetWeaponOfSlot(static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Melee)));
			bot->GetControlInterface()->AimAt(pTarget, IPlayerController::LOOK_INTERESTING, 0.5f, "Staring at my target!");
			bot->GetControlInterface()->PressAttackButton(0.2f);

			CTF2BotPathCost cost(bot);
			m_nav.Update(bot, pTarget, cost);
		}

		break;
	}
	case CTF2BotSetupTimeTask::KILLBIND:
		bot->DelayedFakeClientCommand("explode");
		m_action = IDLE;
		break;
	default:
		break;
	}

	return Continue();
}

void CTF2BotSetupTimeTask::OnTaskEnd(CTF2Bot* bot, AITask<CTF2Bot>* nextTask)
{
	bot->SelectWeapon(bot->GetWeaponOfSlot(static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Primary)));
}

void CTF2BotSetupTimeTask::SelectRandomTarget(CTF2Bot* me)
{
	std::vector<CBaseEntity*> targets;

	UtilHelpers::ForEachPlayer([&me, &targets](int client, edict_t* entity, SourceMod::IGamePlayer* player) {

		if (client != me->GetIndex() && player->IsInGame() && UtilHelpers::IsPlayerAlive(client) && tf2lib::GetEntityTFTeam(client) == me->GetMyTFTeam())
		{
			targets.push_back(gamehelpers->ReferenceToEntity(client));
		}
	});

	if (targets.empty())
	{
		m_target = nullptr;
		return;
	}

	m_target = librandom::utils::GetRandomElementFromVector<CBaseEntity*>(targets);
}
