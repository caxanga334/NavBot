#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/pawnutils.h>
#include <navmesh/nav_mesh.h>
#include "navmesh.h"

static cell_t Native_IsNavMeshLoaded(IPluginContext* context, const cell_t* params)
{
	return TheNavMesh->IsLoaded() ? 1 : 0;
}

static cell_t Native_NavMeshGetNearestNavArea(IPluginContext* context, const cell_t* params)
{
	cell_t* arr = nullptr;
	context->LocalToPhysAddr(params[1], &arr);
	Vector pos = pawnutils::PawnFloatArrayToVector(arr);
	float maxdist = sp_ctof(params[2]);
	bool checkLos = params[3] != 0;
	bool checkGround = params[4] != 0;
	int team = static_cast<int>(params[5]);

	if (maxdist < 1.0f)
	{
		context->ReportError("Invalid max distance %g!", maxdist);
		return 0;
	}

	CNavArea* area = TheNavMesh->GetNearestNavArea(pos, maxdist, checkLos, checkGround, team);

	if (!area)
	{
		return 0;
	}

	return pawnutils::PointerToPawnAddress(context, area);
}

static cell_t Native_NavMeshGetAuthor(IPluginContext* context, const cell_t* params)
{
	const CNavMesh::AuthorInfo& info = TheNavMesh->GetAuthorInfo();

	if (!info.HasCreatorBeenSet())
	{
		return 0;
	}

	char* name;
	char* steamid;

	context->LocalToString(params[1], &name);
	context->LocalToString(params[3], &steamid);

	ke::SafeStrcpy(name, static_cast<size_t>(params[2]), info.GetCreator().first.c_str());
	ke::SafeSprintf(steamid, static_cast<size_t>(params[4]), "%lu", info.GetCreator().second);

	return 1;
}

static cell_t Native_NavMeshGetEditorCount(IPluginContext* context, const cell_t* params)
{
	return static_cast<cell_t>(TheNavMesh->GetAuthorInfo().GetEditorCount());
}

static cell_t Native_NavMeshGetEditor(IPluginContext* context, const cell_t* params)
{
	const std::vector<CNavMesh::NavEditor>& editors = TheNavMesh->GetAuthorInfo().GetEditors();
	
	int index = static_cast<int>(params[1]);

	if (index < 0 || index >= static_cast<int>(editors.size()))
	{
		context->ReportError("Editor index %i is out of bounds! Range 0 - %zu", index, editors.size());
		return 0;
	}

	char* name;
	char* steamid;

	context->LocalToString(params[2], &name);
	context->LocalToString(params[4], &steamid);

	ke::SafeStrcpy(name, static_cast<size_t>(params[3]), editors[index].first.c_str());
	ke::SafeSprintf(steamid, static_cast<size_t>(params[5]), "%lu", editors[index].second);

	return 0;
}

static cell_t Native_NavMeshGetNavAreaByID(IPluginContext* context, const cell_t* params)
{
	unsigned int id = static_cast<unsigned int>(params[1]);
	CNavArea* area = TheNavMesh->GetNavAreaByID(id);

	if (!area)
	{
		return 0;
	}

	return pawnutils::PointerToPawnAddress(context, area);
}

static cell_t Native_NavAreaGetID(IPluginContext* context, const cell_t* params)
{
	CNavArea* area = pawnutils::PawnAddressToPointer<CNavArea>(context, params[1]);

	if (!area)
	{
		context->ReportError("NULL nav area of address %x", params[1]);
		return 0;
	}

	return static_cast<cell_t>(area->GetID());
}

static cell_t Native_NavAreaGetCenter(IPluginContext* context, const cell_t* params)
{
	CNavArea* area = pawnutils::PawnAddressToPointer<CNavArea>(context, params[1]);

	if (!area)
	{
		context->ReportError("NULL nav area of address %x", params[1]);
		return 0;
	}

	cell_t* arr = nullptr;
	context->LocalToPhysAddr(params[2], &arr);
	pawnutils::VectorToPawnFloatArray(arr, area->GetCenter());

	return 0;
}

void natives::navmesh::setup(std::vector<sp_nativeinfo_t>& nv)
{
	sp_nativeinfo_t list[] = {
		{"NavBotNavMesh.IsLoaded", Native_IsNavMeshLoaded},
		{"NavBotNavMesh.GetNearestNavArea", Native_NavMeshGetNearestNavArea},
		{"NavBotNavMesh.GetNavAreaByID", Native_NavMeshGetNavAreaByID},
		{"NavBotNavMesh.GetAuthor", Native_NavMeshGetAuthor},
		{"NavBotNavMesh.GetEditorCount", Native_NavMeshGetEditorCount},
		{"NavBotNavMesh.GetEditor", Native_NavMeshGetEditor},
		{"NavBotNavArea.GetID", Native_NavAreaGetID},
		{"NavBotNavArea.GetCenter", Native_NavAreaGetCenter},
	};

	nv.insert(nv.end(), std::begin(list), std::end(list));
}