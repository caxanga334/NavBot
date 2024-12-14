#include <limits>

#include <extension.h>
#include <manager.h>
#include <mods/basemod.h>
#include <bot/basebot.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_area.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <sdkports/sdk_traces.h>
#include <sdkports/debugoverlay_shared.h>
#include "sensor.h"

#undef max
#undef min
#undef clamp

class BotSensorTraceFilter : public trace::CTraceFilterSimple
{
public:
	BotSensorTraceFilter(int collisionGroup) : trace::CTraceFilterSimple(collisionGroup, nullptr) {}

	bool ShouldHitEntity(int entity, CBaseEntity* pEntity, edict_t* pEdict, const int contentsMask) override;
};

bool BotSensorTraceFilter::ShouldHitEntity(int entity, CBaseEntity* pEntity, edict_t* pEdict, const int contentsMask)
{
	if (CTraceFilterSimple::ShouldHitEntity(entity, pEntity, pEdict, contentsMask))
	{
		if (UtilHelpers::IsPlayerIndex(entity))
		{
			return false; // don't hit players
		}

		return true;
	}

	return false;
}

ISensor::ISensor(CBaseBot* bot) : IBotInterface(bot)
{
	m_knownlist.reserve(256);
	auto profile = bot->GetDifficultyProfile();

	SetFieldOfView(profile->GetFOV());
	m_maxvisionrange = static_cast<float>(profile->GetMaxVisionRange());
	m_maxhearingrange = static_cast<float>(profile->GetMaxHearingRange());
	m_minrecognitiontime = profile->GetMinRecognitionTime();
	m_lastupdatetime = 0.0f;
	m_primarythreatcache = nullptr;
	m_updateNonPlayerTimer.Invalidate();
	m_cachedNPCupdaterate = extmanager->GetMod()->GetModSettings()->GetVisionNPCUpdateRate();
}

ISensor::~ISensor()
{
}

void ISensor::OnDifficultyProfileChanged()
{
	auto profile = GetBot()->GetDifficultyProfile();

	SetFieldOfView(profile->GetFOV());
	m_maxvisionrange = static_cast<float>(profile->GetMaxVisionRange());
	m_maxhearingrange = static_cast<float>(profile->GetMaxHearingRange());
	m_minrecognitiontime = profile->GetMinRecognitionTime();
}

void ISensor::Reset()
{
	m_knownlist.clear();
	m_lastupdatetime = 0.0f;
	m_threatvisibletimer.Invalidate();
	m_updateNonPlayerTimer.Invalidate();
}

void ISensor::Update()
{
	UpdateKnownEntities();
}

void ISensor::Frame()
{
	// frame gets called before update, clear the primary threat cache
	m_primarythreatcache = nullptr;
}

bool ISensor::IsAbleToSee(edict_t* entity, const bool checkFOV)
{
	return IsAbleToSee(entity->GetIServerEntity()->GetBaseEntity(), checkFOV);
}

/**
 * @brief Expensive function that checks if the bot is able to see a given entity, testing for vision blockers, conditions, smokes, etc
 * @param player Player to test for visibility
 * @param checkFOV if true, also check if the given player is in the bot field of view
 * @return true if visible, false otherwise
*/
bool ISensor::IsAbleToSee(CBaseExtPlayer& player, const bool checkFOV)
{
	return IsAbleToSee(player.GetEntity(), checkFOV);
}

bool ISensor::IsAbleToSee(CBaseEntity* entity, const bool checkFOV)
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
		if (IsInFieldOfView(pos) == false)
		{
			return false;
		}
	}

	if (IsLineOfSightClear(entity) == false)
	{
		return false;
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
 * @param pos Position to test for visibility
 * @param checkFOV if true, also check if the given position is in the bot field of view
 * @return true if visible
*/
bool ISensor::IsAbleToSee(const Vector& pos, const bool checkFOV)
{
	auto me = GetBot();
	auto start = me->GetEyeOrigin();
	const auto maxdist = GetMaxVisionRange() * GetMaxVisionRange();
	float distance = (start - pos).LengthSqr();

	if (distance > maxdist)
	{
		return false;
	}

	if (checkFOV == true)
	{
		if (IsInFieldOfView(pos) == false)
		{
			return false;
		}
	}

	if (IsLineOfSightClear(pos) == false)
	{
		return false;
	}

	if (IsPositionObscured(pos) == true)
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

	trace::line(start, pos, MASK_BLOCKLOS | CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, result);

	return result.fraction >= 1.0f && !result.startsolid;
}

bool ISensor::IsLineOfSightClear(CBaseExtPlayer& player)
{
	auto start = GetBot()->GetEyeOrigin();
	BotSensorTraceFilter filter(COLLISION_GROUP_NONE);
	trace_t result;

	trace::line(start, player.GetEyeOrigin(), MASK_BLOCKLOS | CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, result);

	if (result.DidHit())
	{
		trace::line(start, player.WorldSpaceCenter(), MASK_BLOCKLOS | CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, result);

		if (result.DidHit())
		{
			trace::line(start, player.GetAbsOrigin(), MASK_BLOCKLOS | CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, result);
		}
	}

	return result.fraction >= 1.0f && !result.startsolid;
}

bool ISensor::IsLineOfSightClear(edict_t* entity)
{
	auto start = GetBot()->GetEyeOrigin();
	entities::HBaseEntity baseent(entity);
	BotSensorTraceFilter filter(COLLISION_GROUP_NONE);
	trace_t result;

	trace::line(start, baseent.EyePosition(), MASK_BLOCKLOS | CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, result);

	if (result.DidHit())
	{
		trace::line(start, UtilHelpers::getWorldSpaceCenter(entity), MASK_BLOCKLOS | CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, result);

		if (result.DidHit())
		{
			trace::line(start, baseent.GetAbsOrigin(), MASK_BLOCKLOS | CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, result);
		}
	}

	return result.fraction >= 1.0f && !result.startsolid;
}

bool ISensor::IsLineOfSightClear(CBaseEntity* entity)
{
	auto start = GetBot()->GetEyeOrigin();
	entities::HBaseEntity baseent(entity);
	BotSensorTraceFilter filter(COLLISION_GROUP_NONE);
	trace_t result;

	trace::line(start, baseent.EyePosition(), MASK_BLOCKLOS | CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, result);

	if (result.DidHit())
	{
		trace::line(start, UtilHelpers::getWorldSpaceCenter(entity), MASK_BLOCKLOS | CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, result);

		if (result.DidHit())
		{
			trace::line(start, baseent.GetAbsOrigin(), MASK_BLOCKLOS | CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, result);
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

bool ISensor::IsEntityHidden(edict_t* entity)
{
	return IsEntityHidden(entity->GetIServerEntity()->GetBaseEntity());
}

/**
 * @brief Adds the given entity to the known entity list
 * @param entity Entity to be added to the list
 * @return true if the entity was added or already exists, false if it was not possible to add
*/
bool ISensor::AddKnownEntity(edict_t* entity)
{
	auto index = gamehelpers->IndexOfEdict(entity);

	if (index <= 0) // filter invalid edicts and worldspawn entity.
	{
		return false;
	}

	CKnownEntity other(entity);

	if (std::find_if(m_knownlist.begin(), m_knownlist.end(), [&other](std::shared_ptr<CKnownEntity>& obj) { return *obj == other; }) != m_knownlist.end())
	{
		return true; // Entity is already in the list
	}

	m_knownlist.emplace_back(new CKnownEntity(entity));

	return true;
}

// Removes a entity from the known list
void ISensor::ForgetKnownEntity(edict_t* entity)
{
	CKnownEntity other(entity);

	m_knownlist.erase(std::remove_if(m_knownlist.begin(), m_knownlist.end(), [&other](std::shared_ptr<CKnownEntity>& obj) {
		return *obj == other;
	}), m_knownlist.end());
}

void ISensor::ForgetAllKnownEntities()
{
	m_knownlist.clear();
}

bool ISensor::IsKnown(edict_t* entity)
{
	CKnownEntity other(entity);

	for (auto& ptr : m_knownlist)
	{
		auto& known = *ptr;

		if (known == other)
		{
			return true;
		}
	}

	return false;
}

std::shared_ptr<const CKnownEntity> ISensor::GetKnown(CBaseEntity* entity)
{
	CKnownEntity other(entity);

	for (auto& ptr : m_knownlist)
	{
		auto& known = *ptr;

		if (known == other)
		{
			return ptr;
		}
	}

	return nullptr;
}

/**
 * @brief Given an entity, gets a knownentity of it
 * @param entity Entity to search
 * @return Pointer to a Knownentity of the given entity or NULL if the bot doesn't known this entity
*/
std::shared_ptr<const CKnownEntity> ISensor::GetKnown(edict_t* entity)
{
	CKnownEntity other(entity);

	for (auto& ptr : m_knownlist)
	{
		auto& known = *ptr;

		if (known == other)
		{
			return ptr;
		}
	}

	return nullptr;
}

void ISensor::UpdateKnownEntity(edict_t* entity)
{
	auto known = FindKnownEntity(entity);

	if (known == nullptr)
	{
		AddKnownEntity(entity);
	}
	else
	{
		known->UpdatePosition();
	}
}

void ISensor::SetFieldOfView(const float fov)
{
	m_fieldofview = fov;
	m_coshalfFOV = cosf(0.5f * fov * M_PI / 180.0f);
}

const float ISensor::GetTimeSinceVisibleThreat() const
{
	return m_threatvisibletimer.GetElapsedTime();
}

std::shared_ptr<const CKnownEntity> ISensor::GetPrimaryKnownThreat(const bool onlyvisible)
{
	if (m_knownlist.empty())
		return nullptr;

	// cached threat from the last call
	if (m_primarythreatcache)
	{
		// only visible and primary threat is visible right now.
		if (onlyvisible && m_primarythreatcache->IsVisibleNow())
		{
			return m_primarythreatcache;
		}
		else if (!onlyvisible) // allow non visible and we have a cached threat.
		{
			return m_primarythreatcache;
		}

		// if we want only visible threat and the cache is not visible, allow the code below to run and update the cache
	}

	std::shared_ptr<const CKnownEntity> primarythreat = nullptr;

	// get the first valid threat

	for (auto& known : m_knownlist)
	{
		if (!IsAwareOf(known))
			continue;

		if (known->IsObsolete())
			continue;

		if (IsIgnored(known->GetEdict()))
			continue;

		if (!IsEnemy(known->GetEdict()))
			continue;

		if (onlyvisible && !known->WasRecentlyVisible())
			continue;

		primarythreat = known;
		break;
	}

	if (primarythreat == nullptr)
	{
		return nullptr;
	}

	// Selected best threat
	for (auto& known : m_knownlist)
	{
		if (!IsAwareOf(known))
			continue;

		if (known->IsObsolete())
			continue;

		if (IsIgnored(known->GetEdict()))
			continue;

		if (!IsEnemy(known->GetEdict()))
			continue;

		if (onlyvisible && !known->WasRecentlyVisible())
			continue;

		// Ask behavior for the best threat
		primarythreat = GetBot()->GetBehaviorInterface()->SelectTargetThreat(GetBot(), primarythreat, known);
	}

	m_primarythreatcache = primarythreat; // cache primary threat to skip calculations for multiple GetPrimaryKnownThreat calls, cache is cleared on the next server frame
	return primarythreat;
}

int ISensor::GetKnownCount(const int teamindex, const bool onlyvisible, const float rangelimit)
{
	int quantity = 0;
	auto origin = GetBot()->GetEyeOrigin();
	const float limit = rangelimit * rangelimit; // use squared distances

	for (auto& known : m_knownlist)
	{
		if (!IsAwareOf(known))
			continue;

		if (known->IsObsolete())
			continue;

		if (IsIgnored(known->GetEdict()))
			continue;

		if (!IsEnemy(known->GetEdict()))
			continue;

		if (teamindex >= 0 && GetKnownEntityTeamIndex(known.get()) != teamindex)
			continue;

		if (onlyvisible && !known->WasRecentlyVisible())
			continue;

		float dist = GetBot()->GetRangeToSqr(known->GetLastKnownPosition());

		if (dist > limit)
			continue;

		quantity++;
	}

	return quantity;
}

int ISensor::GetKnownEntityTeamIndex(CKnownEntity* known)
{
	int* teamNum = entprops->GetPointerToEntData<int>(known->GetEntity(), Prop_Data, "m_iTeamNum");

	if (teamNum != nullptr)
	{
		return *teamNum;
	}

	return 0;
}

const CKnownEntity* ISensor::GetNearestKnown(const int teamindex)
{
	const CKnownEntity* nearest = nullptr;
	float smallest = std::numeric_limits<float>::max();
	auto origin = GetBot()->GetEyeOrigin();

	for (auto& known : m_knownlist)
	{
		if (!IsAwareOf(known))
			continue;

		if (known->IsObsolete())
			continue;

		if (IsIgnored(known->GetEdict()))
			continue;

		if (!IsEnemy(known->GetEdict()))
			continue;

		if (teamindex >= 0 && GetKnownEntityTeamIndex(known.get()) != teamindex)
			continue;

		float distance = (origin - known->GetLastKnownPosition()).LengthSqr();

		if (distance < smallest)
		{
			nearest = known.get();
			smallest = distance;
		}
	}

	return nearest;
}

void ISensor::UpdateKnownEntities()
{
	std::vector<edict_t*> visibleVec;
	visibleVec.reserve(1024);

	// Vision Update - Phase 1 - Collect entities
	CollectPlayers(visibleVec);

	if (m_updateNonPlayerTimer.IsElapsed())
	{
		m_updateNonPlayerTimer.Start(m_cachedNPCupdaterate);
		CollectNonPlayerEntities(visibleVec);
	}

	// Vision Update - Phase 2 - Clean current database of known entities
	CleanKnownEntities();

	// Vision Update - Phase 3 - Update database
	CollectVisibleEntities(visibleVec);

	m_lastupdatetime = gpGlobals->curtime;
}

void ISensor::CollectVisibleEntities(std::vector<edict_t*>& visibleVec)
{
	auto me = GetBot();

	for (auto edict : visibleVec)
	{
		// all entities inside visibleVec are visible to the bot RIGHT NOW!
		auto known = FindKnownEntity(edict);

		if (known == nullptr) // first time seening this entity
		{
			auto& entry = m_knownlist.emplace_back(new CKnownEntity(edict));
			entry->MarkAsFullyVisible();
			continue;
		}
		else
		{
			known->UpdatePosition(); // we can see it, so update it's position

			if (known->IsVisibleNow() == false)
			{
				known->MarkAsFullyVisible();
			}
		}
	}

	for (auto& known : m_knownlist)
	{
		if (known->GetTimeSinceLastInfo() < 0.2f)
		{
			// reaction time check
			if (known->GetTimeSinceLastVisible() >= GetMinRecognitionTime() && m_lastupdatetime - known->GetTimeWhenBecameVisible() < GetMinRecognitionTime())
			{
				me->OnSight(known->GetEdict());
				m_threatvisibletimer.Start();

				if (me->IsDebugging(BOTDEBUG_SENSOR))
				{
					auto edict = known->GetEdict();
					rootconsole->ConsolePrint("%s caught line of sight with entity %i (%s)", me->GetDebugIdentifier(), 
						gamehelpers->IndexOfEdict(edict), gamehelpers->GetEntityClassname(edict));

					NDebugOverlay::HorzArrow(me->GetEyeOrigin(), UtilHelpers::getWorldSpaceCenter(edict), 4.0f, 0, 255, 0, 255, false, 5.0f);
				}
			}

			continue; // this known entity was updated recently
		}
		else
		{
			if (IsAbleToSee(known->GetLastKnownPosition()) == true)
			{
				known->MarkLastKnownPositionAsSeen();
			}

			// this entity was visible, mark as not visible and notify the bot lost sight of it
			if (known->IsVisibleNow() == true)
			{
				known->MarkAsNotVisible();
				me->OnLostSight(known->GetEdict());

				if (me->IsDebugging(BOTDEBUG_SENSOR))
				{
					auto edict = known->GetEdict();
					rootconsole->ConsolePrint("%s lost line of sight with entity %i (%s)", me->GetDebugIdentifier(),
						gamehelpers->IndexOfEdict(edict), gamehelpers->GetEntityClassname(edict));

					NDebugOverlay::HorzArrow(me->GetEyeOrigin(), UtilHelpers::getWorldSpaceCenter(edict), 4.0f, 255, 0, 0, 255, false, 5.0f);
				}
			}
		}
	}
}

void ISensor::CollectPlayers(std::vector<edict_t*>& visibleVec)
{
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		auto edict = gamehelpers->EdictOfIndex(i);

		if (!UtilHelpers::IsValidEdict(edict)) 
		{
			continue;
		}

		if (i == GetBot()->GetIndex())
			continue; // skip self

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

		if (!UtilHelpers::IsValidEdict(edict))
		{
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
		[](std::shared_ptr<CKnownEntity>& known) { return known->IsObsolete(); });

	m_knownlist.erase(iter, m_knownlist.end());
}

CKnownEntity* ISensor::FindKnownEntity(edict_t* edict)
{
	int index = gamehelpers->IndexOfEdict(edict);

	for (auto& known : m_knownlist)
	{
		if (gamehelpers->IndexOfEdict(known->GetEdict()) == index)
		{
			return known.get();
		}
	}

	return nullptr;
}

