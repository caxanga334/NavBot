#include <string_view>
#include <extension.h>
#include <sdkports/debugoverlay_shared.h>
#include <sdkports/sdk_traces.h>
#include <entities/baseentity.h>
#include <util/helpers.h>
#include "nav_mesh.h"
#include "nav_area.h"
#include "nav_volume.h"

unsigned int CNavVolume::s_nextID = 0;

CNavVolume::CNavVolume() : m_entOrigin(0.0f, 0.0f, 0.0f)
{
	m_id = s_nextID;
	s_nextID++;
	m_teamIndex = NAV_TEAM_ANY;
	std::fill(m_blockedCache.begin(), m_blockedCache.end(), false);
	m_entTargetname.reserve(2);
	m_entClassname.reserve(2);
	m_blockedConditionType = CONDITION_NONE;
	m_inverted = false;
}

CNavVolume::~CNavVolume()
{
	for (auto& area : m_areas)
	{
		area->NotifyNavVolumeDestruction(this);
	}
}

const char* CNavVolume::GetNameOfConditionType(ConditionType type)
{
	using namespace std::literals::string_view_literals;

	constexpr std::array keyNames = {
		"CONDITION_NONE"sv,
		"CONDITION_HULL_COLLISION"sv,
		"CONDITION_ENTITY_EXISTS"sv,
		"CONDITION_ENTITY_ENABLED"sv,
		"CONDITION_ENTITY_LOCKED"sv,
		"CONDITION_DOOR_CLOSED"sv,
	};

	static_assert(keyNames.size() == static_cast<size_t>(CNavVolume::ConditionType::MAX_CONDITION_TYPES), "ConditionType enum and keyNames array size mismatch!");

	return keyNames[static_cast<size_t>(type)].data();
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

void CNavVolume::SetTargetEntity(CBaseEntity* entity)
{
	m_targetEnt = entity;

	if (entity != nullptr)
	{
		entities::HBaseEntity baseent(entity);

		std::unique_ptr<char[]> targetname = std::make_unique<char[]>(256);

		baseent.GetTargetName(targetname.get(), 256);

		int index = UtilHelpers::FindEntityByTargetname(INVALID_EHANDLE_INDEX, targetname.get());

		// in case we get an invalid targetname
		if (index == INVALID_EHANDLE_INDEX)
		{
			m_entTargetname.clear();
		}
		else
		{
			m_entTargetname.assign(targetname.get());
		}
		
		m_entOrigin = baseent.WorldSpaceCenter(); // use world center in case this is a brush entity
		m_entClassname.assign(gamehelpers->GetEntityClassname(entity));
	}
	else
	{
		m_entTargetname.clear();
		m_entClassname.clear();
		m_entOrigin.Init(0.0f, 0.0f, 0.0f);
	}
}

CBaseEntity* CNavVolume::GetTargetEntity() const
{
	return m_targetEnt.Get();
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
	if (m_blockedConditionType == CONDITION_NONE)
	{
		return;
	}

	if (m_scanTimer.HasStarted())
	{
		if (m_scanTimer.IsElapsed())
		{
			m_scanTimer.Invalidate();
			SearchTargetEntity(false);
		}

		return;
	}

	if (!m_entClassname.empty())
	{
		CBaseEntity* pEntity = m_targetEnt.Get();

		if (pEntity == nullptr)
		{
			UpdateBlockedStatus(NAV_TEAM_ANY, false);
			m_scanTimer.StartRandom(5.0f, 10.0f);
			return;
		}
	}

	if (m_blockedConditionType == CONDITION_HULL_COLLISION)
	{
		UpdateCondition_HullCollision();
		return;
	}

	switch (m_blockedConditionType)
	{
	case CNavVolume::CONDITION_NONE:
		return;
	case CNavVolume::CONDITION_HULL_COLLISION:
		UpdateCondition_HullCollision();
		break;
	case CNavVolume::CONDITION_ENTITY_EXISTS:
		UpdateCondition_EntityExists();
		break;
	case CNavVolume::CONDITION_ENTITY_ENABLED:
		UpdateCondition_EntityEnabled();
		break;
	case CNavVolume::CONDITION_ENTITY_LOCKED:
		break;
	case CNavVolume::CONDITION_DOOR_CLOSED:
		break;
	case CNavVolume::MAX_CONDITION_TYPES:
		break;
	default:
#ifdef EXT_DEBUG
		Warning("CNavVolume::Update Condition check for type %i not implemented!\n", static_cast<int>(m_blockedConditionType));
#endif // EXT_DEBUG
		break;
	}
}

void CNavVolume::Save(std::fstream& filestream, uint32_t version)
{
	filestream.write(reinterpret_cast<char*>(&m_id), sizeof(unsigned int));
	filestream.write(reinterpret_cast<char*>(&m_origin), sizeof(Vector));
	filestream.write(reinterpret_cast<char*>(&m_mins), sizeof(Vector));
	filestream.write(reinterpret_cast<char*>(&m_maxs), sizeof(Vector));
	filestream.write(reinterpret_cast<char*>(&m_teamIndex), sizeof(int));
	filestream.write(reinterpret_cast<char*>(&m_blockedConditionType), sizeof(ConditionType));
	filestream.write(reinterpret_cast<char*>(&m_inverted), sizeof(bool));

	bool hasent = !m_entClassname.empty();
	filestream.write(reinterpret_cast<char*>(&hasent), sizeof(bool));

	if (hasent)
	{
		filestream.write(reinterpret_cast<char*>(&m_entOrigin), sizeof(Vector));
		std::uint64_t size = static_cast<std::uint64_t>(m_entClassname.size()) + 1;
		filestream.write(reinterpret_cast<char*>(&size), sizeof(std::uint64_t));
		filestream.write(m_entClassname.c_str(), size);

		bool hastargetname = !m_entTargetname.empty();
		filestream.write(reinterpret_cast<char*>(&hastargetname), sizeof(bool));

		if (hastargetname)
		{
			size = static_cast<std::uint64_t>(m_entTargetname.size()) + 1;
			filestream.write(reinterpret_cast<char*>(&size), sizeof(std::uint64_t));
			filestream.write(m_entTargetname.c_str(), size);
		}
	}
	else
	{
		if (m_blockedConditionType != CONDITION_NONE && m_blockedConditionType != CONDITION_HULL_COLLISION)
		{
			smutils->LogError(myself, "Nav Volume #%i saved without a target entity!", m_id);
		}
	}

	if (m_blockedConditionType == CONDITION_NONE)
	{
		smutils->LogError(myself, "Nav Volume #%i saved without a blocked condition type!", m_id);
	}
}

NavErrorType CNavVolume::Load(std::fstream& filestream, uint32_t version, uint32_t subVersion)
{
	filestream.read(reinterpret_cast<char*>(&m_id), sizeof(unsigned int));
	filestream.read(reinterpret_cast<char*>(&m_origin), sizeof(Vector));
	filestream.read(reinterpret_cast<char*>(&m_mins), sizeof(Vector));
	filestream.read(reinterpret_cast<char*>(&m_maxs), sizeof(Vector));
	filestream.read(reinterpret_cast<char*>(&m_teamIndex), sizeof(int));
	filestream.read(reinterpret_cast<char*>(&m_blockedConditionType), sizeof(ConditionType));
	filestream.read(reinterpret_cast<char*>(&m_inverted), sizeof(bool));

	bool hasent = false;
	filestream.read(reinterpret_cast<char*>(&hasent), sizeof(bool));

	if (hasent)
	{
		filestream.read(reinterpret_cast<char*>(&m_entOrigin), sizeof(Vector));
		std::uint64_t size = 0;
		filestream.read(reinterpret_cast<char*>(&size), sizeof(std::uint64_t));
		std::unique_ptr<char[]> classname = std::make_unique<char[]>(size + 2);
		filestream.read(classname.get(), size);
		m_entClassname.assign(classname.get());

		bool hastargetname = false;
		filestream.read(reinterpret_cast<char*>(&hastargetname), sizeof(bool));

		if (hastargetname)
		{
			size = 0;
			filestream.read(reinterpret_cast<char*>(&size), sizeof(std::uint64_t));
			std::unique_ptr<char[]> targetname = std::make_unique<char[]>(size + 2);
			filestream.read(targetname.get(), size);
			m_entTargetname.assign(targetname.get());
		}

	}

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
	SearchTargetEntity(true);

	return NAV_OK;
}

void CNavVolume::Draw() const
{
	if (TheNavMesh->GetSelectedVolume().get() == this)
	{
		NDebugOverlay::Box(m_origin, m_mins, m_maxs, 255, 255, 255, 100, NDEBUG_PERSIST_FOR_ONE_TICK);
		NDebugOverlay::Sphere(m_origin, 16.0f, 255, 0, 255, true, NDEBUG_PERSIST_FOR_ONE_TICK);
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
	NDebugOverlay::ScreenText(BASE_SCREENX, BASE_SCREENY, 255, 255, 0, 255, NDEBUG_PERSIST_FOR_ONE_TICK, "Selected Nav Volume #%i Team %i", m_id, m_teamIndex);
	NDebugOverlay::ScreenText(BASE_SCREENX, BASE_SCREENY + 0.04f, 255, 255, 0, 255, NDEBUG_PERSIST_FOR_ONE_TICK, "%s Inverted %s", GetNameOfConditionType(m_blockedConditionType),
		m_inverted ? "YES" : "NO");

	if (!m_entClassname.empty())
	{
		int index = gamehelpers->EntityToBCompatRef(m_targetEnt.Get());
		NDebugOverlay::ScreenText(BASE_SCREENX, BASE_SCREENY + 0.08f, 255, 255, 0, 255, NDEBUG_PERSIST_FOR_ONE_TICK, "Target Entity: #%i Class Name: %s Target Name: %s", 
			index, m_entClassname.c_str(), !m_entTargetname.empty() ? m_entTargetname.c_str() : "");
	}
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

void CNavVolume::SearchTargetEntity(bool errorIfNotFound)
{
	if (!m_entTargetname.empty())
	{
		CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(UtilHelpers::FindEntityByTargetname(INVALID_EHANDLE_INDEX, m_entTargetname.c_str()));

		if (errorIfNotFound && pEntity == nullptr)
		{
			smutils->LogError(myself, "Nav Volume #%i failed to find target entity with targetname \"%s\"!", m_id, m_entTargetname.c_str());
		}

		m_targetEnt = pEntity;
	}
	else if (!m_entClassname.empty())
	{
		CBaseEntity* pEntity = nullptr;
		const Vector& start = m_entOrigin;
		float best = 99999999.0f;

		UtilHelpers::ForEachEntityOfClassname(m_entClassname.c_str(), [&pEntity, &start, &best](int index, edict_t* edict, CBaseEntity* entity) {
			if (entity != nullptr)
			{
				entities::HBaseEntity be(entity);
				Vector end = be.WorldSpaceCenter();
				float distance = (start - end).Length();

				if (distance < best)
				{
					best = distance;
					pEntity = entity;
				}

			}

			return true;
		});

		if (errorIfNotFound && pEntity == nullptr)
		{
			smutils->LogError(myself, "Nav Volume #%i failed to find target entity with classname \"%s\"!", m_id, m_entClassname.c_str());
		}

		m_targetEnt = pEntity;
	}
}

void CNavVolume::UpdateCondition_HullCollision()
{
	CTraceFilterWorldAndPropsOnly filter;
	trace_t tr;
	trace::hull(m_origin, m_origin, m_mins, m_maxs, MASK_PLAYERSOLID, &filter, tr);

	bool blocked = m_inverted ? !tr.DidHit() : tr.DidHit();
	UpdateBlockedStatus(m_teamIndex, blocked);
}

void CNavVolume::UpdateCondition_EntityExists()
{
	bool exists = m_targetEnt.Get() != nullptr;

	if (m_inverted)
	{
		exists = !exists;
	}

	UpdateBlockedStatus(m_teamIndex, exists);
}

void CNavVolume::UpdateCondition_EntityEnabled()
{
	CBaseEntity* pEntity = m_targetEnt.Get();

	if (pEntity != nullptr)
	{
		entities::HBaseEntity be(pEntity);

		bool enabled = !be.IsDisabled();

		if (m_inverted)
		{
			enabled = !enabled;
		}

		UpdateBlockedStatus(m_teamIndex, enabled);
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
