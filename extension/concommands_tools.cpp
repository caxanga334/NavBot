#include <chrono>

#include <extension.h>
#include <manager.h>
#include <extplayer.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <navmesh/nav_area.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_pathfind.h>
#include <sdkports/debugoverlay_shared.h>
#include <util/prediction.h>

CON_COMMAND_F(sm_navbot_tool_build_path, "Builds a path from your current position to the marked nav area. (Original Search Method)", FCVAR_CHEAT)
{
	if (engine->IsDedicatedServer())
	{
		META_CONPRINT("This command can only be used on a listen server! \n");
		return;
	}

	edict_t* host = UtilHelpers::GetListenServerHost();
	const Vector& origin = host->GetCollideable()->GetCollisionOrigin();

	CNavArea* start = TheNavMesh->GetNearestNavArea(origin, 256.0f, true, true);
	CNavArea* end = TheNavMesh->GetMarkedArea();

	if (start == nullptr)
	{
		META_CONPRINT("No Nav Area found near you! \n");
		return;
	}

	if (end == nullptr)
	{
		META_CONPRINT("No End Area. Mark the destination area with sm_nav_mark! \n");
		return;
	}

	ShortestPathCost cost;
	CNavArea* closest = nullptr;
	auto tstart = std::chrono::high_resolution_clock::now();
	bool found = NavAreaBuildPath(start, end, nullptr, cost, &closest);
	auto tend = std::chrono::high_resolution_clock::now();

	const std::chrono::duration<double, std::milli> millis = (tend - tstart);

	META_CONPRINTF("NavAreaBuildPath took %f ms.\n", millis.count());

	if (found)
	{
		META_CONPRINT("Found path! \n");
	}
	else
	{
		META_CONPRINT("No path found! \n");
	}

	CNavArea* from = nullptr;
	CNavArea* to = nullptr;

	for (CNavArea* area = closest; area != nullptr; area = area->GetParent())
	{
		if (from == nullptr) // set starting area;
		{
			from = area;
			area->DrawFilled(0, 128, 0, 64, 30.0f);
			continue;
		}

		to = area;

		to->DrawFilled(0, 128, 0, 64, 30.0f);

		from = to;
	}
}

CON_COMMAND_F(sm_navbot_tool_build_path_new, "Builds a path from your current position to the marked nav area. (New Search Method)", FCVAR_CHEAT)
{
	if (engine->IsDedicatedServer())
	{
		META_CONPRINT("This command can only be used on a listen server! \n");
		return;
	}

	edict_t* host = UtilHelpers::GetListenServerHost();
	const Vector& origin = host->GetCollideable()->GetCollisionOrigin();

	CNavArea* start = TheNavMesh->GetNearestNavArea(origin, 256.0f, true, true);
	CNavArea* end = TheNavMesh->GetMarkedArea();

	if (start == nullptr)
	{
		META_CONPRINT("No Nav Area found near you! \n");
		return;
	}

	if (end == nullptr)
	{
		META_CONPRINT("No End Area. Mark the destination area with sm_nav_mark! \n");
		return;
	}

	NavAStarPathCost cost;
	NavAStarHeuristicCost heuristic;
	INavAStarSearch<CNavArea> search;
	search.SetStart(start);
	search.SetGoalArea(end);

	auto tstart = std::chrono::high_resolution_clock::now();
	search.DoSearch(cost, heuristic);
	auto tend = std::chrono::high_resolution_clock::now();

	const std::chrono::duration<double, std::milli> millis = (tend - tstart);

	META_CONPRINTF("INavAStarSearch<CNavArea>::DoSearch() took %f ms.\n", millis.count());

	if (search.FoundPath())
	{
		META_CONPRINT("Found path! \n");
	}
	else
	{
		META_CONPRINT("No path found! \n");
	}

	for (auto area : search.GetPath())
	{
		area->DrawFilled(0, 128, 0, 64, 30.0f);
	}
}

CON_COMMAND_F(sm_navbot_tool_report_hull_sizes, "Prints the player's hull size to the console.", FCVAR_CHEAT)
{
	edict_t* host = UtilHelpers::GetListenServerHost();

	const Vector& mins = host->GetCollideable()->OBBMins();
	const Vector& maxs = host->GetCollideable()->OBBMaxs();

	META_CONPRINTF("Mins: %3.2f %3.2f %3.2f\n", mins.x, mins.y, mins.z);
	META_CONPRINTF("Maxs: %3.2f %3.2f %3.2f\n", maxs.x, maxs.y, maxs.z);
}

CON_COMMAND_F(sm_navbot_tool_projectile_aim, "Tests projectile aim parameters", FCVAR_CHEAT)
{
	if (engine->IsDedicatedServer())
	{
		Msg("This command can only be used on a Listen Server! \n");
		return;
	}

	if (args.ArgC() < 9)
	{
		Msg("[SM] Usage: sm_navbot_tool_projectile_aim <target x> <target y> <target z> <projectile speed> <projectile gravity> <ballistics start range> <ballistics end range> <ballistics min rate> <ballistics max rate>\n");
		return;
	}

	ConVar* sv_gravity = g_pCVar->FindVar("sv_gravity");

	if (!sv_gravity)
	{
		Warning("sm_navbot_tool_projectile_aim FindVar sv_gravity == NULL! \n");
	}

	CBaseExtPlayer host{ UtilHelpers::GetListenServerHost() };
	const float x = atof(args[1]);
	const float y = atof(args[2]);
	const float z = atof(args[3]);
	const Vector targetPos{ x,y,z };
	const float projSpeed = atof(args[4]);
	const float projGrav = atof(args[5]);
	const float startRange = atof(args[6]);
	const float endRange = atof(args[7]);
	const float startRate = atof(args[8]);
	const float endRate = atof(args[9]);
	const Vector eyePos = host.GetEyeOrigin();
	const Vector enemyVel{ 0.0f, 0.0f, 0.0f }; // always zero in this case
	float rangeTo = (targetPos - eyePos).Length();
	Vector aimPos = pred::SimpleProjectileLead(targetPos, enemyVel, projSpeed, rangeTo);
	rangeTo = (eyePos - aimPos).Length(); // update range for gravity compensation
	const float elevation_rate = RemapValClamped(rangeTo, startRange, endRange, startRate, endRate);
	const float elevation_Z = pred::GravityComp(rangeTo, projGrav, elevation_rate);
	aimPos.z += elevation_Z;

	Vector to = (aimPos - eyePos);
	to.NormalizeInPlace();
	QAngle viewAngles;
	VectorAngles(to, viewAngles);
	host.SnapEyeAngles(viewAngles);

	debugoverlay->AddLineOverlay(eyePos, targetPos, 255, 0, 0, true, 20.0f); // eye to target pos
	debugoverlay->AddLineOverlay(eyePos, aimPos, 0, 128, 0, true, 20.0f); // eye to calculated pos
	debugoverlay->AddLineOverlay(aimPos, targetPos, 0, 128, 255, true, 20.0f); // aim pos to target
}

CON_COMMAND_F(sm_navbot_tool_projectile_aim_entity, "Tests projectile aim parameters", FCVAR_CHEAT)
{
	if (engine->IsDedicatedServer())
	{
		Msg("This command can only be used on a Listen Server! \n");
		return;
	}

	if (args.ArgC() < 7)
	{
		Msg("[SM] Usage: sm_navbot_tool_projectile_aim <ent index> <projectile speed> <projectile gravity> <ballistics start range> <ballistics end range> <ballistics min rate> <ballistics max rate>\n");
		return;
	}

	ConVar* sv_gravity = g_pCVar->FindVar("sv_gravity");

	if (!sv_gravity)
	{
		Warning("sm_navbot_tool_projectile_aim FindVar sv_gravity == NULL! \n");
	}

	CBaseExtPlayer host{ UtilHelpers::GetListenServerHost() };
	const int entindex = atof(args[1]);
	CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(entindex);

	if (!pEntity)
	{
		Warning("No entity of index %i \n", entindex);
		return;
	}

	const Vector targetPos = UtilHelpers::getWorldSpaceCenter(pEntity);
	const float projSpeed = atof(args[2]);
	const float projGrav = atof(args[3]);
	const float startRange = atof(args[4]);
	const float endRange = atof(args[5]);
	const float startRate = atof(args[6]);
	const float endRate = atof(args[7]);
	const Vector eyePos = host.GetEyeOrigin();
	Vector enemyVel{ 0.0f, 0.0f, 0.0f };
	entityprops::GetEntityAbsVelocity(pEntity, enemyVel);
	float rangeTo = (targetPos - eyePos).Length();
	Vector aimPos = pred::SimpleProjectileLead(targetPos, enemyVel, projSpeed, rangeTo);
	rangeTo = (eyePos - aimPos).Length(); // update range for gravity compensation
	const float elevation_rate = RemapValClamped(rangeTo, startRange, endRange, startRate, endRate);
	const float elevation_Z = pred::GravityComp(rangeTo, projGrav, elevation_rate);
	aimPos.z += elevation_Z;

	Vector to = (aimPos - eyePos);
	to.NormalizeInPlace();
	QAngle viewAngles;
	VectorAngles(to, viewAngles);
	host.SnapEyeAngles(viewAngles);

	debugoverlay->AddLineOverlay(eyePos, targetPos, 255, 0, 0, true, 20.0f); // eye to target pos
	debugoverlay->AddLineOverlay(eyePos, aimPos, 0, 128, 0, true, 20.0f); // eye to calculated pos
	debugoverlay->AddLineOverlay(aimPos, targetPos, 0, 128, 255, true, 20.0f); // aim pos to target
}

CON_COMMAND_F(sm_navbot_tool_give_infammo, "Gives infinite reserve ammo to every player", FCVAR_CHEAT)
{
	if (engine->IsDedicatedServer())
	{
		return;
	}

	auto func = [](int client, edict_t* entity, SourceMod::IGamePlayer* player) {
		if (player->IsInGame())
		{
			for (int i = 0; i < MAX_AMMO_TYPES; i++)
			{
				entprops->SetEntProp(client, Prop_Send, "m_iAmmo", 9999, 4, i);
			}
		}
	};

	UtilHelpers::ForEachPlayer(func);
}

CON_COMMAND_F(sm_navbot_tool_bots_go_to, "Bots will move to your current position.", FCVAR_CHEAT)
{
	if (engine->IsDedicatedServer())
	{
		Msg("This command can only be used on a Listen Server! \n");
		return;
	}

	auto func = [](CBaseBot* bot) {
		bot->OnDebugMoveToHostCommand();
	};

	extmanager->ForEachBot(func);
}

CON_COMMAND_F(sm_navbot_tool_reset_bots, "Reset the bot interfaces.", FCVAR_CHEAT)
{
	if (engine->IsDedicatedServer())
	{
		Msg("This command can only be used on a Listen Server! \n");
		return;
	}

	auto func = [](CBaseBot* bot) {
		bot->Reset();
	};

	extmanager->ForEachBot(func);
}

CON_COMMAND_F(sm_navbot_tool_toggle_non_solid, "Makes your non solid to triggers.", FCVAR_CHEAT)
{
	edict_t* host = gamehelpers->EdictOfIndex(1);

	static int oldsolidtype = -1;

	if (oldsolidtype < 0)
	{
		entprops->GetEntProp(1, Prop_Data, "m_nSolidType", oldsolidtype);
		entprops->SetEntProp(1, Prop_Data, "m_nSolidType", static_cast<int>(SolidType_t::SOLID_NONE));
		META_CONPRINTF("Setting to SOLID_NONE! Old = %i \n", oldsolidtype);
	}
	else
	{
		entprops->SetEntProp(1, Prop_Data, "m_nSolidType", oldsolidtype);
		META_CONPRINTF("Restoring to old solid type %i \n", oldsolidtype);
		oldsolidtype = -1;
	}	
}
