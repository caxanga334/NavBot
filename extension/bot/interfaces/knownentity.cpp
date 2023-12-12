#include <extension.h>
#include <ifaces_extern.h>
#include <extplayer.h>
#include <bot/interfaces/base_interface.h>
#include <bot/interfaces/knownentity.h>
#include <bot/basebot.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_area.h>
#include "knownentity.h"

CKnownEntity::CKnownEntity(edict_t* entity)
{
	m_timeknown = gpGlobals->curtime;
	m_timelastvisible = -9999.0f;
	m_timelastinfo = -9999.0f;
	m_volume = 0;
	gamehelpers->SetHandleEntity(m_handle, entity);
	UpdatePosition();
}

CKnownEntity::CKnownEntity(int entity)
{
	m_timeknown = gpGlobals->curtime;
	m_timelastvisible = -9999.0f;
	m_timelastinfo = -9999.0f;
	m_volume = 0;
	auto edict = gamehelpers->EdictOfIndex(entity);
	gamehelpers->SetHandleEntity(m_handle, edict);
	UpdatePosition();
}

CKnownEntity::~CKnownEntity()
{
}

bool CKnownEntity::operator==(const CKnownEntity& other)
{
	return m_handle == other.m_handle;
}

float CKnownEntity::GetTimeSinceBecomeKnown() const { return gpGlobals->curtime - m_timeknown; }

float CKnownEntity::GetTimeSinceLastVisible() const { return gpGlobals->curtime - m_timelastvisible; }

float CKnownEntity::GetTimeSinceLastInfo() const { return gpGlobals->curtime - m_timelastinfo; }

bool CKnownEntity::IsObsolete()
{
	return !m_handle.IsValid() || gamehelpers->GetHandleEntity(m_handle) == nullptr || GetTimeSinceLastInfo() > NKnownEntity::TIME_FOR_OBSOLETE;
}

bool CKnownEntity::IsValid()
{
	return m_handle.IsValid() == true && gamehelpers->GetHandleEntity(m_handle) != nullptr;
}

void CKnownEntity::UpdatePosition()
{
	constexpr auto NAV_AREA_DIST = 128.0f;

	auto entity = gamehelpers->GetHandleEntity(m_handle);

	if (entity)
	{
		m_timelastinfo = gpGlobals->curtime;
		auto pos = UtilHelpers::getWorldSpaceCenter(entity);
		m_lastknownposition = pos;
		m_lastknownarea = TheNavMesh->GetNearestNavArea(pos, NAV_AREA_DIST);
	}
}

void CKnownEntity::UpdatePosition(const Vector& newPos)
{
	constexpr auto NAV_AREA_DIST = 128.0f;

	m_timelastinfo = gpGlobals->curtime;
	m_lastknownposition = newPos;
	m_lastknownarea = TheNavMesh->GetNearestNavArea(newPos, NAV_AREA_DIST);
}

void CKnownEntity::MarkAsFullyVisible()
{
	m_timelastvisible = gpGlobals->curtime;
	m_visible = true;
}

bool CKnownEntity::IsEntity(edict_t* entity)
{
	auto me = gamehelpers->GetHandleEntity(m_handle);
	return me == entity;
}

bool CKnownEntity::IsEntity(const int entity)
{
	return entity == m_handle.GetEntryIndex();
}

edict_t* CKnownEntity::GetEdict()
{
	return gamehelpers->GetHandleEntity(m_handle);
}
