#include NAVBOT_PCH_FILE
#include "nav_mesh.h"
#include "nav_blocker.h"

void INavBlocker::Register()
{
	TheNavMesh->RegisterNavBlocker(this);
}

void INavBlocker::Unregister()
{
	TheNavMesh->UnregisterNavBlocker(this);
}

void INavBlocker::NotifyDestruction()
{
	INavBlocker::NotifyBlockerDestruction<CNavArea> functor{ this };
	CNavMesh::ForAllAreas<decltype(functor)>(functor);
}
