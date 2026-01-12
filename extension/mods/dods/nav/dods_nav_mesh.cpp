#include NAVBOT_PCH_FILE
#include <extension.h>
#include <mods/dods/dodslib.h>
#include <ports/rcbot2_waypoint.h>
#include "dods_nav_area.h"
#include "dods_nav_mesh.h"
#include "dods_nav_waypoint.h"

CDoDSNavMesh::CDoDSNavMesh() :
	CNavMesh()
{
}

CDoDSNavMesh::~CDoDSNavMesh()
{
}

void CDoDSNavMesh::RegisterModCommands()
{
	RegisterEditCommands();
	CDoDSWaypoint::RegisterCommands();
}

unsigned int CDoDSNavMesh::GetGenerationTraceMask(void) const
{
	return MASK_PLAYERSOLID_BRUSHONLY;
}

CNavArea* CDoDSNavMesh::CreateArea(void) const
{
	return new CDoDSNavArea(GetNavPlace());
}

void CDoDSNavMesh::OnPreRCBot2WaypointImport(const CRCBot2WaypointLoader& loader)
{
}

void CDoDSNavMesh::OnRCBot2WaypointImported(const CRCBot2Waypoint& waypoint, const CRCBot2WaypointLoader& loader)
{
	using namespace dayofdefeatsource;

	if (waypoint.HasFlags(CRCBot2Waypoint::W_FL_UNREACHABLE))
		return;

	CDoDSWaypoint* nbwpt = nullptr;

	if (waypoint.HasFlags(CRCBot2Waypoint::W_FL_SNIPER))
	{
		auto result = TheNavMesh->AddWaypoint(waypoint.GetOrigin());
		nbwpt = static_cast<CDoDSWaypoint*>(result.value().get());
		nbwpt->SetFlags(CWaypoint::BaseFlags::BASEFLAGS_SNIPER);
	}
	else if (waypoint.HasFlags(CRCBot2Waypoint::W_FL_MACHINEGUN))
	{
		auto result = TheNavMesh->AddWaypoint(waypoint.GetOrigin());
		nbwpt = static_cast<CDoDSWaypoint*>(result.value().get());
		nbwpt->SetDoDWaypointAttributes(CDoDSWaypoint::DOD_WP_ATTRIBS_MGNEST);
	}
	if (waypoint.HasFlags(CRCBot2Waypoint::W_FL_ROUTE) || waypoint.HasFlags(CRCBot2Waypoint::W_FL_CAPPOINT) || waypoint.HasFlags(CRCBot2Waypoint::W_FL_BOMBS_HERE))
	{
		auto result = TheNavMesh->AddWaypoint(waypoint.GetOrigin());
		nbwpt = static_cast<CDoDSWaypoint*>(result.value().get());
		nbwpt->SetFlags(CWaypoint::BaseFlags::BASEFLAGS_ROAM);
	}

	if (!nbwpt)
	{
		if (waypoint.HasFlags(CRCBot2Waypoint::W_FL_DEFEND))
		{
			auto result = TheNavMesh->AddWaypoint(waypoint.GetOrigin());
			nbwpt = static_cast<CDoDSWaypoint*>(result.value().get());
			nbwpt->SetFlags(CWaypoint::BaseFlags::BASEFLAGS_DEFEND);
		}
	}

	if (nbwpt)
	{
		QAngle angle{ 0.0f, static_cast<float>(waypoint.GetAimYaw()), 0.0f};
		nbwpt->AddAngle(angle);


		nbwpt->AssignToControlPoint(waypoint.GetAreaNumber());

		if (waypoint.HasFlags(CRCBot2Waypoint::W_FL_CROUCH))
		{
			nbwpt->SetFlags(CWaypoint::BaseFlags::BASEFLAGS_CROUCH);
		}

		// defend may sometimes be used with another flag
		if (waypoint.HasFlags(CRCBot2Waypoint::W_FL_DEFEND))
		{
			nbwpt->SetFlags(CWaypoint::BaseFlags::BASEFLAGS_DEFEND);
		}

		if (waypoint.HasFlags(CRCBot2Waypoint::W_FL_PRONE))
		{
			nbwpt->SetDoDWaypointAttributes(CDoDSWaypoint::DOD_WP_ATTRIBS_PRONE);
		}

		if (waypoint.HasFlags(CRCBot2Waypoint::W_FL_NOALLIES))
		{
			nbwpt->SetTeam(static_cast<int>(DoDTeam::DODTEAM_AXIS));
		}

		if (waypoint.HasFlags(CRCBot2Waypoint::W_FL_NOAXIS))
		{
			nbwpt->SetTeam(static_cast<int>(DoDTeam::DODTEAM_ALLIES));
		}
	}
}

std::shared_ptr<CWaypoint> CDoDSNavMesh::CreateWaypoint() const
{
	return std::make_shared<CDoDSWaypoint>();
}