#include NAVBOT_PCH_FILE
#include <mods/modhelpers.h>
#include "tfnavmesh.h"
#include "tfnav_door_blocker.h"

void CTFDoorNavBlocker::UpdateDoor()
{
	// run base
	CDoorNavBlocker::UpdateDoor();

	CBaseEntity* door = m_door.Get();

	if (!door)
	{
		return;
	}

	CBaseEntity* trigger = m_trigger.Get();
	CBaseEntity* filter = m_filter.Get();

	if (trigger && filter)
	{
		if (UtilHelpers::FClassnameIs(filter, "filter_activator_tfteam"))
		{
			m_teamOnly = true;
			m_teamNum = modhelpers->GetEntityTeamNumber(filter);
		}
	}
}
