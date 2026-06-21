#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/pawnutils.h>
#include <navmesh/nav_mesh.h>
#include "navareavector.h"
#include "navblocker.h"
#include "navmesh.h"

static cell_t Native_IsNavMeshLoaded(IPluginContext* context, const cell_t* params)
{
	return TheNavMesh->IsLoaded() ? 1 : 0;
}

static cell_t Native_NavMeshGetNearestNavArea(IPluginContext* context, const cell_t* params)
{
	std::size_t firstparam = pawnutils::GetIndexOfParam(context, 1);
	cell_t* arr = nullptr;
	context->LocalToPhysAddr(params[firstparam], &arr);
	Vector pos = pawnutils::PawnFloatArrayToVector(arr);
	float maxdist = sp_ctof(params[firstparam + 1]);
	bool checkLos = params[firstparam + 2] != 0;
	bool checkGround = params[firstparam + 3] != 0;
	int team = static_cast<int>(params[firstparam + 4]);

	if (maxdist < 1.0f)
	{
		context->ReportError("Invalid max distance %g!", maxdist);
		return 0;
	}

	CNavArea* area = TheNavMesh->GetNearestNavArea(pos, maxdist, checkLos, checkGround, team);
	return pawnutils::ReturnPointerToPawn(context, params, area);
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
	ke::SafeSprintf(steamid, static_cast<size_t>(params[4]), "%" PRIu64 "", info.GetCreator().second);

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
	ke::SafeSprintf(steamid, static_cast<size_t>(params[5]), "%" PRIu64 "", editors[index].second);

	return 0;
}

static cell_t Native_NavMeshGetNavAreaByID(IPluginContext* context, const cell_t* params)
{
	unsigned int id = static_cast<unsigned int>(params[pawnutils::GetIndexOfParam(context, 1)]);
	CNavArea* area = TheNavMesh->GetNavAreaByID(id);
	return pawnutils::ReturnPointerToPawn(context, params, area);
}

static cell_t Native_NavMeshGetLastLoadResult(IPluginContext* context, const cell_t* params)
{
	return static_cast<cell_t>(TheNavMesh->GetLastLoadAttemptResult());
}

static void FrameAction_NavMeshGenerate(void* data)
{
	TheNavMesh->BeginGeneration(false);
}

static cell_t Native_NavMeshGenerate(IPluginContext* context, const cell_t* params)
{
	if (!TheNavMesh->IsLoaded())
	{
		// Avoid potential issues with SlowScriptTimeout
		smutils->AddFrameAction(FrameAction_NavMeshGenerate, nullptr);
	}

	return 0;
}

static cell_t Native_NavMeshLoad(IPluginContext* context, const cell_t* params)
{
	if (!TheNavMesh->IsLoaded())
	{
		TheNavMesh->DoLoad();
	}

	return 0;
}

static cell_t Native_NavMeshGetFullPathToNavMeshFile(IPluginContext* context, const cell_t* params)
{
	auto path = TheNavMesh->GetFullPathToNavMeshFile(pawnutils::ReadBool(params, 1));
	auto str = path.string();
	context->StringToLocal(params[2], static_cast<size_t>(params[3]), str.c_str());
	return 0;
}

static cell_t Native_NavMeshCollectAreasOverlappingExtent(IPluginContext* context, const cell_t* params)
{
	Extent extent;
	extent.lo = pawnutils::ReadVector(context, params, 1);
	extent.hi = pawnutils::ReadVector(context, params, 2);
	
	std::vector<CNavArea*>* vec = nullptr;
	auto handle = natives::navmesh::navareavector::createhandle(context, &vec);

	if (handle == BAD_HANDLE)
	{
		return BAD_HANDLE;
	}

	TheNavMesh->CollectAreasOverlappingExtent(extent, *vec);
	return handle;
}

static cell_t Native_NavMeshCollectAreasOverlappingExtentEntity(IPluginContext* context, const cell_t* params)
{
	CBaseEntity* pEntity = pawnutils::ReadEntity(context, params, 1);

	if (!pEntity)
	{
		return BAD_HANDLE;
	}

	Extent extent(pEntity);

	float xy_offset = pawnutils::ReadFloat(params, 2);
	float z_offset = pawnutils::ReadFloat(params, 3);

	extent.lo.x -= xy_offset;
	extent.lo.y -= xy_offset;
	extent.lo.z -= z_offset;
	extent.hi.x += xy_offset;
	extent.hi.y += xy_offset;
	extent.hi.z += z_offset;

	std::vector<CNavArea*>* vec = nullptr;
	auto handle = natives::navmesh::navareavector::createhandle(context, &vec);

	if (handle == BAD_HANDLE)
	{
		return BAD_HANDLE;
	}

	TheNavMesh->CollectAreasOverlappingExtent(extent, *vec);
	return handle;
}

static cell_t Native_NavMeshCollectAreasTouchingEntity(IPluginContext* context, const cell_t* params)
{
	CBaseEntity* pEntity = pawnutils::ReadEntity(context, params, 1);

	if (!pEntity)
	{
		return BAD_HANDLE;
	}

	std::vector<CNavArea*>* vec = nullptr;
	auto handle = natives::navmesh::navareavector::createhandle(context, &vec);

	if (handle == BAD_HANDLE)
	{
		return BAD_HANDLE;
	}

	TheNavMesh->CollectAreasTouchingEntity(pEntity, *vec);
	return handle;
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
		{"NavBotNavMesh.GetLastLoadResult", Native_NavMeshGetLastLoadResult},
		{"NavBotNavMesh.Generate", Native_NavMeshGenerate},
		{"NavBotNavMesh.Load", Native_NavMeshLoad},
		{"NavBotNavMesh.GetFullPathToNavMeshFile", Native_NavMeshGetFullPathToNavMeshFile},
		{"NavBotNavMesh.CollectAreasOverlappingExtent", Native_NavMeshCollectAreasOverlappingExtent},
		{"NavBotNavMesh.CollectAreasOverlappingExtentEx", Native_NavMeshCollectAreasOverlappingExtentEntity},
		{"NavBotNavMesh.CollectAreasTouchingEntity", Native_NavMeshCollectAreasTouchingEntity},
	};

	nv.insert(nv.end(), std::begin(list), std::end(list));

	natives::navmesh::navareavector::setup(nv);
	natives::navmesh::navblocker::setup(nv);
}