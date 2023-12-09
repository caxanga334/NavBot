#include <extension.h>
#include <bot/basebot.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_area.h>
#include <ifaces_extern.h>
#include <util/helpers.h>
#include "sensor.h"

BotSensorTraceFilter::BotSensorTraceFilter(int collisionGroup) :
	CTraceFilterSimple(nullptr, collisionGroup, nullptr)
{
}

bool BotSensorTraceFilter::ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask)
{
	auto entity = entityFromEntityHandle(pHandleEntity);

	if (entity == nullptr)
	{
		return false;
	}
	
	const auto index = gamehelpers->IndexOfEdict(entity);
	const bool isplayer = UtilHelpers::IsPlayerIndex(index);

	if (isplayer)
	{
		return false; // Don't hit players
	}

	return CTraceFilterSimple::ShouldHitEntity(pHandleEntity, contentsMask);
}

ISensor::ISensor(CBaseBot* bot) : IBotInterface(bot)
{
	m_knownlist.reserve(256);
	auto& profile = bot->GetDifficultyProfile();

	SetFieldOfView(profile.GetFOV());
	m_maxvisionrange = static_cast<float>(profile.GetMaxVisionRange());
	m_maxhearingrange = static_cast<float>(profile.GetMaxHearingRange());
}

ISensor::~ISensor()
{
}

void ISensor::OnDifficultyProfileChanged()
{
	auto& profile = GetBot()->GetDifficultyProfile();

	SetFieldOfView(profile.GetFOV());
	m_maxvisionrange = static_cast<float>(profile.GetMaxVisionRange());
	m_maxhearingrange = static_cast<float>(profile.GetMaxHearingRange());
}

void ISensor::Reset()
{
	m_knownlist.clear();
}

void ISensor::Update()
{
	UpdateKnownEntities();
}

void ISensor::Frame()
{
}

bool ISensor::IsAbleToSee(edict_t* entity, const bool checkFOV)
{
	auto me = GetBot();
	auto start = me->GetEyeOrigin();
	auto pos = UtilHelpers::getWorldSpaceCenter(entity);
	const auto maxdist = GetMaxVisionRange() * GetMaxVisionRange();
	float distance = (start - pos).LengthSqr();

	if (distance > maxdist)
	{
		return false;
	}

	// Check FOV
	if (checkFOV)
	{
		if (!IsLineOfSightClear(pos))
		{
			return false;
		}
	}

	// Test for smoke, fog, ...
	if (IsEntityHidden(entity))
	{
		return false;
	}

	return true;
}

/**
 * @brief Expensive function that checks if the bot is able to see a given entity, testing for vision blockers, conditions, smokes, etc
 * @param player Player to test for visibility
 * @param checkFOV if true, also check if the given player is in the bot field of view
 * @return true if visible, false otherwise
*/
bool ISensor::IsAbleToSee(CBaseExtPlayer& player, const bool checkFOV)
{
	auto me = GetBot();
	auto start = me->GetEyeOrigin();
	auto pos = player.WorldSpaceCenter();
	const auto maxdist = GetMaxVisionRange() * GetMaxVisionRange();
	float distance = (start - pos).LengthSqr();

	if (distance > maxdist)
	{
		return false;
	}

	// Check FOV
	if (checkFOV)
	{
		if (!IsLineOfSightClear(player))
		{
			return false;
		}
	}

	// Test for smoke, fog, ...
	if (IsEntityHidden(player.GetEdict()))
	{
		return false;
	}

	return true;
}

bool ISensor::IsAbleToHear(edict_t* entity)
{
	// Bots only hear other players by default
	const auto index = gamehelpers->IndexOfEdict(entity);
	return UtilHelpers::IsPlayerIndex(index);
}

bool ISensor::IsLineOfSightClear(const Vector& pos)
{
	auto start = GetBot()->GetEyeOrigin();

	BotSensorTraceFilter filter(COLLISION_GROUP_NONE);
	trace_t result;
	UTIL_TraceLine(start, pos, MASK_BLOCKLOS | CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, &result);

	return result.fraction >= 1.0f && !result.startsolid;
}

bool ISensor::IsLineOfSightClear(CBaseExtPlayer& player)
{
	auto start = GetBot()->GetEyeOrigin();
	BotSensorTraceFilter filter(COLLISION_GROUP_NONE);
	trace_t result;

	UTIL_TraceLine(start, player.GetEyeOrigin(), MASK_BLOCKLOS | CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, &result);

	if (result.DidHit())
	{
		UTIL_TraceLine(start, player.WorldSpaceCenter(), MASK_BLOCKLOS | CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, &result);

		if (result.DidHit())
		{
			UTIL_TraceLine(start, player.GetAbsOrigin(), MASK_BLOCKLOS | CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, &result);
		}
	}

	return result.fraction >= 1.0f && !result.startsolid;
}

bool ISensor::IsInFieldOfView(const Vector& pos)
{
	Vector forward;
	GetBot()->EyeVectors(&forward);
	return UtilHelpers::PointWithinViewAngle(GetBot()->GetEyeOrigin(), pos, forward, m_coshalfFOV);
}

/**
 * @brief Adds the given entity to the known entity list
 * @param entity Entity to be added to the list
 * @return true if the entity was added or already exists, false if it was not possible to add
*/
bool ISensor::AddKnownEntity(edict_t* entity)
{
	auto index = gamehelpers->IndexOfEdict(entity);

	if (index == 0) // index 0 is the world entity
	{
		return false;
	}

	CKnownEntity other(entity);

	if (std::find(m_knownlist.begin(), m_knownlist.end(), other) != m_knownlist.end())
	{
		return true; // Entity is already in the list
	}

	m_knownlist.push_back(other);

	return true;
}

// Removes a entity from the known list
void ISensor::ForgetKnownEntity(edict_t* entity)
{
	CKnownEntity other(entity);

	m_knownlist.erase(std::remove(m_knownlist.begin(), m_knownlist.end(), other), m_knownlist.end());
}

void ISensor::ForgetAllKnownEntities()
{
	m_knownlist.clear();
}

bool ISensor::IsKnown(edict_t* entity)
{
	CKnownEntity other(entity);

	for (auto& known : m_knownlist)
	{
		if (known == other)
		{
			return true;
		}
	}

	return false;
}

/**
 * @brief Given an entity, gets a knownentity of it
 * @param entity Entity to search
 * @return Pointer to a Knownentity of the given entity or NULL if the bot doesn't known this entity
*/
CKnownEntity* ISensor::GetKnown(edict_t* entity)
{
	CKnownEntity other(entity);

	for (auto& known : m_knownlist)
	{
		if (known == other)
		{
			return &known;
		}
	}

	return nullptr;
}

void ISensor::SetFieldOfView(const float fov)
{
	m_fieldofview = fov;
	m_coshalfFOV = cosf(0.5f * fov * M_PI / 180.0f);
}

void ISensor::UpdateKnownEntities()
{
	std::vector<edict_t*> visibleVec;
	visibleVec.reserve(1024);

	// Vision Update - Phase 1 - Collect entities
	CollectPlayers(visibleVec);
	CollectNonPlayerEntities(visibleVec);

	// Vision Update - Phase 2 - Clean current database of known entities
	CleanKnownEntities();

	// Vision Update - Phase 3 - Update database
	CollectVisibleEntities(visibleVec);

}

void ISensor::CollectVisibleEntities(std::vector<edict_t*>& visibleVec)
{
	for (auto edict : visibleVec)
	{
		auto known = GetKnown(edict);
		auto pos = UtilHelpers::getWorldSpaceCenter(edict);

		if (known == nullptr)
		{
			auto& entry = m_knownlist.emplace_back(edict);
			entry.MarkAsFullyVisible();
			continue;
		}

		known->UpdatePosition();
	}

}

void ISensor::CollectPlayers(std::vector<edict_t*>& visibleVec)
{
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		auto edict = gamehelpers->EdictOfIndex(i);

		if (edict == nullptr) {
			continue;
		}

		auto gp = playerhelpers->GetGamePlayer(i);

		if (!gp->IsInGame())
		{
			continue; // Client must be fully connected
		}

		auto info = gp->GetPlayerInfo();

		if (info && info->GetTeamIndex() <= TEAM_SPECTATOR) {
			continue; // Ignore spectators by default
		}

		if (info && info->IsDead()) {
			continue; // Ignore dead players
		}

		if (IsIgnored(edict)) {
			continue; // Ignored player
		}

		CBaseExtPlayer player(edict);

		if (IsAbleToSee(player))
		{
			visibleVec.push_back(edict);
		}
	}
}

void ISensor::CollectNonPlayerEntities(std::vector<edict_t*>& visibleVec)
{
	for (int i = gpGlobals->maxClients + 1; i < gpGlobals->maxEntities; i++)
	{
		auto edict = gamehelpers->EdictOfIndex(i);

		if (edict == nullptr) {
			continue;
		}

		if (edict->GetUnknown() == nullptr) {
			continue;
		}

		if (IsIgnored(edict)) {
			continue;
		}

		if (IsAbleToSee(edict))
		{
			visibleVec.push_back(edict);
		}
	}
}

// Removes obsolete known entities from the list
void ISensor::CleanKnownEntities()
{
	// Removes all obsoletes known entities
	auto iter = std::remove_if(m_knownlist.begin(), m_knownlist.end(),
		[](CKnownEntity& known) { return known.IsObsolete(); });

	m_knownlist.erase(iter, m_knownlist.end());
}

