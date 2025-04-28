#include <extension.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <entities/tf2/tf_entities.h>
#include <bot/tf2/tf2bot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <bot/tf2/tasks/tf2bot_roam.h>
#include "tf2bot_attack_controlpoint.h"
#include "tf2bot_defend_controlpoint.h"
#include "tf2bot_controlpoints_monitor.h"

TaskResult<CTF2Bot> CTF2BotControlPointMonitorTask::OnTaskUpdate(CTF2Bot* bot)
{
	auto tf2mod = CTeamFortress2Mod::GetTF2Mod();

	std::vector<CBaseEntity*> attackPoints;
	std::vector<CBaseEntity*> defendPoints;
	std::vector<CBaseEntity*> pointsUnderSiege;
	tf2mod->CollectControlPointsToAttack(bot->GetMyTFTeam(), attackPoints);
	tf2mod->CollectControlPointsToDefend(bot->GetMyTFTeam(), defendPoints);

	for (auto controlpoint : defendPoints)
	{
		tfentities::HTeamControlPoint tcp(controlpoint);
		float cap = tf2mod->GetTFObjectiveResource()->GetCapturePercentage(tcp.GetPointIndex());

		if (cap >= 0.01f)
		{
			pointsUnderSiege.push_back(controlpoint);
		}
	}

	// no attack or defend
	if (defendPoints.empty() && attackPoints.empty())
	{
		return PauseFor(new CTF2BotRoamTask, "No point to attack or defend, roaming around!");
	}
	else if (defendPoints.empty())
	{
		// no points to defend
		CBaseEntity* target = librandom::utils::GetRandomElementFromVector<CBaseEntity*>(attackPoints);
		return PauseFor(new CTF2BotAttackControlPointTask(target), "Attacking control point!");
	}
	else if (attackPoints.empty())
	{
		// no points to attack
		CBaseEntity* target = librandom::utils::GetRandomElementFromVector<CBaseEntity*>(defendPoints);
		return PauseFor(new CTF2BotGuardControlPointTask(target), "Guarding control point!");
	}
	else
	{
		// points to defend and attack

		
		if (!pointsUnderSiege.empty())
		{
			CBaseEntity* target = librandom::utils::GetRandomElementFromVector<CBaseEntity*>(pointsUnderSiege);
			return PauseFor(new CTF2BotDefendControlPointTask(target), "Defending control point being captured!");
		}

		// if no points are under attack randomly attack or guard
		if (randomgen->GetRandomInt<int>(1, 100) > tf2mod->GetModSettings()->GetDefendRate())
		{
			CBaseEntity* target = librandom::utils::GetRandomElementFromVector<CBaseEntity*>(attackPoints);
			return PauseFor(new CTF2BotAttackControlPointTask(target), "Attacking control point!");
		}
		else
		{
			CBaseEntity* target = librandom::utils::GetRandomElementFromVector<CBaseEntity*>(defendPoints);
			return PauseFor(new CTF2BotGuardControlPointTask(target), "Guarding control point!");
		}
	}

	return Continue();
}
