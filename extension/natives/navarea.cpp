#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/pawnutils.h>
#include <navmesh/nav_mesh.h>
#include "navareavector.h"
#include "navarea.h"

namespace natives::navarea::offmesh
{
	static cell_t GetType(IPluginContext* context, const cell_t* params)
	{
		NavOffMeshConnection* conn = pawnutils::UnsafeCastPawnAddressToObject<NavOffMeshConnection>(context, params, 1);

		if (!conn)
		{
			context->ReportError("NULL off-mesh connection.");
			return 0;
		}

		return static_cast<cell_t>(conn->GetType());
	}
	static cell_t GetStartPosition(IPluginContext* context, const cell_t* params)
	{
		NavOffMeshConnection* conn = pawnutils::UnsafeCastPawnAddressToObject<NavOffMeshConnection>(context, params, 1);

		if (!conn)
		{
			context->ReportError("NULL off-mesh connection.");
			return 0;
		}

		pawnutils::WriteVector(context, params, 2, conn->GetStart());
		return 0;
	}
	static cell_t GetEndPosition(IPluginContext* context, const cell_t* params)
	{
		NavOffMeshConnection* conn = pawnutils::UnsafeCastPawnAddressToObject<NavOffMeshConnection>(context, params, 1);

		if (!conn)
		{
			context->ReportError("NULL off-mesh connection.");
			return 0;
		}

		pawnutils::WriteVector(context, params, 2, conn->GetEnd());
		return 0;
	}
	static cell_t GetConnectedArea(IPluginContext* context, const cell_t* params)
	{
		std::size_t firstparam = pawnutils::GetIndexOfParam(context, 1);
		NavOffMeshConnection* conn = pawnutils::UnsafeCastPawnAddressToObject<NavOffMeshConnection>(context, params, firstparam);

		if (!conn)
		{
			context->ReportError("NULL off-mesh connection.");
			return 0;
		}

		return pawnutils::ReturnPointerToPawn(context, params, conn->GetConnectedArea());
	}
	static cell_t GetConnectionLength(IPluginContext* context, const cell_t* params)
	{
		NavOffMeshConnection* conn = pawnutils::UnsafeCastPawnAddressToObject<NavOffMeshConnection>(context, params, 1);

		if (!conn)
		{
			context->ReportError("NULL off-mesh connection.");
			return 0;
		}

		return pawnutils::ReturnFloat(conn->GetConnectionLength());
	}

	void setup(std::vector<sp_nativeinfo_t>& nv)
	{
		sp_nativeinfo_t list[] = {
			{"NavBotNavOffMeshConnection.GetType", GetType},
			{"NavBotNavOffMeshConnection.GetStartPosition", GetStartPosition},
			{"NavBotNavOffMeshConnection.GetEndPosition", GetEndPosition},
			{"NavBotNavOffMeshConnection.GetConnectedArea", GetConnectedArea},
			{"NavBotNavOffMeshConnection.GetConnectionLength", GetConnectionLength},
		};

		nv.insert(nv.end(), std::begin(list), std::end(list));
	}
}

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

namespace natives::navarea
{
	static cell_t IsConnectedToAny(IPluginContext* context, const cell_t* params)
	{
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		CNavArea* other = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 2);

		if (!other)
		{
			context->ReportError("NULL nav area (second param)!");
			return 0;
		}

		return pawnutils::ReturnBool(area->HasAnyConnectionsTo(other));
	}
	static cell_t IsConnectedToDir(IPluginContext* context, const cell_t* params)
	{
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		CNavArea* other = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 2);

		if (!other)
		{
			context->ReportError("NULL nav area (second param)!");
			return 0;
		}

		if (params[3] < 0 || params[3] > static_cast<cell_t>(NavDirType::NUM_DIRECTIONS))
		{
			context->ReportError("Invalid direction %i!", params[3]);
			return 0;
		}

		return pawnutils::ReturnBool(area->IsConnected(other, static_cast<NavDirType>(params[3])));
	}
	static cell_t IsConnectedToOffMesh(IPluginContext* context, const cell_t* params)
	{
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		CNavArea* other = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 2);

		if (!other)
		{
			context->ReportError("NULL nav area (second param)!");
			return 0;
		}

		if (params[3] < 0 || params[3] >= static_cast<cell_t>(OffMeshConnectionType::MAX_OFFMESH_CONNECTION_TYPES))
		{
			context->ReportError("Invalid off-mesh connection type %i!", params[3]);
			return 0;
		}

		return pawnutils::ReturnBool(area->IsConnected(other, static_cast<OffMeshConnectionType>(params[3])));
	}
	static cell_t GetOffMeshConnectionCount(IPluginContext* context, const cell_t* params)
	{
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		return static_cast<cell_t>(area->GetOffMeshConnectionCount());
	}
	static cell_t GetOffMeshConnection(IPluginContext* context, const cell_t* params)
	{
		std::size_t firstparam = pawnutils::GetIndexOfParam(context, 1);
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, firstparam);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		std::size_t index = static_cast<std::size_t>(params[firstparam + 1U]);
		auto& connections = area->GetOffMeshConnections();

		if (index >= connections.size())
		{
			context->ReportError("Index %zu is out of bounds! Limit: %zu", index, connections.size());
			return 0;
		}

		const NavOffMeshConnection* conn = &connections[index];
		return pawnutils::ReturnPointerToPawn(context, params, conn);
	}
	static cell_t CollectConnectedAreas(IPluginContext* context, const cell_t* params)
	{
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		auto vec = natives::navmesh::navareavector::readhandle(context, static_cast<SourceMod::Handle_t>(params[2]));

		if (!vec)
		{
			return 0;
		}

		auto func = [&vec](CNavArea* connectedArea) {
			for (CNavArea* area : *vec)
			{
				if (area == connectedArea)
				{
					return;
				}
			}

			vec->push_back(connectedArea);
		};

		area->ForEachConnectedArea(func);
		return 0;
	}
	static cell_t ComputeAdjacentConnectionHeightChange(IPluginContext* context, const cell_t* params)
	{
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		CNavArea* other = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 2);

		if (!other)
		{
			context->ReportError("NULL nav area (second param)!");
			return 0;
		}

		return pawnutils::ReturnFloat(area->ComputeAdjacentConnectionHeightChange(other));
	}
	static cell_t ComputeAdjacentConnectionGapDistance(IPluginContext* context, const cell_t* params)
	{
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		CNavArea* other = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 2);

		if (!other)
		{
			context->ReportError("NULL nav area (second param)!");
			return 0;
		}

		return pawnutils::ReturnFloat(area->ComputeAdjacentConnectionGapDistance(other));
	}

	void setup(std::vector<sp_nativeinfo_t>& nv)
	{
		sp_nativeinfo_t list[] = {
			{"NavBotNavArea.GetID", Native_NavAreaGetID},
			{"NavBotNavArea.GetCenter", Native_NavAreaGetCenter},
			{"NavBotNavArea.GetCorner", Native_NavAreaGetCorner},
			{"NavBotNavArea.GetExtent", Native_NavAreaGetExtent},
			{"NavBotNavArea.IsVisible", Native_NavAreaIsVisible},
			{"NavBotNavArea.IsBlocked", Native_NavAreaIsBlocked},
			{"NavBotNavArea.GetClosestPointOnArea", Native_NavAreaGetClosestPointOnArea},
			{"NavBotNavArea.IsConnectedToAny", IsConnectedToAny},
			{"NavBotNavArea.IsConnectedToDir", IsConnectedToDir},
			{"NavBotNavArea.IsConnectedToOffMesh", IsConnectedToOffMesh},
			{"NavBotNavArea.GetOffMeshConnectionCount", GetOffMeshConnectionCount},
			{"NavBotNavArea.GetOffMeshConnection", GetOffMeshConnection},
			{"NavBotNavArea.CollectConnectedAreas", CollectConnectedAreas},
			{"NavBotNavArea.ComputeAdjacentConnectionHeightChange", ComputeAdjacentConnectionHeightChange},
			{"NavBotNavArea.ComputeAdjacentConnectionGapDistance", ComputeAdjacentConnectionGapDistance},
		};

		nv.insert(nv.end(), std::begin(list), std::end(list));

		natives::navarea::offmesh::setup(nv);
	}
}
