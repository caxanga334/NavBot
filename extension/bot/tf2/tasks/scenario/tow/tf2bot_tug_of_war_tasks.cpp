#include NAVBOT_PCH_FILE
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/nav/tfnavmesh.h>
#include <entities/tf2/tf_entities.h>
#include <sdkports/sdk_takedamageinfo.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tf2bot.h>
#include <bot/interfaces/path/chasenavigator.h>
#include <bot/tasks_shared/bot_shared_roam.h>
#include "tf2bot_tug_of_war_tasks.h"

TaskResult<CTF2Bot> CTF2BotTugOfWarMonitorTask::OnTaskUpdate(CTF2Bot* bot)
{
	CTeamFortress2Mod* tf2mod = CTeamFortress2Mod::GetTF2Mod();
	TeamFortress2::TFTeam myteam = bot->GetMyTFTeam();
	std::vector<CBaseEntity*> points;

	tf2mod->CollectControlPointsToAttack(myteam, points);

	if (!points.empty())
	{
		return PauseFor(new CTF2BotTugOfWarAttackTask, "Attacking the tug!");
	}

	tf2mod->CollectControlPointsToDefend(myteam, points);

	if (!points.empty())
	{
		return PauseFor(new CTF2BotTugOfWarAttackTask, "Attacking the tug!");
	}

	return PauseFor(new CBotSharedRoamTask<CTF2Bot, CTF2BotPathCost>(bot, 4096.0f, true), "Nothing to do, roaming!");
}

CTF2BotTugOfWarAttackTask::CTF2BotTugOfWarAttackTask()
{
	m_goal = CTeamFortress2Mod::GetTF2Mod()->GetTugOfWarGoal();
}

TaskResult<CTF2Bot> CTF2BotTugOfWarAttackTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	if (m_goal.Get() == nullptr)
	{
		return Done("NULL goal entity!");
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotTugOfWarAttackTask::OnTaskUpdate(CTF2Bot* bot)
{
	CBaseEntity* pGoal = m_goal.Get();

	if (!pGoal)
	{
		return Done("NULL goal entity!");
	}

	if (m_nav.NeedsRepath() || !m_nav.IsValid())
	{
		Vector vGoal = UtilHelpers::getWorldSpaceCenter(pGoal);
		CTF2BotPathCost cost{ bot, FASTEST_ROUTE };
		
		m_nav.ComputePathToPosition(bot, vGoal, cost);
		m_nav.StartRepathTimer(1.0f); // faster repaths since the entity may be moving
	}

	m_nav.Update(bot);

	return Continue();
}
