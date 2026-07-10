#include NAVBOT_PCH_FILE
#include <mods/basemod.h>
#include <mods/modhelpers.h>
#include "nav_mesh.h"
#include "nav_blocker_func_breakable.h"

bool CFuncBreakableNavBlocker::IsValid() const
{
	if (m_type == BreakableType::TYPE_NONE) { return false; }

	CBaseEntity* entity = m_breakable.Get();

	if (!entity)
	{
		return false;
	}

	return true;
}

void CFuncBreakableNavBlocker::Update()
{
	switch (m_type)
	{
	case BreakableType::TYPE_FILTER:
	{
		return;
	}
	case BreakableType::TYPE_BIGHEALTH:
	{
		m_blocked = modhelpers->IsEntityDamageable(m_breakable.Get(), extmanager->GetMod()->GetModSettings()->GetBreakableMaxHealth());
		return;
	}
	default:
		return;
	}
}

bool CFuncBreakableNavBlocker::IsBlocked(int teamID) const
{
	if (m_teamNum != NAV_TEAM_ANY)
	{
		// inverted filter logic
		if (m_negated)
		{
			if (m_teamNum == teamID)
			{
				return m_teamOnly;
			}

			return m_blocked;
		}
		else
		{
			if (m_teamNum == teamID)
			{
				return m_blocked;
			}

			return m_teamOnly;
		}
	}

	return m_blocked;
}

void CFuncBreakableNavBlocker::PrintDebugInfo() const
{
	CBaseEntity* breakable = m_breakable.Get();
	META_CONPRINTF("Breakable: %s \n", UtilHelpers::textformat::FormatEntity(breakable));
	META_CONPRINTF("Type: %i Team Num: %i Team Only: %i Negated: %i \n", static_cast<int>(m_type), m_teamNum, static_cast<int>(m_teamOnly), static_cast<int>(m_negated));

	if (breakable)
	{
		CBaseEntity* filter = entityprops::GetEntityDamageFilter(breakable);
		META_CONPRINTF("Damage Filter: %s \n", UtilHelpers::textformat::FormatEntity(filter));
	}
}

void CFuncBreakableNavBlocker::Init(CBaseEntity* breakable)
{
	AddAreasTouchingEntity(breakable, navgenparams->half_human_height, navgenparams->half_human_width);

	if (IsAreaVectorEmpty())
	{
		m_breakable.Term();
		return;
	}

	m_breakable = breakable;
	int spawnflags = entityprops::GetEntitySpawnFlags(breakable);

	if ((spawnflags & SF_BREAK_ONLY_ON_TRIGGER) != 0)
	{
		m_blocked = true; // these are always blocked until the entity is triggered
		m_type = BreakableType::TYPE_TRIGGER_ONLY;
		return;
	}

	const CBaseMod* mod = extmanager->GetMod();

	CBaseEntity* filter = entityprops::GetEntityDamageFilter(breakable);

	if (filter)
	{
		if (CheckFilter(filter))
		{
			m_type = BreakableType::TYPE_FILTER;
			return;
		}
	}

	if (!modhelpers->IsEntityDamageable(breakable, mod->GetModSettings()->GetBreakableMaxHealth()))
	{
		m_type = BreakableType::TYPE_BIGHEALTH;
		return;
	}

	// Remove the blocker so it doesn't block the path, bots will automatically try breaking it.
	// this will cause IsValid to return false and the nav mesh will destroy this instance
	m_breakable.Term();
}

bool CFuncBreakableNavBlocker::CheckFilter(CBaseEntity* filter)
{
	if (UtilHelpers::FClassnameIs(filter, "filter_activator_team"))
	{
		int teamNum = TEAM_UNASSIGNED;
		entprops->GetEntProp(filter, Prop_Data, "m_iFilterTeam", teamNum);

		if (teamNum > TEAM_UNASSIGNED)
		{
			entprops->GetEntPropBool(filter, Prop_Data, "m_bNegated", m_negated);

			m_blocked = false;
			m_teamNum = teamNum;
			m_teamOnly = true;
			return true;
		}
	}

	// TO-DO: check for filter_activator_class

	return false;
}
