#include NAVBOT_PCH_FILE
#include <extension.h>
#include <manager.h>
#include <mods/basemod.h>
#include <util/helpers.h>
#include <bot/basebot.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_area.h>
#include "sharedmemory.h"

ISharedBotMemory::EntityInfo::EntityInfo(CBaseEntity* pEntity) :
	m_handle(pEntity)
{
	m_lastknownposition = UtilHelpers::getEntityOrigin(pEntity);
	m_lastknownarea = TheNavMesh->GetNearestNavArea(m_lastknownposition, 256.0f, true);
	m_timecreated = gpGlobals->curtime;
	m_timeupdated = gpGlobals->curtime;
}

bool ISharedBotMemory::EntityInfo::operator==(const EntityInfo& other)
{
	return this->m_handle == other.m_handle;
}

bool ISharedBotMemory::EntityInfo::operator!=(const EntityInfo& other)
{
	return this->m_handle != other.m_handle;
}

bool ISharedBotMemory::EntityInfo::IsObsolete()
{
	return m_handle.Get() == nullptr || GetTimeSinceLastUpdated() >= ISharedBotMemory::ENTITY_INFO_EXPIRE_TIME;
}

void ISharedBotMemory::EntityInfo::Update()
{
	CBaseEntity* pEntity = m_handle.Get();
	m_lastknownposition = UtilHelpers::getEntityOrigin(pEntity);
	m_lastknownarea = TheNavMesh->GetNearestNavArea(m_lastknownposition, 256.0f, true);
	m_timeupdated = gpGlobals->curtime;
}

float ISharedBotMemory::EntityInfo::GetTimeSinceCreation() const
{
	return gpGlobals->curtime - m_timecreated;
}

float ISharedBotMemory::EntityInfo::GetTimeSinceLastUpdated() const
{
	return gpGlobals->curtime - m_timeupdated;
}

ISharedBotMemory::ISharedBotMemory()
{
}

ISharedBotMemory::~ISharedBotMemory()
{
}

void ISharedBotMemory::Reset()
{
}

void ISharedBotMemory::Update()
{
}

void ISharedBotMemory::Frame()
{
}

void ISharedBotMemory::OnRoundRestart()
{
	Reset();
}
