#include <vector>
#include <extension.h>
#include <util/entprops.h>
#include <util/librandom.h>
#include <mods/tf2/tf2lib.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/nav/tfnavmesh.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <bot/tf2/tf2bot.h>
#include <bot/tf2/tasks/engineer/tf2bot_engineer_main.h>
#include <bot/tf2/tasks/medic/tf2bot_medic_main_task.h>
#include "tf2bot_mvm_upgrade.h"
#include "tf2bot_mvm_idle.h"

class CTF2BotMvMIdleSelectRandomFrontlineArea
{
public:

	bool operator()(CNavArea* area)
	{
		CTFNavArea* tfarea = static_cast<CTFNavArea*>(area);

		if (tfarea->HasMVMAttributes(CTFNavArea::MvMNavAttributes::MVMNAV_FRONTLINES))
		{
			frontlines.push_back(tfarea);
		}

		return true;
	}

	CTFNavArea* GetRandomFrontlineArea()
	{
		if (frontlines.empty()) { return nullptr; }

		return librandom::utils::GetRandomElementFromVector(frontlines);
	}

private:
	std::vector<CTFNavArea*> frontlines;
};

TaskResult<CTF2Bot> CTF2BotMvMIdleTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	FindIdlePosition();

	if (entprops->GameRules_GetRoundState() == RoundState_RoundRunning)
	{
		if (bot->GetCurrency() >= BUY_UPGRADE_DURING_WAVE_MIN_CURRENCY)
		{
			bot->GetUpgradeManager().OnWaveEnd(); // force the manager to think it's ok to upgrade
			m_upgradeDuringWave = true; // TO-DO: Check if we don't need to rush to defend the bomb
		}
	}

	m_readyCheck.Start(1.0f);

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMvMIdleTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (entprops->GameRules_GetRoundState() == RoundState_RoundRunning)
	{
		if (m_upgradeDuringWave)
		{
			// Should the bot buy upgrades? Always buy upgrades if allowed
			if (!bot->GetUpgradeManager().IsManagerReady() || bot->GetUpgradeManager().ShouldGoToAnUpgradeStation())
			{
				// Buy upgrades
				return PauseFor(new CTF2BotMvMUpgradeTask, "Going to use an upgrade station!");
			}
		}

		// Wave has started!
		return Done("Wave started."); // unpause the main mvm task
	}

	// Wave has not started yet
	if (entprops->GameRules_GetRoundState() == RoundState_BetweenRounds)
	{
		// Should the bot buy upgrades? Always buy upgrades if allowed
		if (!bot->GetUpgradeManager().IsManagerReady() || bot->GetUpgradeManager().ShouldGoToAnUpgradeStation())
		{
			// Buy upgrades
			return PauseFor(new CTF2BotMvMUpgradeTask, "Going to use an upgrade station!");
		}
		else if (bot->GetMyClassType() == TeamFortress2::TFClass_Engineer || bot->GetMyClassType() == TeamFortress2::TFClass_Medic)
		{
			return Done("Done upgrading, returning to class behavior!");
		}

		if (m_readyCheck.IsElapsed())
		{
			// Misc: randomize a bit how frequently bots will toggle ready to make it a bit more human
			m_readyCheck.StartRandom(2.0f, 5.0f);

			if (bot->GetBehaviorInterface()->IsReady(bot) == ANSWER_YES)
			{
				if (!bot->TournamentIsReady() && tf2lib::MVM_ShouldBotsReadyUp())
				{
					bot->ToggleTournamentReadyStatus(true);
				}
			}
		}
	}

	if (bot->GetRangeTo(m_goal) > 72.0f)
	{
		if (m_repathtimer.IsElapsed())
		{
			m_repathtimer.Start(1.0f);
			CTF2BotPathCost cost(bot);
			m_nav.ComputePathToPosition(bot, m_goal, cost);
		}

		m_nav.Update(bot);
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMvMIdleTask::OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	m_nav.Invalidate();
	m_repathtimer.Invalidate();

	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotMvMIdleTask::OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	return TryContinue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotMvMIdleTask::OnMoveToSuccess(CTF2Bot* bot, CPath* path)
{
	return TryContinue();
}

QueryAnswerType CTF2BotMvMIdleTask::IsReady(CBaseBot* me)
{
	CTF2Bot* tf2bot = static_cast<CTF2Bot*>(me);
	auto& manager = tf2bot->GetUpgradeManager();

	if (!manager.IsManagerReady() || manager.ShouldGoToAnUpgradeStation())
	{
		return ANSWER_NO;
	}

	if (tf2bot->GetRangeTo(m_goal) > 128.0f)
	{
		return ANSWER_NO;
	}

	return ANSWER_YES;
}

void CTF2BotMvMIdleTask::FindIdlePosition()
{
	CTF2BotMvMIdleSelectRandomFrontlineArea frontlineAreas;

	CNavMesh::ForAllAreas(frontlineAreas);

	CTFNavArea* goalArea = frontlineAreas.GetRandomFrontlineArea();

	if (goalArea == nullptr)
	{
		m_goal = CTeamFortress2Mod::GetTF2Mod()->GetMvMBombHatchPosition();
	}
	else
	{
		m_goal = goalArea->GetCenter();
	}
}
