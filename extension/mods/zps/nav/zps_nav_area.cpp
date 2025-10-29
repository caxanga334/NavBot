#include NAVBOT_PCH_FILE
#include <mods/zps/zps_mod.h>
#include "zps_nav_area.h"

CZPSNavArea::CZPSNavArea(unsigned int place) :
	CNavArea(place)
{
	m_zpsattributes = 0;
	m_blockedByPhysProps = false;
}

void CZPSNavArea::Save(std::fstream& filestream, uint32_t version)
{
	CNavArea::Save(filestream, version);

	filestream.write(reinterpret_cast<char*>(&m_zpsattributes), sizeof(int));
}

NavErrorType CZPSNavArea::Load(std::fstream& filestream, uint32_t version, uint32_t subVersion)
{
	if (filestream.bad())
	{
		return NAV_CORRUPT_DATA;
	}
	
	NavErrorType base = CNavArea::Load(filestream, version, subVersion);

	filestream.read(reinterpret_cast<char*>(&m_zpsattributes), sizeof(int));

	return base;
}

void CZPSNavArea::OnUpdate()
{
	CNavArea::OnUpdate();
}

void CZPSNavArea::OnRoundRestart(void)
{
	CNavArea::OnRoundRestart();
	m_blockedByPhysProps = false;
}

bool CZPSNavArea::IsBlocked(int teamID, bool ignoreNavBlockers) const
{
	if (HasZPSAttributes(ZPS_ATTRIBUTE_TRANSIENT_PHYSPROPS) && m_blockedByPhysProps)
	{
		return true;
	}

	if (HasZPSAttributes(ZPS_ATTRIBUTE_TRANSIENT_SURVIVORSONLY) && teamID == static_cast<int>(zps::ZPSTeam::ZPS_TEAM_SURVIVORS) && m_blockedByPhysProps)
	{
		return true;
	}

	return CNavArea::IsBlocked(teamID, ignoreNavBlockers);
}

void CZPSNavArea::UpdateBlocked(bool force, int teamID)
{
	if (HasZPSAttributes(ZPS_ATTRIBUTE_TRANSIENT_PHYSPROPS) || HasZPSAttributes(ZPS_ATTRIBUTE_TRANSIENT_SURVIVORSONLY))
	{
		m_blockedByPhysProps = UpdateZPSPhysPropsTransientStatus();
	}

	return CNavArea::UpdateBlocked(force, teamID);
}

static bool ZPSNavArea_EnumeratorFilter_PhysProps(IHandleEntity* handle)
{
	CBaseEntity* entity = trace::EntityFromEntityHandle(handle);

	if (entity && UtilHelpers::FClassnameIs(entity, "prop_phys*"))
	{
		return true;
	}

	return false;
}

bool CZPSNavArea::UpdateZPSPhysPropsTransientStatus()
{
	Extent extent;
	GetExtent(&extent);

	auto func = std::bind(ZPSNavArea_EnumeratorFilter_PhysProps, std::placeholders::_1);
	UtilHelpers::CEntityEnumerator enumerator{ func };
	UtilHelpers::EntitiesInBox(extent.lo, extent.hi, enumerator);

	return enumerator.Count() > 0;
}
