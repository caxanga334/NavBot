#include NAVBOT_PCH_FILE
#include "nav_pathcost_mod.h"
#include "nav_mesh.h"
#include "nav_area.h"

bool INavPathCostMod::NotifyPathCostModDestruction::operator()(CNavArea* area)
{
	area->UnregisterNavPathCostMod(this->pathcostmod);
	return true;
}

INavPathCostMod::RegisterPathCostModExtent::RegisterPathCostModExtent(const INavPathCostMod* pathcostmod, const Extent& extent)
{
	TheNavMesh->CollectAreasOverlappingExtent<CNavArea>(extent, areas);

	for (CNavArea* area : areas)
	{
		area->RegisterNavPathCostMod(pathcostmod);
	}
}

CNavPathCostMod::~CNavPathCostMod()
{
	INavPathCostMod::NotifyPathCostModDestruction functor{ this };
	CNavMesh::ForAllAreas(functor);
}
