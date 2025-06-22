#include <extension.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <mods/tf2/nav/tfnav_waypoint.h>
#include <navmesh/nav_pathfind.h>
#include <mods/tf2/teamfortress2mod.h>
#include <entities/tf2/tf_entities.h>
#include <mods/tf2/tf2lib.h>
#include "tf2bot_task_sniper_snipe_area.h"
#include "tf2bot_task_sniper_move_to_sniper_spot.h"

class TF2SniperSpotAreaCollector : public INavAreaCollector<CTFNavArea>
{
public:
	TF2SniperSpotAreaCollector(CTF2Bot* me, CTFNavArea* startArea) :
		INavAreaCollector<CTFNavArea>(startArea, 8000.0f)
	{
		m_me = me;
	}

	bool ShouldSearch(CTFNavArea* area) override;
	bool ShouldCollect(CTFNavArea* area) override;
private:
	CTF2Bot* m_me;
};

bool TF2SniperSpotAreaCollector::ShouldSearch(CTFNavArea* area)
{
	if (area->IsBlocked(static_cast<int>(m_me->GetMyTFTeam())))
	{
		return false;
	}

	return true;
}

bool TF2SniperSpotAreaCollector::ShouldCollect(CTFNavArea* area)
{
	if (!m_me->GetMovementInterface()->IsAreaTraversable(area))
	{
		return false;
	}

	if (area->GetSizeX() < 16.0f || area->GetSizeY() < 16.0f)
	{
		// don't snipe in small areas
		return false;
	}

	if (area->HasTFPathAttributes(CTFNavArea::TFNavPathAttributes::TFNAV_PATH_DYNAMIC_SPAWNROOM))
	{
		// don't snipe inside spawnrooms
		return false;
	}

	if (area->IsBlocked(m_me->GetCurrentTeamIndex()))
	{
		// don't snipe on blocked areas
		return false;
	}

	return true;
}

TaskResult<CTF2Bot> CTF2BotSniperMoveToSnipingSpotTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	FindSniperSpot(bot);

	CTF2BotPathCost cost(bot, SAFEST_ROUTE);
	m_nav.ComputePathToPosition(bot, m_goal, cost);

	m_repathTimer.StartRandom();
	m_sniping = false;

	if (bot->IsUsingSniperScope())
	{
		bot->GetControlInterface()->PressSecondaryAttackButton();
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotSniperMoveToSnipingSpotTask::OnTaskUpdate(CTF2Bot* bot)
{
	auto mod = CTeamFortress2Mod::GetTF2Mod();

	// non defenders bots don't move during setup
	if (mod->IsInSetup() && mod->GetTeamRole(bot->GetMyTFTeam()) != TeamFortress2::TEAM_ROLE_DEFENDERS)
	{
		return Continue(); 
	}

	if (bot->GetRangeTo(m_goal) <= 16.0f)
	{
		// reached sniping goal
		m_sniping = true;
		bot->GetMovementInterface()->DoCounterStrafe();
		return SwitchTo(new CTF2BotSniperSnipeAreaTask(m_waypoint), "Sniping!");
	}

	if (m_repathTimer.IsElapsed() || !m_nav.IsValid())
	{
		m_repathTimer.StartRandom();

		CTF2BotPathCost cost(bot, SAFEST_ROUTE);
		m_nav.ComputePathToPosition(bot, m_goal, cost);
	}

	m_nav.Update(bot);

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotSniperMoveToSnipingSpotTask::OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	if (m_sniping)
	{
		FindSniperSpot(bot);
		m_sniping = false;
	}

	return Continue();
}

void CTF2BotSniperMoveToSnipingSpotTask::FindSniperSpot(CTF2Bot* bot)
{
	// TO-DO: Add game mode sniping spots

	GetRandomSnipingSpot(bot);
}

void CTF2BotSniperMoveToSnipingSpotTask::GetRandomSnipingSpot(CTF2Bot* bot)
{
	std::vector<CTFWaypoint*> spots;
	auto& thewaypoints = CTeamFortress2Mod::GetTF2Mod()->GetAllSniperWaypoints();

	for (auto waypoint : thewaypoints)
	{
		if (waypoint->IsEnabled() && waypoint->IsAvailableToTeam(static_cast<int>(bot->GetMyTFTeam())) && waypoint->CanBeUsedByBot(bot))
		{
			spots.push_back(waypoint);
		}
	}

	if (!spots.empty())
	{
		FilterWaypoints(bot, spots);
	}

	if (spots.empty())
	{
		m_goal = GetRandomSnipingPosition(bot);
		m_waypoint = nullptr;
		return;
	}

	CTFWaypoint* goal = librandom::utils::GetRandomElementFromVector<CTFWaypoint*>(spots);
	goal->Use(bot); // prevent other bots from selecting
	m_waypoint = goal;
	m_goal = goal->GetRandomPoint();
}

void CTF2BotSniperMoveToSnipingSpotTask::FilterWaypoints(CTF2Bot* bot, std::vector<CTFWaypoint*>& waypoints)
{
	if (CTeamFortress2Mod::GetTF2Mod()->GetCurrentGameMode() != TeamFortress2::GameModeType::GM_MVM)
	{
		return; // only mvm needs filtering for now
	}

	CBaseEntity* pEntity = tf2lib::mvm::GetMostDangerousFlag();
	Vector pos = bot->GetAbsOrigin();

	if (pEntity != nullptr)
	{
		tfentities::HCaptureFlag flag(pEntity);
		pos = flag.GetPosition();
	}

	waypoints.erase(std::remove_if(waypoints.begin(), waypoints.end(), [&pos](const CTFWaypoint* waypoint) {
		if ((waypoint->GetOrigin() - pos).Length() > 1500.0f)
		{
			return true;
		}

		return false;
	}), waypoints.end());
}

Vector CTF2BotSniperMoveToSnipingSpotTask::GetRandomSnipingPosition(CTF2Bot* bot)
{
	Vector startPos = GetSnipingSearchStartPosition(bot);
	CNavArea* start = TheNavMesh->GetNearestNavArea(startPos, 256.0f);

	if (start == nullptr)
	{
		start = bot->GetLastKnownNavArea();

		if (start == nullptr)
		{
			return bot->GetAbsOrigin();
		}
	}

	TF2SniperSpotAreaCollector collector(bot, static_cast<CTFNavArea*>(start));
	collector.Execute();

	if (collector.IsCollectedAreasEmpty())
	{
		return bot->GetAbsOrigin();
	}

	CTFNavArea* randomArea = collector.GetRandomCollectedArea();
	return randomArea->GetRandomPoint();
}

Vector CTF2BotSniperMoveToSnipingSpotTask::GetSnipingSearchStartPosition(CTF2Bot* bot)
{
	switch (CTeamFortress2Mod::GetTF2Mod()->GetCurrentGameMode())
	{
	case TeamFortress2::GameModeType::GM_MVM:
	{
		CBaseEntity* flag = tf2lib::mvm::GetMostDangerousFlag(false);

		if (flag)
		{
			tfentities::HCaptureFlag cf(flag);
			Vector pos = cf.GetPosition();
			CNavArea* area = TheNavMesh->GetNearestNavArea(pos, 256.0f);

			if (area)
			{
				return area->GetCenter();
			}
		}

		break;
	}
	case TeamFortress2::GameModeType::GM_ADCP:
	{
		std::vector<CBaseEntity*> controlpoints;
		controlpoints.reserve(8);
		CTeamFortress2Mod::GetTF2Mod()->CollectControlPointsToAttack(bot->GetMyTFTeam(), controlpoints);
		CTeamFortress2Mod::GetTF2Mod()->CollectControlPointsToDefend(bot->GetMyTFTeam(), controlpoints);

		if (!controlpoints.empty())
		{
			CBaseEntity* point = librandom::utils::GetRandomElementFromVector<CBaseEntity*>(controlpoints);
			Vector pos = UtilHelpers::getWorldSpaceCenter(point);
			pos = trace::getground(pos);
			return pos;
		}

		break;
	}
	case TeamFortress2::GameModeType::GM_CP:
		[[fallthrough]];
	case TeamFortress2::GameModeType::GM_ARENA:
		[[fallthrough]];
	case TeamFortress2::GameModeType::GM_KOTH:
		[[fallthrough]];
	case TeamFortress2::GameModeType::GM_TC:
	{
		std::vector<CBaseEntity*> controlpoints;
		controlpoints.reserve(8);

		if (bot->GetDifficultyProfile()->GetAggressiveness() >= 25)
		{
			CTeamFortress2Mod::GetTF2Mod()->CollectControlPointsToAttack(bot->GetMyTFTeam(), controlpoints);
		}
		
		CTeamFortress2Mod::GetTF2Mod()->CollectControlPointsToDefend(bot->GetMyTFTeam(), controlpoints);

		if (!controlpoints.empty())
		{
			CBaseEntity* point = librandom::utils::GetRandomElementFromVector<CBaseEntity*>(controlpoints);
			Vector pos = UtilHelpers::getWorldSpaceCenter(point);
			pos = trace::getground(pos);
			return pos;
		}

		break;
	}
	case TeamFortress2::GameModeType::GM_PL:
	{
		CBaseEntity* payload = CTeamFortress2Mod::GetTF2Mod()->GetBLUPayload();

		if (!payload)
		{
			// red attacking?
			payload = CTeamFortress2Mod::GetTF2Mod()->GetREDPayload();
		}

		if (payload)
		{
			return UtilHelpers::getWorldSpaceCenter(payload);
		}

		break;
	}
	case TeamFortress2::GameModeType::GM_PL_RACE:
	{
		CBaseEntity* payload = nullptr;

		if (randomgen->GetRandomInt<int>(0, 1) == 1)
		{
			payload = CTeamFortress2Mod::GetTF2Mod()->GetBLUPayload();
		}
		else
		{
			payload = CTeamFortress2Mod::GetTF2Mod()->GetREDPayload();
		}

		return UtilHelpers::getWorldSpaceCenter(payload);

		break;
	}
	case TeamFortress2::GameModeType::GM_CTF:
	{
		// Random chance to go snipe near the enemy flag
		if (bot->GetDifficultyProfile()->GetAggressiveness() >= 50 && randomgen->GetRandomInt<int>(1, 3) == 3)
		{
			edict_t* entity = bot->GetFlagToFetch();

			if (entity)
			{
				tfentities::HCaptureFlag flag(entity);
				return flag.GetPosition();
			}
		}

		edict_t* ent = bot->GetFlagToDefend(false, false);

		if (ent)
		{
			tfentities::HCaptureFlag flag(ent);
			return flag.GetPosition();
		}

		ent = bot->GetFlagCaptureZoreToDeliver();

		if (ent)
		{
			return UtilHelpers::getWorldSpaceCenter(ent);
		}

		break;
	}
	case TeamFortress2::GameModeType::GM_SD:
	{
		if (randomgen->GetRandomInt<int>(1, 3) == 3)
		{
			CBaseEntity* zone = tf2lib::sd::GetSpecialDeliveryCaptureZone();

			if (zone)
			{
				Vector pos = UtilHelpers::getWorldSpaceCenter(zone);
				pos = trace::getground(pos);
				return pos;
			}
		}
		else
		{
			CBaseEntity* flag = tf2lib::sd::GetSpecialDeliveryFlag();

			if (flag)
			{
				tfentities::HCaptureFlag flagent(flag);
				return flagent.GetPosition();
			}
		}

		break;
	}
	default:
		break;
	}



	return bot->GetAbsOrigin();
}
