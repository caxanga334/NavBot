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
	m_timelastvisible = -1.0f;
	m_timelastinfo = -1.0f;
	m_volume = 0;
	gamehelpers->SetHandleEntity(m_handle, entity);
}

CKnownEntity::CKnownEntity(int entity)
{
	m_timeknown = gpGlobals->curtime;
	m_timelastvisible = -1.0f;
	m_timelastinfo = -1.0f;
	m_volume = 0;
	auto edict = gamehelpers->EdictOfIndex(entity);
	gamehelpers->SetHandleEntity(m_handle, edict);
}

CKnownEntity::~CKnownEntity()
{
}

bool CKnownEntity::operator==(const CKnownEntity& other)
{
	return m_handle == other.m_handle;
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
