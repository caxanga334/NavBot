#include "tfnavarea.h"
#include "tfnavmesh.h"

CNavArea* CTFNavMesh::CreateArea(void) const
{
	return new CTFNavArea(GetNavPlace());
}

void CTFNavMesh::Update()
{
	CNavMesh::Update();
}