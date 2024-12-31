#include <chrono>

#include <extension.h>
#include <util/helpers.h>
#include <navmesh/nav_area.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_pathfind.h>
#include <sdkports/debugoverlay_shared.h>

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
