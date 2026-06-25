#include NAVBOT_PCH_FILE
#include <navmesh/nav_blocker_door.h>
#include "zps_nav_mesh.h"

extern NavAreaVector TheNavAreas;

CNavArea* CZPSNavMesh::CreateArea(void) const
{
	return new CZPSNavArea(GetNavPlace());
}

class CZPSDoorBlocker : public CDoorNavBlocker
{
public:
	void Update() override
	{
		CDoorNavBlocker::Update();

		if (m_doortype == DOORTYPE_BRUSH)
		{
			CBaseEntity* entity = m_door.Get();
			int togglestate = static_cast<int>(TOGGLE_STATE::TS_AT_BOTTOM);
			entprops->GetEntProp(entity, Prop_Data, "m_toggle_state", togglestate);

			if (togglestate == static_cast<int>(TOGGLE_STATE::TS_AT_TOP) || togglestate == static_cast<int>(TOGGLE_STATE::TS_GOING_UP))
			{
				// open doors don't block
				m_blocked = false;
				// once open, no longer team only
				m_teamNum = NAV_TEAM_ANY;
				m_teamOnly = false;
				return;
			}
		}

		if (m_activatortype == ACTIVATOR_BUTTON)
		{
			CBaseEntity* button = m_button.Get();

			if (button)
			{
				// update team, inputs can change the button's team
				int team = 0;
				entprops->GetEntProp(button, Prop_Data, "m_iFilterAllowedTeam", team);

				if (team > 0)
				{
					m_teamNum = team;
					m_teamOnly = true;
				}
				else
				{
					m_teamNum = NAV_TEAM_ANY;
					m_teamOnly = false;
				}
			}
		}
	}

protected:
	void UpdateDoor() override
	{
		CDoorNavBlocker::UpdateDoor();
	}

};

CDoorNavBlocker* CZPSNavMesh::CreateDoorBlocker() const
{
	return new CZPSDoorBlocker;
}