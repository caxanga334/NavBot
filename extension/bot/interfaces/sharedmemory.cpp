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

ISharedBotMemory::EntityInfo::EntityInfo(CBaseEntity* pEntity) :
	m_handle(pEntity)
{
	m_lastknownposition = UtilHelpers::getEntityOrigin(pEntity);
	m_lastknownarea = TheNavMesh->GetNearestNavArea(m_lastknownposition, 256.0f, true);
	m_timecreated = gpGlobals->curtime;
	m_timeupdated = gpGlobals->curtime;
	m_classname.assign(gamehelpers->GetEntityClassname(pEntity));
}

bool ISharedBotMemory::EntityInfo::operator==(const ISharedBotMemory::EntityInfo& other) const
{
	return this->m_handle == other.m_handle;
}

bool ISharedBotMemory::EntityInfo::operator!=(const ISharedBotMemory::EntityInfo& other) const
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
	m_lastknownarea = TheNavMesh->GetNearestNavArea(m_lastknownposition, CPath::PATH_GOAL_MAX_DISTANCE_TO_AREA * 2.5f, true);
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

bool ISharedBotMemory::EntityInfo::ClassnameMatches(const char* pattern) const
{
	return UtilHelpers::StringMatchesPattern(m_classname.c_str(), pattern, 0);
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
	PurgeInvalidEntityInfos();
}

void ISharedBotMemory::Frame()
{
}

void ISharedBotMemory::OnRoundRestart()
{
	Reset();
}

const ISharedBotMemory::EntityInfo* ISharedBotMemory::AddEntityInfo(CBaseEntity* entity)
{
	for (ISharedBotMemory::EntityInfo& info : m_ents)
	{
		if (info.GetEntity() == entity)
		{
			info.Update();
			return &info;
		}
	}

	ISharedBotMemory::EntityInfo* info = &m_ents.emplace_back(entity);
	return info;
}

const ISharedBotMemory::EntityInfo* ISharedBotMemory::GetEntityInfo(CBaseEntity* entity) const
{
	ISharedBotMemory::EntityInfo other{ entity };

	auto it = std::find(m_ents.cbegin(), m_ents.cend(), other);

	if (it != m_ents.cend())
	{
		return &(*it);
	}

	return nullptr;
}

void ISharedBotMemory::PurgeInvalidEntityInfos()
{
	if (m_ents.empty()) { return; }

	m_ents.erase(std::remove_if(m_ents.begin(), m_ents.end(), [](ISharedBotMemory::EntityInfo& info) {
		return info.IsObsolete();
	}), m_ents.end());
}
