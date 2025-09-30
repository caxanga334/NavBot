#include NAVBOT_PCH_FILE
#include <extension.h>
#include <bot/interfaces/base_interface.h>
#include <bot/basebot.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_area.h>
#include <util/helpers.h>
#include <util/sdkcalls.h>
#include <entities/baseentity.h>
#include "knownentity.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

CKnownEntity::CKnownEntity(edict_t* entity) :
	m_baseent(entity)
{
	m_player = nullptr;
	gamehelpers->SetHandleEntity(m_handle, entity);
	Init();
	UpdatePosition();
}

CKnownEntity::CKnownEntity(int entity) :
	m_baseent(gamehelpers->EdictOfIndex(entity))
{
	m_player = nullptr;
	auto edict = gamehelpers->EdictOfIndex(entity);
	gamehelpers->SetHandleEntity(m_handle, edict);
	Init();
	UpdatePosition();
}

CKnownEntity::CKnownEntity(CBaseEntity* entity) :
	m_baseent(entity)
{
	m_player = nullptr;
	m_handle.Set(entity);
	Init();
	UpdatePosition();
}

CKnownEntity::~CKnownEntity()
{
	if (m_player)
	{
		delete m_player;
		m_player = nullptr;
	}
}

void CKnownEntity::Init()
{
	m_classname = gamehelpers->GetEntityClassname(m_handle.Get());
	m_timeknown = gpGlobals->curtime;
	m_timelastvisible = -9999.0f;
	m_timelastinfo = -9999.0f;
	m_timesincelastnoise = -9999.0f;
	m_visible = false;
	m_lkpwasseen = false;

	if (UtilHelpers::IsPlayerIndex(m_handle.GetEntryIndex()))
	{
		m_player = new CBaseExtPlayer(UtilHelpers::BaseEntityToEdict(m_handle.Get()));
	}
}

bool CKnownEntity::operator==(const CKnownEntity& other)
{
	return m_handle == other.m_handle;
}

bool CKnownEntity::operator==(const CKnownEntity* other)
{
	return this->m_handle == other->m_handle;
}

float CKnownEntity::GetTimeSinceBecomeKnown() const { return gpGlobals->curtime - m_timeknown; }

float CKnownEntity::GetTimeSinceLastVisible() const { return gpGlobals->curtime - m_timelastvisible; }

float CKnownEntity::GetTimeSinceLastInfo() const { return gpGlobals->curtime - m_timelastinfo; }

float CKnownEntity::GetTimeSinceLastHeard() const { return gpGlobals->curtime - m_timesincelastnoise; }

bool CKnownEntity::IsObsolete() const
{
	return !m_handle.IsValid() || m_handle.Get() == nullptr || GetTimeSinceLastInfo() > time_to_become_obsolete() || !UtilHelpers::IsEntityAlive(m_handle.Get());
}

bool CKnownEntity::IsValid() const
{
	return m_handle.IsValid() == true && m_handle.Get() != nullptr;
}

void CKnownEntity::UpdatePosition()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CKnownEntity::UpdatePosition", "NavBot");
#endif // EXT_VPROF_ENABLED

	constexpr auto NAV_AREA_DIST = 512.0f;

	CBaseEntity* pEntity = m_handle.Get();

	if (pEntity != nullptr)
	{
		entities::HBaseEntity be(pEntity);
		m_timelastinfo = gpGlobals->curtime;
		m_lastknownposition = UtilHelpers::getEntityOrigin(pEntity);
		m_lastknownvelocity = be.GetAbsVelocity();
		CNavArea* newArea = TheNavMesh->GetNearestNavArea(m_lastknownposition, NAV_AREA_DIST);

		if (newArea != nullptr)
		{
			m_lastknownarea = newArea;
		}
	}
}

void CKnownEntity::UpdateHeard()
{
	UpdatePosition();
	m_timesincelastnoise = gpGlobals->curtime;
}

void CKnownEntity::MarkAsFullyVisible()
{
	m_timelastvisible = gpGlobals->curtime;
	m_visible = true;
}

bool CKnownEntity::IsEntity(edict_t* entity) const
{
	auto me = UtilHelpers::GetEdictFromCBaseHandle(m_handle);
	return me == entity;
}

bool CKnownEntity::IsEntity(const int entity) const
{
	return entity == m_handle.GetEntryIndex();
}

void CKnownEntity::DebugDraw(const float duration) const
{
	static Vector box_mins{ -16.0f, -16.0f, -16.0f };
	static Vector box_maxs{ 16.0f, 16.0f, 16.0f };
	static Vector text_height_offset{ 0.0f, 0.0f, 32.0f };

	int r, g, b;

	if (IsVisibleNow())
	{
		r = 0;
		g = 180;
		b = 0;
	}
	else
	{
		r = 255;
		g = 0;
		b = 0;
	}

	NDebugOverlay::Box(m_lastknownposition, box_mins, box_maxs, r, g, b, 127, duration);
	NDebugOverlay::Text(m_lastknownposition + text_height_offset, true, duration, "#%i <%s> TSK: %g TSV: %g TSI: %g TSH: %g", 
		m_handle.GetEntryIndex(), m_classname.c_str(), GetTimeSinceBecomeKnown(), GetTimeSinceLastVisible(), GetTimeSinceLastInfo(), 
		GetTimeSinceLastHeard());
}

const bool CKnownEntity::IsEntityOfClassname(const char* classname) const
{
	return UtilHelpers::StringMatchesPattern(m_classname.c_str(), classname, 0);
}
