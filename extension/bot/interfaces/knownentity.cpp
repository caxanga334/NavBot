#include <extension.h>
#include <bot/interfaces/base_interface.h>
#include <bot/basebot.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_area.h>
#include <util/helpers.h>
#include <util/sdkcalls.h>
#include <entities/baseentity.h>
#include "knownentity.h"

CKnownEntity::CKnownEntity(edict_t* entity)
{
	gamehelpers->SetHandleEntity(m_handle, entity);
	Init();
	UpdatePosition();
}

CKnownEntity::CKnownEntity(int entity)
{
	
	auto edict = gamehelpers->EdictOfIndex(entity);
	gamehelpers->SetHandleEntity(m_handle, edict);
	Init();
	UpdatePosition();
}

CKnownEntity::CKnownEntity(CBaseEntity* entity)
{
	m_handle.Set(entity);
	Init();
	UpdatePosition();
}

CKnownEntity::~CKnownEntity()
{
}

void CKnownEntity::Init()
{
	m_timeknown = gpGlobals->curtime;
	m_timelastvisible = -9999.0f;
	m_timelastinfo = -9999.0f;
	m_timesincelastnoise = -9999.0f;
	m_visible = false;
	m_lkpwasseen = false;
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
	constexpr auto NAV_AREA_DIST = 512.0f;

	CBaseEntity* pEntity = m_handle.Get();

	if (pEntity != nullptr)
	{
		entities::HBaseEntity be(pEntity);
		m_timelastinfo = gpGlobals->curtime;
		m_lastknownposition = be.GetAbsOrigin();
		m_lastknownvelocity = be.GetAbsVelocity();
		m_lastknownarea = TheNavMesh->GetNearestNavArea(m_lastknownposition, NAV_AREA_DIST);
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

edict_t* CKnownEntity::GetEdict() const
{
	return m_handle.ToEdict();
}

CBaseEntity* CKnownEntity::GetEntity() const
{
	return m_handle.Get();
}

int CKnownEntity::GetIndex() const
{
	return m_handle.GetEntryIndex();
}

bool CKnownEntity::IsPlayer() const
{
	return UtilHelpers::IsPlayerIndex(m_handle.GetEntryIndex());
}
