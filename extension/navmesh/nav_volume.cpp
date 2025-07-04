#include NAVBOT_PCH_FILE
#include <string_view>
#include <extension.h>
#include <sdkports/debugoverlay_shared.h>
#include <sdkports/sdk_traces.h>
#include <entities/baseentity.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include "nav_mesh.h"
#include "nav_area.h"
#include "nav_volume.h"

unsigned int CNavVolume::s_nextID = 0;

CNavVolume::CNavVolume()
{
	m_id = s_nextID;
	s_nextID++;
	m_teamIndex = NAV_TEAM_ANY;
	std::fill(m_blockedCache.begin(), m_blockedCache.end(), false);
}

CNavVolume::~CNavVolume()
{
	for (auto& area : m_areas)
	{
		area->NotifyNavVolumeDestruction(this);
	}
}

void CNavVolume::SetOrigin(const Vector& origin)
{
	m_origin = origin;
	m_calculatedMins = (m_origin + m_mins);
	m_calculatedMaxs = (m_origin + m_maxs);
}

const Vector& CNavVolume::GetOrigin() const
{
	return m_origin;
}

void CNavVolume::SetBounds(const Vector& mins, const Vector& maxs)
{
	m_mins = mins;
	m_maxs = maxs;
	m_calculatedMins = (m_origin + m_mins);
	m_calculatedMaxs = (m_origin + m_maxs);
}

void CNavVolume::GetBounds(Vector& mins, Vector& maxs) const
{
	mins = m_mins;
	maxs = m_maxs;
}

bool CNavVolume::IntersectsWith(const CNavVolume* other) const
{
	if (this == other)
	{
		return false;
	}

	return UtilHelpers::AABBIntersectsAABB(this->m_calculatedMins, this->m_calculatedMaxs, other->m_calculatedMins, other->m_calculatedMaxs);
}

void CNavVolume::Update()
{
	if (m_toggle_condition.GetToggleConditionType() == navscripting::ToggleCondition::TCTypes::TYPE_NOT_SET)
	{
		return;
	}

	if (m_scanTimer.HasStarted())
	{
		if (m_scanTimer.IsElapsed())
		{
			m_scanTimer.Invalidate();
			SearchTargetEntity();
		}

		return;
	}

	if (m_toggle_condition.HasTargetEntityBeenSet())
	{
		CBaseEntity* pEntity = m_toggle_condition.GetTargetEntity();

		if (pEntity == nullptr)
		{
			UpdateBlockedStatus(NAV_TEAM_ANY, false);
			m_scanTimer.StartRandom(5.0f, 10.0f);
			return;
		}
	}

	bool result = m_toggle_condition.RunTestCondition();
	UpdateBlockedStatus(m_teamIndex, result);
}

void CNavVolume::Save(std::fstream& filestream, uint32_t version)
{
	filestream.write(reinterpret_cast<char*>(&m_id), sizeof(unsigned int));
	filestream.write(reinterpret_cast<char*>(&m_origin), sizeof(Vector));
	filestream.write(reinterpret_cast<char*>(&m_mins), sizeof(Vector));
	filestream.write(reinterpret_cast<char*>(&m_maxs), sizeof(Vector));
	filestream.write(reinterpret_cast<char*>(&m_teamIndex), sizeof(int));
	m_toggle_condition.Save(filestream, version);
}

NavErrorType CNavVolume::Load(std::fstream& filestream, uint32_t version, uint32_t subVersion)
{
	filestream.read(reinterpret_cast<char*>(&m_id), sizeof(unsigned int));
	filestream.read(reinterpret_cast<char*>(&m_origin), sizeof(Vector));
	filestream.read(reinterpret_cast<char*>(&m_mins), sizeof(Vector));
	filestream.read(reinterpret_cast<char*>(&m_maxs), sizeof(Vector));
	filestream.read(reinterpret_cast<char*>(&m_teamIndex), sizeof(int));
	m_toggle_condition.Load(filestream, version);

	return NAV_OK;
}

NavErrorType CNavVolume::PostLoad(void)
{
	m_calculatedMins = (m_origin + m_mins);
	m_calculatedMaxs = (m_origin + m_maxs);
	
	if (m_teamIndex >= static_cast<int>(NAV_TEAMS_ARRAY_SIZE))
	{
		smutils->LogError(myself, "Nav Volume #%i has invalid team index #%i! Limit is %i.\n", m_teamIndex, NAV_TEAMS_ARRAY_SIZE);
		m_teamIndex = NAV_TEAM_ANY;
	}

	SearchForNavAreas();

	return NAV_OK;
}

void CNavVolume::Draw() const
{
	if (TheNavMesh->GetSelectedVolume().get() == this)
	{
		NDebugOverlay::Box(m_origin, m_mins, m_maxs, 255, 255, 255, 100, NDEBUG_PERSIST_FOR_ONE_TICK);
		NDebugOverlay::Sphere(m_origin, 16.0f, 255, 0, 255, true, NDEBUG_PERSIST_FOR_ONE_TICK);

		CBaseEntity* pTargetEnt = GetTargetEntity();

		if (pTargetEnt)
		{
			NDebugOverlay::EntityBounds(pTargetEnt, 255, 255, 0, 160, NDEBUG_PERSIST_FOR_ONE_TICK);
			NDebugOverlay::EntityText(gamehelpers->EntityToBCompatRef(pTargetEnt), 0, "Target Entity", NDEBUG_PERSIST_FOR_ONE_TICK);
		}
	}
	else
	{
		NDebugOverlay::Box(m_origin, m_mins, m_maxs, 135, 206, 250, 100, NDEBUG_PERSIST_FOR_ONE_TICK);
		NDebugOverlay::Text(m_origin, false, NDEBUG_PERSIST_FOR_ONE_TICK, "Nav Volume #%i", m_id);
	}
}

void CNavVolume::DrawAreas() const
{
	for (auto& area : m_areas)
	{
		area->DrawFilled(0, 128, 0, 64, 10.0f);
	}
}

void CNavVolume::ScreenText() const
{
	NDebugOverlay::ScreenText(BASE_SCREENX, BASE_SCREENY, 255, 255, 0, 255, NDEBUG_PERSIST_FOR_ONE_TICK, "Selected Nav Volume #%i Team %i Blocked %s", 
		m_id, m_teamIndex, IsBlocked(m_teamIndex) ? "YES" : "NO");
	
	m_toggle_condition.DebugScreenOverlay(BASE_SCREENX, BASE_SCREENY + 0.04f, NDEBUG_PERSIST_FOR_ONE_TICK);
}

bool CNavVolume::IsBlocked(int teamID) const
{
	if (teamID < 0 || teamID >= static_cast<int>(m_blockedCache.size()))
	{
		for (auto& i : m_blockedCache)
		{
			if (i == true)
			{
				return true;
			}
		}

		return false;
	}

	return m_blockedCache[teamID];
}

void CNavVolume::SearchForNavAreas()
{
	for (auto& area : m_areas)
	{
		area->NotifyNavVolumeDestruction(this);
	}

	m_areas.clear();
	FindAreasInVolume search(this);
	CNavMesh::ForAllAreas(search);

	if (m_areas.empty())
	{
		Warning("Nav Volume #%i: No areas found inside bounds!\n", m_id);
	}

	for (auto& area : m_areas)
	{
		area->SetNavVolume(this);
	}
}

bool CNavVolume::FindAreasInVolume::operator()(CNavArea* area)
{
	Vector point = area->GetCenter();
	point.z += navgenparams->step_height * 0.5f;

	if (UtilHelpers::PointIsInsideAABB(point, volume->m_calculatedMins, volume->m_calculatedMaxs))
	{
		volume->m_areas.push_back(area);
	}

	return true; // return true to keep looping areas
}
