#include <extension.h>
#include <util/librandom.h>
#include <entities/tf2/tf_entities.h>
#include <mods/tf2/teamfortress2mod.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <navmesh/nav_pathfind.h>
#include "tf2bot_engineer_build_object.h"
#include "tf2bot_engineer_repair_object.h"
#include "tf2bot_engineer_upgrade_object.h"
#include "tf2bot_engineer_nest.h"

static ConVar c_navbot_tf_engineer_tele_entrance_build_range("sm_navbot_tf_engineer_teleentrance_build_range", "3000.0", FCVAR_GAMEDLL, "Maximum distance the engineer will build a teleporter entrance.");

class EngineerBuildableLocationCollector : public INavAreaCollector<CTFNavArea>
{
public:
	EngineerBuildableLocationCollector(CTF2Bot* engineer, CTFNavArea* startArea) :
		INavAreaCollector<CTFNavArea>(startArea, 4096.0f)
	{
		m_me = engineer;
	}

	bool ShouldCollect(CTFNavArea* area) override;
private:
	CTF2Bot* m_me;
};

bool EngineerBuildableLocationCollector::ShouldCollect(CTFNavArea* area)
{
	if (!area->IsBuildable())
	{
		return false;
	}

	return true;
}

CTF2BotEngineerNestTask::CTF2BotEngineerNestTask()
{
	m_goal.Init(0.0f, 0.0f, 0.0f);
}

TaskResult<CTF2Bot> CTF2BotEngineerNestTask::OnTaskUpdate(CTF2Bot* bot)
{
	bot->FindMyBuildings();

	AITask<CTF2Bot>* nextTask = NestTask(bot);

	if (nextTask != nullptr)
	{
		return PauseFor(nextTask, "Taking care of my own nest!");
	}

	return Continue();
}

bool CTF2BotEngineerNestTask::OnTaskPause(CTF2Bot* bot, AITask<CTF2Bot>* nextTask)
{
	m_nav.Invalidate();
	return true;
}

TaskResult<CTF2Bot> CTF2BotEngineerNestTask::OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotEngineerNestTask::OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	return TryContinue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotEngineerNestTask::OnMoveToSuccess(CTF2Bot* bot, CPath* path)
{
	return TryContinue();
}

AITask<CTF2Bot>* CTF2BotEngineerNestTask::NestTask(CTF2Bot* me)
{
	if (me->GetMyTeleporterEntrance() == nullptr)
	{
		Vector goal;

		if (FindSpotToBuildTeleEntrance(me, goal))
		{
			return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_TELEPORTER_ENTRANCE, goal);
		}
	}
	else if (me->GetMySentryGun() == nullptr)
	{
		Vector goal;

		if (FindSpotToBuildSentryGun(me, goal))
		{
			return new CTF2BotEngineerBuildObjectTask(CTF2BotEngineerBuildObjectTask::OBJECT_SENTRYGUN, goal);
		}
	}

	if (me->GetMySentryGun() != nullptr)
	{
		tfentities::HBaseObject sentrygun(me->GetMySentryGun());

		if (!sentrygun.IsAtMaxLevel())
		{
			return new CTF2BotEngineerUpgradeObjectTask(gamehelpers->ReferenceToEntity(sentrygun.GetIndex()));
		}
	}

	return nullptr;
}

bool CTF2BotEngineerNestTask::FindSpotToBuildSentryGun(CTF2Bot* me, Vector& out)
{
	// TO-DO: Add game mode specific sentry spots
	return GetRandomSentrySpot(me, out);
}

bool CTF2BotEngineerNestTask::FindSpotToBuildTeleEntrance(CTF2Bot* me, Vector& out)
{
	me->UpdateLastKnownNavArea(true);
	CNavArea* start = me->GetLastKnownNavArea();

	if (start == nullptr)
	{
		return false;
	}

	EngineerBuildableLocationCollector collector(me, static_cast<CTFNavArea*>(start));

	collector.SetTravelLimit(c_navbot_tf_engineer_tele_entrance_build_range.GetFloat()); // default is 2048, reduce it for tele entrance
	collector.Execute();

	if (collector.IsCollectedAreasEmpty())
	{
		return false;
	}

	auto& areas = collector.GetCollectedAreas();

	CTFNavArea* buildGoal = nullptr;
	std::vector<CTFNavArea*> hintAreas;
	hintAreas.reserve(32);

	for (auto tfarea : areas)
	{
		if (tfarea->HasTFAttributes(CTFNavArea::TFNAV_TELE_ENTRANCE_HINT) && !tfarea->IsTFAttributesRestrictedForTeam(me->GetMyTFTeam()))
		{
			hintAreas.push_back(tfarea);
		}
	}

	if (hintAreas.empty())
	{
		// no hints were found, pick a random one
		buildGoal = areas[randomgen->GetRandomInt<size_t>(0, areas.size() - 1)];
	}
	else
	{
		buildGoal = hintAreas[randomgen->GetRandomInt<size_t>(0, hintAreas.size() - 1)];
	}

	out = buildGoal->GetRandomPoint();

	if (me->IsDebugging(BOTDEBUG_TASKS))
	{
		buildGoal->DrawFilled(0, 128, 0, 255, 5.0f, true);
	}

	return true;
}

bool CTF2BotEngineerNestTask::GetRandomSentrySpot(CTF2Bot* me, Vector& out)
{
	me->UpdateLastKnownNavArea(true);
	CNavArea* start = me->GetLastKnownNavArea();

	if (start == nullptr)
	{
		return false;
	}

	
	EngineerBuildableLocationCollector collector(me, static_cast<CTFNavArea*>(start));

	collector.SetTravelLimit(4096.0f);
	collector.Execute();

	if (collector.IsCollectedAreasEmpty())
	{
		return false;
	}

	auto& areas = collector.GetCollectedAreas();

	CTFNavArea* buildGoal = nullptr;
	std::vector<CTFNavArea*> hintAreas;
	hintAreas.reserve(32);

	for (auto tfarea : areas)
	{
		if (tfarea->HasTFAttributes(CTFNavArea::TFNAV_SENTRYGUN_HINT) && !tfarea->IsTFAttributesRestrictedForTeam(me->GetMyTFTeam()))
		{
			hintAreas.push_back(tfarea);
		}
	}

	if (hintAreas.empty())
	{
		// no hints were found, pick a random one
		buildGoal = areas[randomgen->GetRandomInt<size_t>(0, areas.size() - 1)];
	}
	else
	{
		buildGoal = hintAreas[randomgen->GetRandomInt<size_t>(0, hintAreas.size() - 1)];
	}

	out = buildGoal->GetRandomPoint();

	if (me->IsDebugging(BOTDEBUG_TASKS))
	{
		buildGoal->DrawFilled(0, 128, 0, 255, 5.0f, true);
	}

	return true;
}

