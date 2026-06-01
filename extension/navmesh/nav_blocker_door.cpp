#include NAVBOT_PCH_FILE
#include <mods/modhelpers.h>
#include "nav_mesh.h"
#include "nav_blocker_door.h"

CDoorNavBlocker::CDoorNavBlocker()
{
	m_doortype = DOORTYPE_BRUSH;
	m_activatortype = ACTIVATOR_NONE;
	m_blocked = false;
	m_teamOnly = false;
	m_negated = false;
	m_teamNum = NAV_TEAM_ANY;
}

bool CDoorNavBlocker::IsValid()
{
	return m_door.Get() != nullptr;
}

void CDoorNavBlocker::Update()
{
	// Update is only called if IsValid returns true, which checks if the door is non-NULL.
	CBaseEntity* door = m_door.Get();
	bool locked = false;
	m_blocked = false;

	entprops->GetEntPropBool(door, Prop_Data, "m_bLocked", locked);

	if (locked)
	{
		// door is locked, mark as blocked
		m_blocked = true;
		return;
	}

	if (m_doortype == DOORTYPE_BRUSH)
	{
		int togglestate = static_cast<int>(TS_AT_TOP);
		entprops->GetEntProp(door, Prop_Data, "m_toggle_state", togglestate);

		// door is currently open, assume not blocked
		if (togglestate == static_cast<int>(TS_AT_TOP) || togglestate == static_cast<int>(TS_GOING_UP))
		{
			m_blocked = false;
			return;
		}
	}

	CBaseEntity* trigger = m_trigger.Get();

	if (m_activatortype == ACTIVATOR_TRIGGER && trigger)
	{
		bool disabled = false; 
		entprops->GetEntPropBool(trigger, Prop_Data, "m_bDisabled", disabled);

		if (disabled)
		{
			// trigger volume for the door is disabled, mark as blocked
			m_blocked = true;
			return;
		}
	}
}

bool CDoorNavBlocker::IsBlocked(int teamID)
{
	if (m_teamNum != NAV_TEAM_ANY)
	{
		// inverted filter logic
		if (m_negated)
		{
			// assigned team is blocked if this is a team only door
			if (m_teamNum == teamID)
			{
				return m_teamOnly;
			}

			// other teams can pass if the door is not blocked
			return m_blocked;
		}
		else
		{
			// assigned team can pass if not blocked
			if (m_teamNum == teamID)
			{
				return m_blocked;
			}

			// other teams can't pass if it's a team only door.
			return m_teamOnly;
		}
	}

	return m_blocked;
}

void CDoorNavBlocker::PrintDebugInfo()
{
	META_CONPRINTF("Door: %s\n", UtilHelpers::textformat::FormatEntity(m_door.Get()));
	META_CONPRINTF("Trigger: %s\n", UtilHelpers::textformat::FormatEntity(m_trigger.Get()));
	META_CONPRINTF("Filter: %s\n", UtilHelpers::textformat::FormatEntity(m_filter.Get()));
	META_CONPRINTF("Team Only: %s Team: %i\n", UtilHelpers::textformat::FormatBool(m_teamOnly), m_teamNum);
}

void CDoorNavBlocker::UpdateDoor()
{
	CBaseEntity* door = m_door.Get();

	char targetname[128];
	targetname[0] = 0;
	size_t length = 0;

	entprops->GetEntPropString(door, Prop_Data, "m_iName", targetname, sizeof(targetname), length);

	// unnamed door, assume never blocks
	if (targetname[0] == '\0' || length == 0U)
	{
		m_door.Term(); // invalidate the ehandle to trigger a destruction of this instance
		return;
	}

	if (UtilHelpers::FClassnameIs(door, "prop_door*"))
	{
		m_doortype = DOORTYPE_PROP; // brush is default, no need to set again
	}

	Extent doorExtent{ door };
	doorExtent.lo.z -= navgenparams->step_height;
	doorExtent.hi.z += navgenparams->step_height;
	TheNavMesh->CollectAreasOverlappingExtent(doorExtent, m_areas);

	// collect triggers connected to this door
	std::vector<CBaseEntity*> triggers;
	auto func = [&targetname, &triggers](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			if (UtilHelpers::io::IsConnectedTo(entity, targetname))
			{
				triggers.push_back(entity);
			}
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("trigger_multiple", func);
	CBaseEntity* trigger = nullptr; // selected trigger
	CBaseEntity* filter = nullptr;

	if (!triggers.empty())
	{
		// filter triggers
		if (triggers.size() == 1)
		{
			// if only one trigger, always use it
			trigger = triggers[0];
		}
		else
		{
			for (auto& entity : triggers)
			{
				bool disabled = false;
				entprops->GetEntPropBool(entity, Prop_Data, "m_bDisabled", disabled);

				// TF2: some doors have one trigger per team, we need to find which trigger is currently active.
				if (disabled)
				{
					continue;
				}

				// use the first available entity
				trigger = entity;
				break;
			}

			// all triggers were filtered
			if (!trigger)
			{
				// take the first one
				trigger = triggers[0];
			}
		}

		if (trigger)
		{
			m_activatortype = ACTIVATOR_TRIGGER;
			m_trigger = trigger;
			entprops->GetEntPropEnt(trigger, Prop_Data, "m_hFilter", nullptr, &filter);

			if (filter)
			{
				m_filter = filter;
			}
		}
	}

	int team = modhelpers->GetEntityTeamNumber(door);
	m_teamNum = team == TEAM_UNASSIGNED ? NAV_TEAM_ANY : team;

	if (trigger && filter)
	{
		if (UtilHelpers::FClassnameIs(filter, "filter_activator_team"))
		{
			int teamNum = TEAM_UNASSIGNED;
			entprops->GetEntProp(filter, Prop_Data, "m_iFilterTeam", teamNum);

			if (teamNum > TEAM_UNASSIGNED)
			{
				entprops->GetEntPropBool(filter, Prop_Data, "m_bNegated", m_negated);

				m_teamNum = teamNum;
				m_teamOnly = true;
			}
		}
	}

	if (m_areas.empty())
	{
		m_door.Term(); // invalidate the ehandle to trigger a destruction of this instance
		return;
	}
}
