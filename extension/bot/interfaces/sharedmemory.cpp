#include NAVBOT_PCH_FILE
#include <extension.h>
#include <manager.h>
#include <mods/basemod.h>
#include <util/helpers.h>
#include <bot/basebot.h>
#include <bot/interfaces/path/basepath.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_area.h>
#include "sharedmemory.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

ISharedBotMemory::ReportedEntityData::ReportedEntityData(CBaseEntity* pEntity) :
	m_handle(pEntity)
{
	m_lastknownposition = UtilHelpers::getEntityOrigin(pEntity);
	m_lastknownarea = TheNavMesh->GetNearestNavArea(m_lastknownposition, 256.0f, true);
	m_timecreated = gpGlobals->curtime;
	m_timeupdated = gpGlobals->curtime;
	m_classname.assign(gamehelpers->GetEntityClassname(pEntity));
	m_cleared = false;
}

bool ISharedBotMemory::ReportedEntityData::operator==(const ISharedBotMemory::ReportedEntityData& other) const
{
	return this->m_handle == other.m_handle;
}

bool ISharedBotMemory::ReportedEntityData::operator!=(const ISharedBotMemory::ReportedEntityData& other) const
{
	return this->m_handle != other.m_handle;
}

bool ISharedBotMemory::ReportedEntityData::IsObsolete() const
{
	return m_handle.Get() == nullptr || GetTimeSinceLastUpdated() >= ISharedBotMemory::REPORTED_ENTITY_BECOME_OBSOLETE_AFTER;
}

void ISharedBotMemory::ReportedEntityData::Update()
{
	CBaseEntity* pEntity = m_handle.Get();
	m_lastknownposition = UtilHelpers::getEntityOrigin(pEntity);
	m_lastknownarea = TheNavMesh->GetNearestNavArea(m_lastknownposition, CPath::PATH_GOAL_MAX_DISTANCE_TO_AREA * 2.5f, true);
	m_timeupdated = gpGlobals->curtime;
	m_cleared = false;
}

float ISharedBotMemory::ReportedEntityData::GetTimeSinceCreation() const
{
	return gpGlobals->curtime - m_timecreated;
}

float ISharedBotMemory::ReportedEntityData::GetTimeSinceLastUpdated() const
{
	return gpGlobals->curtime - m_timeupdated;
}

bool ISharedBotMemory::ReportedEntityData::ClassnameMatches(const char* pattern) const
{
	return UtilHelpers::StringMatchesPattern(m_classname.c_str(), pattern, 0);
}

ISharedBotMemory::ISharedBotMemory()
{
	m_defenders = 0;
	m_reportedentitiesvec.reserve(32);
}

ISharedBotMemory::~ISharedBotMemory()
{
}

void ISharedBotMemory::Reset()
{
	m_defenders = 0;
	m_reportedentitiesvec.clear();
}

void ISharedBotMemory::Update()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISharedBotMemory::Update", "NavBot");
#endif // EXT_VPROF_ENABLED

	UpdateReportedEntities();
}

void ISharedBotMemory::Frame()
{
}
