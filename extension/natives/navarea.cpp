#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/pawnutils.h>
#include <navmesh/nav_mesh.h>
#include "navarea.h"

static cell_t Native_NavAreaGetID(IPluginContext* context, const cell_t* params)
{
	CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

	if (!area)
	{
		context->ReportError("NULL nav area of address %x", params[1]);
		return 0;
	}

	return static_cast<cell_t>(area->GetID());
}

static cell_t Native_NavAreaGetCenter(IPluginContext* context, const cell_t* params)
{
	CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

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

static cell_t Native_NavAreaGetCorner(IPluginContext* context, const cell_t* params)
{
	CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

	if (!area)
	{
		context->ReportError("NULL nav area of address %x", params[1]);
		return 0;
	}

	if (params[2] < 0 || params[2] >= static_cast<cell_t>(NavCornerType::NUM_CORNERS))
	{
		context->ReportError("Invalid corner type: %i", params[2]);
		return 0;
	}

	cell_t* arr = nullptr;
	context->LocalToPhysAddr(params[3], &arr);
	pawnutils::VectorToPawnFloatArray(arr, area->GetCorner(static_cast<NavCornerType>(params[2])));

	return 0;
}

static cell_t Native_NavAreaGetExtent(IPluginContext* context, const cell_t* params)
{
	CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

	if (!area)
	{
		context->ReportError("NULL nav area of address %x", params[1]);
		return 0;
	}

	Extent extent;
	area->GetExtent(&extent);
	pawnutils::WriteVector(context, params, 2, extent.lo);
	pawnutils::WriteVector(context, params, 3, extent.hi);
	return 0;
}

static cell_t Native_NavAreaIsVisible(IPluginContext* context, const cell_t* params)
{
	CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

	if (!area)
	{
		context->ReportError("NULL nav area of address %x", params[1]);
		return 0;
	}

	Vector eyes = pawnutils::ReadVector(context, params, 2);
	Vector visSpot(0.0f, 0.0f, 0.0f);
	bool result = area->IsVisible(eyes, &visSpot);
	
	if (!pawnutils::IsNULLVector(context, params, 3))
	{
		pawnutils::WriteVector(context, params, 3, visSpot);
	}

	return pawnutils::ReturnBool(result);
}

static cell_t Native_NavAreaIsBlocked(IPluginContext* context, const cell_t* params)
{
	CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

	if (!area)
	{
		context->ReportError("NULL nav area of address %x", params[1]);
		return 0;
	}

	int teamID = static_cast<int>(params[2]);
	bool ignorenavblockers = pawnutils::ReadBool(params, 3);
	return pawnutils::ReturnBool(area->IsBlocked(teamID, ignorenavblockers));
}

static cell_t Native_NavAreaGetClosestPointOnArea(IPluginContext* context, const cell_t* params)
{
	CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

	if (!area)
	{
		context->ReportError("NULL nav area of address %x", params[1]);
		return 0;
	}

	Vector pos = pawnutils::ReadVector(context, params, 2);
	Vector close(0.0f, 0.0f, 0.0f);
	area->GetClosestPointOnArea(&pos, &close);
	pawnutils::WriteVector(context, params, 3, close);
	return 0;
}

void natives::navarea::setup(std::vector<sp_nativeinfo_t>& nv)
{
	sp_nativeinfo_t list[] = {
		{"NavBotNavArea.GetID", Native_NavAreaGetID},
		{"NavBotNavArea.GetCenter", Native_NavAreaGetCenter},
		{"NavBotNavArea.GetCorner", Native_NavAreaGetCorner},
		{"NavBotNavArea.GetExtent", Native_NavAreaGetExtent},
		{"NavBotNavArea.IsVisible", Native_NavAreaIsVisible},
		{"NavBotNavArea.IsBlocked", Native_NavAreaIsBlocked},
		{"NavBotNavArea.GetClosestPointOnArea", Native_NavAreaGetClosestPointOnArea},
	};

	nv.insert(nv.end(), std::begin(list), std::end(list));
}