#include <extension.h>
#include <manager.h>
#include <bot/basebot.h>
#include <bot/interfaces/behavior.h>
#include <bot/interfaces/path/basepath.h>
#include <entities/baseentity.h>
#include <navmesh/nav.h>
#include <navmesh/nav_area.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_pathfind.h>
#include <sdkports/debugoverlay_shared.h>
#include <util/helpers.h>
#include <util/librandom.h>
#include <util/UtilTrace.h>

#ifdef EXT_DEBUG
CON_COMMAND(sm_navbot_debug_vectors, "[LISTEN SERVER] Debug player vectors")
{
	auto edict = gamehelpers->EdictOfIndex(1);

	if (edict == nullptr)
	{
		return;
	}

	// CBaseExtPlayer can be used for both players and bots
	CBaseExtPlayer player(edict);

	static Vector mins(-4.0f, -4.0f, -4.0f);
	static Vector maxs(4.0f, 4.0f, 4.0f);

	auto origin = player.GetAbsOrigin();
	auto angles = player.GetAbsAngles();
	auto eyepos = player.GetEyeOrigin();
	auto eyeangles = player.GetEyeAngles();

	rootconsole->ConsolePrint("Origin: <%3.2f> <%3.2f> <%3.2f>", origin.x, origin.y, origin.z);
	rootconsole->ConsolePrint("Angles: <%3.2f> <%3.2f> <%3.2f>", angles.x, angles.y, angles.z);
	rootconsole->ConsolePrint("Eye Position: <%3.2f> <%3.2f> <%3.2f>", eyepos.x, eyepos.y, eyepos.z);
	rootconsole->ConsolePrint("Eye Angles: <%3.2f> <%3.2f> <%3.2f>", eyeangles.x, eyeangles.y, eyeangles.z);

	debugoverlay->AddBoxOverlay(origin, mins, maxs, angles, 0, 255, 0, 200, 15.0f);
	debugoverlay->AddBoxOverlay(eyepos, mins, maxs, eyeangles, 0, 0, 255, 200, 15.0f);

	static Vector forward;

	AngleVectors(eyeangles, &forward);

	forward = forward * 300.0f;

	debugoverlay->AddLineOverlay(eyepos, eyepos + forward, 255, 0, 0, true, 15.0f);
}

CON_COMMAND_F(sm_navbot_debug_do_not_use, "Do not even think about executing this command.", FCVAR_CHEAT)
{
	throw std::runtime_error("Exception test!");
}

CON_COMMAND(sm_navbot_debug_event_propagation, "Event propagation test")
{
	auto& bots = extmanager->GetAllBots();

	for (auto& botptr : bots)
	{
		auto bot = botptr.get();
		bot->OnTestEventPropagation();
		bot->GetBehaviorInterface()->ShouldFreeRoam(bot);
	}
}

CON_COMMAND(sm_navbot_debug_pathfind, "Path finding debug.")
{
	static Vector start;
	static Vector end;
	static bool reset = false;

	auto edict = gamehelpers->EdictOfIndex(1);

	if (edict == nullptr)
	{
		return;
	}

	// CBaseExtPlayer can be used for both players and bots
	CBaseExtPlayer player(edict);

	if (reset == false)
	{
		start = player.GetAbsOrigin();
		reset = true;
		NDebugOverlay::SweptBox(start, start, player.GetMins(), player.GetMaxs(), vec3_angle, 0, 220, 255, 255, 15.0f);
		rootconsole->ConsolePrint("Start position set to <%3.2f, %3.2f, %3.2f>", start.x, start.y, start.z);
		return;
	}

	reset = false;
	end = player.GetAbsOrigin();

	CNavArea* startArea = TheNavMesh->GetNearestNavArea(start, 256.0f, true, true);

	if (startArea == nullptr)
	{
		rootconsole->ConsolePrint("Path find: Failed to get starting area!");
		return;
	}

	CNavArea* goalArea = TheNavMesh->GetNearestNavArea(end, 256.0f, true, true);

	ShortestPathCost cost;
	CNavArea* closest = nullptr;
	bool path = NavAreaBuildPath(startArea, goalArea, &end, cost, &closest, 10000.0f, -2);

	CNavArea* from = nullptr;
	CNavArea* to = nullptr;

	for (CNavArea* area = closest; area != nullptr; area = area->GetParent())
	{
		if (from == nullptr) // set starting area;
		{
			from = area;
			continue;
		}

		to = area;

		NDebugOverlay::HorzArrow(from->GetCenter(), to->GetCenter(), 4.0f, 0, 255, 0, 255, true, 40.0f);

		rootconsole->ConsolePrint("Path: from #%i to #%i. gap: %3.2f", from->GetID(), to->GetID(), from->ComputeAdjacentConnectionGapDistance(to));

		from = to;
	}

	rootconsole->ConsolePrint("Path found!");
}

CON_COMMAND(sm_navbot_debug_botpath, "Debug bot path")
{
	CBaseBot* bot = nullptr;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		bot = extmanager->GetBotByIndex(i);

		if (bot != nullptr)
		{
			break;
		}
	}

	if (bot == nullptr)
	{
		rootconsole->ConsolePrint("Add a bot to the game first!");
		return;
	}

	auto edict = gamehelpers->EdictOfIndex(1);

	if (edict == nullptr)
	{
		return;
	}

	// CBaseExtPlayer can be used for both players and bots
	CBaseExtPlayer player(edict);
	ShortestPathCost cost;
	CPath path;
	Vector start = player.GetAbsOrigin();

	path.ComputePathToPosition(bot, start, cost, 0.0f, true);
	path.DrawFullPath(30.0f);

	rootconsole->ConsolePrint("Showing path from bot position to your position.");
}

CON_COMMAND(sm_navbot_debug_rng, "Debug random number generator")
{
	for (int i = 0; i < 15; i++)
	{
		int n = librandom::generate_random_int(0, 100);
		rootconsole->ConsolePrint("Random Number: %i", n);
	}
}

CON_COMMAND_F(free_nullptr_crash_for_everyone, "Crashes your game LMAO", FCVAR_CHEAT)
{
	auto edict = gamehelpers->EdictOfIndex(2040);
	auto& origin = edict->GetCollideable()->GetCollisionOrigin();
	Msg("%3.2f %3.2f %3.2f \n", origin.x, origin.y, origin.z);
}

CON_COMMAND_F(sm_navbot_debug_worldcenter, "Debugs the extension CBaseEntity::WorldSpaceCenter implementation.", FCVAR_GAMEDLL)
{
	if (args.ArgC() < 2)
	{
		Msg("Usage: sm_navbot_debug_worldcenter <classname> \n");
		return;
	}

	auto classname = args[1];
	int index = UtilHelpers::FindEntityByClassname(-1, classname);
	CBaseEntity* entity = nullptr;
	edict_t* edict = nullptr;

	if (!UtilHelpers::IndexToAThings(index, &entity, &edict))
	{
		Warning("Failed to obtain an entity pointer! \n");
		return;
	}

	entities::HBaseEntity baseent(entity);
	Vector method1 = UtilHelpers::getWorldSpaceCenter(edict);
	Vector method2 = baseent.WorldSpaceCenter();

	Msg("Method 1 (OLD): %3.4f %3.4f %3.4f \n", method1.x, method1.y, method1.z);
	Msg("Method 2 (NEW): %3.4f %3.4f %3.4f \n", method2.x, method2.y, method2.z);
}

CON_COMMAND_F(sm_debug_cbasehandles, "Debug CBaseHandle", FCVAR_CHEAT)
{
	CBaseEntity* baseentity = nullptr;
	CBaseHandle handle;
	handle.Set(nullptr);

	if (UtilHelpers::IndexToAThings(1, &baseentity, nullptr))
	{
		IServerEntity* serverent = reinterpret_cast<IServerEntity*>(baseentity);
		handle.Set(serverent);
		
		Msg("Handle Entity: %i(%i) -- %p \n", handle.GetEntryIndex(), handle.GetSerialNumber(), serverent);

		auto out = handle.Get();
		IServerEntity* ent2 = reinterpret_cast<IServerEntity*>(out);
		CBaseEntity* b2 = reinterpret_cast<CBaseEntity*>(out);
		auto edict = ent2->GetNetworkable()->GetEdict();
		Msg("Base Entity: %i \n", gamehelpers->EntityToBCompatRef(b2));
		Msg("Stored Entity: %p, %i \n", edict, gamehelpers->IndexOfEdict(edict));
		edict_t* ed1 = UtilHelpers::GetEdictFromCBaseHandle(handle);
		CBaseEntity* b3 = UtilHelpers::GetBaseEntityFromCBaseHandle(handle);
		Msg("Edict: %i \n", gamehelpers->IndexOfEdict(ed1));
		Msg("Entity: %i \n", gamehelpers->EntityToBCompatRef(b3));
		return;
	}

	Msg("Failed to get CBaseEntity! \n");
}

CON_COMMAND_F(sm_navbot_debug_traces1, "Trace debugging command #1", FCVAR_CHEAT)
{
	auto host = gamehelpers->EdictOfIndex(1);
	entities::HBaseEntity entity(host);
	CBaseExtPlayer player(host);

	Vector mins = entity.WorldAlignMins();
	Vector maxs = entity.WorldAlignMaxs();
	Vector origin = entity.GetAbsOrigin();
	Vector forward;
	auto angles = player.GetEyeAngles();
	player.EyeVectors(&forward);
	forward.NormalizeInPlace();

	CTraceFilterNoNPCsOrPlayer filter(host->GetIServerEntity(), COLLISION_GROUP_PLAYER);
	trace_t result;

	constexpr auto inc = 24.0f;
	float length = inc;
	Vector startpos = origin;
	Vector endpos = origin + (forward * length);
	bool loop = true;
	while (loop)
	{
		UTIL_TraceHull(startpos, endpos, mins, maxs, MASK_PLAYERSOLID, filter, &result);

		if (result.DidHit())
		{
			NDebugOverlay::SweptBox(startpos, endpos, mins, maxs, angles, 255, 0, 0, 255, 10.0f);
			loop = false;

			int entity = result.GetEntityIndex();
			Msg("Hit entity %i \n", entity);

			CBaseEntity* be = nullptr;
			if (UtilHelpers::IndexToAThings(entity, &be, nullptr))
			{
				Msg("    %s  \n", gamehelpers->GetEntityClassname(be));
			}
		}
		else
		{
			NDebugOverlay::SweptBox(startpos, endpos, mins, maxs, angles, 0, 255, 0, 255, 10.0f);
		}

		startpos = endpos;
		length += inc;
		endpos = origin + (forward * length);

		if (length >= 2048.0f) { loop = false; break; }
	}
}
#endif // EXT_DEBUG