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

	static void setup(std::vector<sp_nativeinfo_t>& nv)
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
	static cell_t GetAdjacentAreaCount(IPluginContext* context, const cell_t* params)
	{
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		NavDirType dir = static_cast<NavDirType>(params[2]);

		if (dir < NavDirType::NORTH || dir >= NavDirType::NUM_DIRECTIONS)
		{
			context->ReportError("%i is not a valid direction!", params[2]);
			return 0;
		}

		return area->GetAdjacentCount(dir);
	}
	static cell_t GetAdjacentArea(IPluginContext* context, const cell_t* params)
	{
		const std::size_t start = pawnutils::GetIndexOfParam(context, 1);
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, start);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		NavDirType dir = static_cast<NavDirType>(params[start + 1]);

		if (dir < NavDirType::NORTH || dir >= NavDirType::NUM_DIRECTIONS)
		{
			context->ReportError("%i is not a valid direction!", params[start + 1]);
			return 0;
		}

		int index = static_cast<int>(params[start + 2]);
		const int c = area->GetAdjacentCount(dir);

		if (index < 0 || index >= c)
		{
			context->ReportError("Index %i is out of bounds! Must be between 0 and %i!", params[start + 2], c);
			return 0;
		}

		CNavArea* adjacent = area->GetAdjacentArea(dir, index);
		return pawnutils::ReturnPointerToPawn(context, params, adjacent);
	}
	static cell_t ConnectToAdjacent(IPluginContext* context, const cell_t* params)
	{
		CNavArea* from = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

		if (!from)
		{
			context->ReportError("NULL from nav area!");
			return 0;
		}

		CNavArea* to = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 2);

		if (!to)
		{
			context->ReportError("NULL to nav area!");
			return 0;
		}

		Vector center;
		float halfWidth;
		NavDirType dir = from->ComputeLargestPortal(to, &center, &halfWidth);

		if (dir == NavDirType::NUM_DIRECTIONS)
		{
			return pawnutils::ReturnBool(false);
		}

		from->ConnectTo(to, dir);
		return pawnutils::ReturnBool(true);
	}
	static cell_t DisconnectFromAdjacent(IPluginContext* context, const cell_t* params)
	{
		CNavArea* from = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

		if (!from)
		{
			context->ReportError("NULL from nav area!");
			return 0;
		}

		CNavArea* to = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 2);

		if (!to)
		{
			context->ReportError("NULL to nav area!");
			return 0;
		}

		from->Disconnect(to);
		return 0;
	}
	static cell_t GetPlaceName(IPluginContext* context, const cell_t* params)
	{
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		Place place = area->GetPlace();
		const std::string* name = TheNavMesh->GetPlaceName(place);

		if (!name)
		{
			return pawnutils::ReturnBool(false);
		}

		context->StringToLocal(params[2], static_cast<std::size_t>(params[3]), name->c_str());
		return pawnutils::ReturnBool(true);
	}
	static cell_t IsUnderwater(IPluginContext* context, const cell_t* params)
	{
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		return pawnutils::ReturnBool(area->IsUnderwater());
	}
	static cell_t IsInShallowWater(IPluginContext* context, const cell_t* params)
	{
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		return pawnutils::ReturnBool(area->IsInShallowWater());
	}
	static cell_t IsInWater(IPluginContext* context, const cell_t* params)
	{
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		return pawnutils::ReturnBool(area->IsInWater());
	}
	static cell_t IsOverlappingPos(IPluginContext* context, const cell_t* params)
	{
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		Vector pos = pawnutils::ReadVector(context, params, 2);
		float tolerance = pawnutils::ReadFloat(params, 3);

		return pawnutils::ReturnBool(area->IsOverlapping(pos, tolerance));
	}
	static cell_t IsContiguous(IPluginContext* context, const cell_t* params)
	{
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

		if (!area)
		{
			context->ReportError("NULL nav area (first)!");
			return 0;
		}

		CNavArea* other = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 2);

		if (!other)
		{
			context->ReportError("NULL nav area (second)!");
			return 0;
		}

		return pawnutils::ReturnBool(area->IsContiguous(other));
	}
	static cell_t GetDanger(IPluginContext* context, const cell_t* params)
	{
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		int teamindex = static_cast<int>(params[2]);

		// Throw an error if an invalid team is passed to alert plugin coders since they can't tell if the team is invalid just by the return value alone.
		if (teamindex < 0 || teamindex >= static_cast<int>(NAV_TEAMS_ARRAY_SIZE))
		{
			context->ReportError("Team index %i is out of bounds! Should be between 0 and %u.", teamindex, NAV_TEAMS_ARRAY_SIZE - 1U);
			return 0;
		}

		return pawnutils::ReturnFloat(area->GetDanger(teamindex));
	}
	static cell_t IncreaseDanger(IPluginContext* context, const cell_t* params)
	{
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		int teamindex = static_cast<int>(params[2]);
		float amount = pawnutils::ReadFloat(params, 3);
		area->IncreaseDanger(teamindex, amount);
		return 0;
	}
	static cell_t ClearDanger(IPluginContext* context, const cell_t* params)
	{
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		int teamindex = static_cast<int>(params[2]);
		area->ClearDanger(teamindex);
		return 0;
	}
	static cell_t GetSizeX(IPluginContext* context, const cell_t* params)
	{
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		return pawnutils::ReturnFloat(area->GetSizeX());
	}
	static cell_t GetSizeY(IPluginContext* context, const cell_t* params)
	{
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		return pawnutils::ReturnFloat(area->GetSizeY());
	}
	static cell_t GetRandomPoint(IPluginContext* context, const cell_t* params)
	{
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		Vector point = area->GetRandomPoint();
		pawnutils::WriteVector(context, params, 2, point);
		return 0;
	}
	static cell_t GetDistanceSquaredToPoint(IPluginContext* context, const cell_t* params)
	{
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		Vector point = pawnutils::ReadVector(context, params, 2);
		return pawnutils::ReturnFloat(area->GetDistanceSquaredToPoint(point));
	}
	static cell_t GetCostSoFar(IPluginContext* context, const cell_t* params)
	{
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		return pawnutils::ReturnFloat(area->GetCostSoFar());
	}
	static cell_t ComputeLargestPortal(IPluginContext* context, const cell_t* params)
	{
		CNavArea* first = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);
		CNavArea* second = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 2);

		if (first == nullptr || second == nullptr)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		Vector center(0.0f, 0.0f, 0.0f);
		float halfWidth = 0.0f;
		NavDirType dir = first->ComputeLargestPortal(second, &center, &halfWidth);
		pawnutils::WriteVector(context, params, 3, center);
		pawnutils::WriteFloatByRef(context, params, 4, halfWidth);
		return static_cast<cell_t>(dir);
	}
	static cell_t ComputePortal(IPluginContext* context, const cell_t* params)
	{
		CNavArea* first = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);
		CNavArea* second = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 2);

		if (first == nullptr || second == nullptr)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		Vector center(0.0f, 0.0f, 0.0f);
		float halfWidth = 0.0f;
		NavDirType dir = static_cast<NavDirType>(params[3]);
		first->ComputePortal(second, dir, &center, &halfWidth);
		pawnutils::WriteVector(context, params, 4, center);
		pawnutils::WriteFloatByRef(context, params, 5, halfWidth);
		return 0;
	}
	static cell_t ComputeClosestPointInPortal(IPluginContext* context, const cell_t* params)
	{
		CNavArea* first = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);
		CNavArea* second = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 2);

		if (first == nullptr || second == nullptr)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		NavDirType dir = static_cast<NavDirType>(params[3]);
		Vector fromPos = pawnutils::ReadVector(context, params, 4);
		Vector close(0.0f, 0.0f, 0.0f);
		first->ComputeClosestPointInPortal(second, dir, fromPos, &close);
		pawnutils::WriteVector(context, params, 5, close);
		return 0;
	}
	static cell_t IsEdge(IPluginContext* context, const cell_t* params)
	{
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

		if (area == nullptr)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		NavDirType dir = static_cast<NavDirType>(params[2]);
		return pawnutils::ReturnBool(area->IsEdge(dir));
	}
	static cell_t ComputeNormal(IPluginContext* context, const cell_t* params)
	{
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

		if (area == nullptr)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		bool alternate = params[3] != 0;
		Vector normal(0.0f, 0.0f, 0.0f);
		area->ComputeNormal(&normal, alternate);
		pawnutils::WriteVector(context, params, 2, normal);
		return 0;
	}
	static cell_t SearchConnectedAreas(IPluginContext* context, const cell_t* params)
	{
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 1);

		if (area == nullptr)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		SourcePawn::IPluginFunction* callback = nullptr;
		if (!pawnutils::ReadFunctionByID(context, params, 2, &callback))
		{
			return 0;
		}

		cell_t data = params[3];
		DetectExceptions eh(context);
		
		auto func = [&callback, &eh, &area, &data](CNavArea* neighborArea, NavTraverseType how) {
			if (eh.HasException())
			{
				return false;
			}

			cell_t result = 0;

#if SMINTERFACE_EXTENSIONAPI_VERSION >= 9
			sp::CallArgs args;
			pawnutils::PushPtrToCall(&args, area);
			pawnutils::PushPtrToCall(&args, neighborArea);
			args.PushCell(static_cast<cell_t>(how));
			args.PushCell(data);
			callback->Invoke(args, &result);
#else
			pawnutils::PushPtrToCall(callback, area);
			pawnutils::PushPtrToCall(callback, neighborArea);
			callback->PushCell(static_cast<cell_t>(how));
			callback->PushCell(data);
			callback->Invoke(&result);
#endif // SMINTERFACE_EXTENSIONAPI_VERSION >= 9

			return result != 0;
		};

		bool result = area->SearchConnectedAreas(func);
		return pawnutils::ReturnBool(result);
	}
	static cell_t GetOffMeshConnectionToArea(IPluginContext* context, const cell_t* params)
	{
		std::size_t firstparam = pawnutils::GetIndexOfParam(context, 1);
		CNavArea* from = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, firstparam);
		CNavArea* to = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, firstparam + 1);

		if (from == nullptr || to == nullptr)
		{
			context->ReportError("NULL nav area !");
			return 0;
		}

		const NavOffMeshConnection* conn = from->GetOffMeshConnectionToArea(to);
		return pawnutils::ReturnPointerToPawn(context, params, conn);
	}
	static cell_t GetLadderConnectionToArea(IPluginContext* context, const cell_t* params)
	{
		std::size_t firstparam = pawnutils::GetIndexOfParam(context, 1);
		CNavArea* from = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, firstparam);
		CNavArea* to = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, firstparam + 1);

		if (from == nullptr || to == nullptr)
		{
			context->ReportError("NULL nav area !");
			return 0;
		}

		const CNavLadder* ladder = from->GetLadderConnectionToArea(to);
		return pawnutils::ReturnPointerToPawn(context, params, ladder);
	}
	static cell_t GetElevatorConnectionToArea(IPluginContext* context, const cell_t* params)
	{
		std::size_t firstparam = pawnutils::GetIndexOfParam(context, 1);
		CNavArea* from = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, firstparam);
		CNavArea* to = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, firstparam + 1);

		if (from == nullptr || to == nullptr)
		{
			context->ReportError("NULL nav area !");
			return 0;
		}

		const CNavElevator* elev = from->GetElevatorConnectionToArea(to);
		return pawnutils::ReturnPointerToPawn(context, params, elev);
	}
	static cell_t ConnectToOffMesh(IPluginContext* context, const cell_t* params)
	{
		constexpr std::size_t firstparam = 1U;
		CNavArea* from = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, firstparam);
		CNavArea* to = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, firstparam + 1);

		if (from == nullptr || to == nullptr)
		{
			context->ReportError("NULL nav area !");
			return 0;
		}

		OffMeshConnectionType type = static_cast<OffMeshConnectionType>(params[firstparam + 2]);

		if (type <= OffMeshConnectionType::OFFMESH_INVALID || type >= OffMeshConnectionType::MAX_OFFMESH_CONNECTION_TYPES)
		{
			context->ReportError("%i is not a valid off-mesh connection type!", params[firstparam + 2]);
			return 0;
		}

		Vector start = pawnutils::ReadVector(context, params, firstparam + 3);
		Vector end = pawnutils::ReadVector(context, params, firstparam + 4);
		return pawnutils::ReturnBool(from->ConnectTo(to, type, start, end));
	}
	static cell_t DisconnectOffMesh(IPluginContext* context, const cell_t* params)
	{
		constexpr std::size_t firstparam = 1U;
		CNavArea* from = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, firstparam);
		CNavArea* to = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, firstparam + 1);

		if (from == nullptr || to == nullptr)
		{
			context->ReportError("NULL nav area !");
			return 0;
		}

		OffMeshConnectionType type = static_cast<OffMeshConnectionType>(params[firstparam + 2]);

		if (type <= OffMeshConnectionType::OFFMESH_INVALID || type >= OffMeshConnectionType::MAX_OFFMESH_CONNECTION_TYPES)
		{
			context->ReportError("%i is not a valid off-mesh connection type!", params[firstparam + 2]);
			return 0;
		}

		from->Disconnect(to, type);
		return 0;
	}
	static cell_t GetIncomingConnectionsCount(IPluginContext* context, const cell_t* params)
	{
		constexpr std::size_t firstparam = 1U;
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, firstparam);

		if (area == nullptr)
		{
			context->ReportError("NULL nav area !");
			return 0;
		}

		NavDirType dir = static_cast<NavDirType>(params[firstparam + 1]);

		if (dir < NavDirType::NORTH || dir >= NavDirType::NUM_DIRECTIONS)
		{
			context->ReportError("%i is not a valid direction!", params[firstparam + 1]);
			return 0;
		}

		return static_cast<cell_t>(area->GetIncomingConnections(dir)->Count());
	}
	static cell_t GetIncomingConnection(IPluginContext* context, const cell_t* params)
	{
		std::size_t firstparam = pawnutils::GetIndexOfParam(context, 1);
		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, firstparam);

		if (area == nullptr)
		{
			context->ReportError("NULL nav area !");
			return 0;
		}

		NavDirType dir = static_cast<NavDirType>(params[firstparam + 1]);

		if (dir < NavDirType::NORTH || dir >= NavDirType::NUM_DIRECTIONS)
		{
			context->ReportError("%i is not a valid direction!", params[firstparam + 1]);
			return 0;
		}

		int idx = params[firstparam + 2];
		const NavConnectVector* vec = area->GetIncomingConnections(dir);

		if (idx < 0 || idx >= vec->Count())
		{
			context->ReportError("Index %i is out of bounds! Range: 0 - %i", idx, vec->Count() - 1);
			return 0;
		}

		auto& conn = (*vec)[idx];
		return pawnutils::ReturnPointerToPawn(context, params, conn.area);
	}
	static cell_t ComputeGroundHeightChange(IPluginContext* context, const cell_t* params)
	{
		constexpr std::size_t firstparam = 1U;
		CNavArea* from = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, firstparam);
		CNavArea* to = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, firstparam + 1);

		if (from == nullptr || to == nullptr)
		{
			context->ReportError("NULL nav area !");
			return 0;
		}

		return pawnutils::ReturnFloat(from->ComputeGroundHeightChange(to));
	}

	void setup(std::vector<sp_nativeinfo_t>& nv)
	{
		sp_nativeinfo_t list[] = {
			{ "NavBotNavArea.GetID", Native_NavAreaGetID },
			{ "NavBotNavArea.GetCenter", Native_NavAreaGetCenter },
			{ "NavBotNavArea.GetCorner", Native_NavAreaGetCorner },
			{ "NavBotNavArea.GetExtent", Native_NavAreaGetExtent },
			{ "NavBotNavArea.IsVisible", Native_NavAreaIsVisible },
			{ "NavBotNavArea.IsBlocked", Native_NavAreaIsBlocked },
			{ "NavBotNavArea.GetClosestPointOnArea", Native_NavAreaGetClosestPointOnArea },
			{ "NavBotNavArea.IsConnectedToAny", IsConnectedToAny },
			{ "NavBotNavArea.IsConnectedToDir", IsConnectedToDir },
			{ "NavBotNavArea.IsConnectedToOffMesh", IsConnectedToOffMesh },
			{ "NavBotNavArea.GetOffMeshConnectionCount", GetOffMeshConnectionCount },
			{ "NavBotNavArea.GetOffMeshConnection", GetOffMeshConnection },
			{ "NavBotNavArea.CollectConnectedAreas", CollectConnectedAreas },
			{ "NavBotNavArea.ComputeAdjacentConnectionHeightChange", ComputeAdjacentConnectionHeightChange },
			{ "NavBotNavArea.ComputeAdjacentConnectionGapDistance", ComputeAdjacentConnectionGapDistance },
			{ "NavBotNavArea.GetAdjacentAreaCount", GetAdjacentAreaCount },
			{ "NavBotNavArea.GetAdjacentArea", GetAdjacentArea },
			{ "NavBotNavArea.ConnectToAdjacent", ConnectToAdjacent },
			{ "NavBotNavArea.DisconnectFromAdjacent", DisconnectFromAdjacent },
			{ "NavBotNavArea.GetPlaceName", GetPlaceName },
			{ "NavBotNavArea.IsUnderwater", IsUnderwater },
			{ "NavBotNavArea.IsInShallowWater", IsInShallowWater },
			{ "NavBotNavArea.IsInWater", IsInWater },
			{ "NavBotNavArea.IsOverlappingPos", IsOverlappingPos },
			{ "NavBotNavArea.IsContiguous", IsContiguous },
			{ "NavBotNavArea.GetDanger", GetDanger },
			{ "NavBotNavArea.IncreaseDanger", IncreaseDanger },
			{ "NavBotNavArea.ClearDanger", ClearDanger },
			{ "NavBotNavArea.GetSizeX", GetSizeX },
			{ "NavBotNavArea.GetSizeY", GetSizeY },
			{ "NavBotNavArea.GetRandomPoint", GetRandomPoint },
			{ "NavBotNavArea.GetDistanceSquaredToPoint", GetDistanceSquaredToPoint },
			{ "NavBotNavArea.GetCostSoFar", GetCostSoFar },
			{ "NavBotNavArea.ComputeLargestPortal", ComputeLargestPortal },
			{ "NavBotNavArea.ComputePortal", ComputePortal },
			{ "NavBotNavArea.ComputeClosestPointInPortal", ComputeClosestPointInPortal },
			{ "NavBotNavArea.IsEdge", IsEdge },
			{ "NavBotNavArea.ComputeNormal", ComputeNormal },
			{ "NavBotNavArea.SearchConnectedAreas", SearchConnectedAreas },
			{ "NavBotNavArea.GetOffMeshConnectionToArea", GetOffMeshConnectionToArea },
			{ "NavBotNavArea.GetLadderConnectionToArea", GetLadderConnectionToArea },
			{ "NavBotNavArea.GetElevatorConnectionToArea", GetElevatorConnectionToArea },
			{ "NavBotNavArea.ConnectToOffMesh", ConnectToOffMesh },
			{ "NavBotNavArea.DisconnectOffMesh", DisconnectOffMesh },
			{ "NavBotNavArea.GetIncomingConnectionsCount", GetIncomingConnectionsCount },
			{ "NavBotNavArea.GetIncomingConnection", GetIncomingConnection },
			{ "NavBotNavArea.ComputeGroundHeightChange", ComputeGroundHeightChange },
		};

		nv.insert(nv.end(), std::begin(list), std::end(list));

		natives::navarea::offmesh::setup(nv);
	}
}
