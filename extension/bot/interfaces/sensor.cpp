#include NAVBOT_PCH_FILE
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

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

#undef max
#undef min
#undef clamp

ConVar cvar_navbot_notarget("sm_navbot_debug_blind", "0", FCVAR_CHEAT | FCVAR_GAMEDLL, "When set to 1, disables the bot's vision.");
ConVar cvar_navbot_skip_pvs("sm_navbot_vision_skip_pvs", "0", FCVAR_GAMEDLL, "When enabled, skips the PVS check and always uses raycast for vision. Raycast is more expensive and enabling this will impact performance.");

class BotSensorTraceFilter : public trace::CTraceFilterSimple
{
public:
	BotSensorTraceFilter(int collisionGroup) : trace::CTraceFilterSimple(collisionGroup, nullptr) {}

	bool ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask) override;
};

bool BotSensorTraceFilter::ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("BotSensorTraceFilter::ShouldHitEntity", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (trace::CTraceFilterSimple::ShouldHitEntity(pHandleEntity, contentsMask))
	{
		CBaseEntity* pEntity = trace::EntityFromEntityHandle(pHandleEntity);

		if (!pEntity)
		{
			return true;
		}

		if (UtilHelpers::IsPlayer(pEntity))
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
	m_updateStatisticsTimer.Start(extmanager->GetMod()->GetModSettings()->GetVisionStatisticsUpdateRate());
	m_statsvisibleallies = 0;
	m_statsvisibleenemies = 0;
}

ISensor::~ISensor()
{
}

void ISensor::SetupPVS(CBaseBot* bot)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISensor::SetupPVS", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (cvar_navbot_skip_pvs.GetBool())
	{
		return;
	}

	engine->ResetPVS(ISensor::s_pvs.data(), static_cast<int>(ISensor::s_pvs.size()));
	engine->AddOriginToPVS(bot->GetEyeOrigin());
}

bool ISensor::IsInPVS(const Vector& origin)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISensor::IsInPVS( Point )", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (cvar_navbot_skip_pvs.GetBool())
	{
		return true;
	}

	return engine->CheckOriginInPVS(origin, ISensor::s_pvs.data(), static_cast<int>(ISensor::s_pvs.size()));
}

bool ISensor::IsInPVS(const Vector& mins, const Vector& maxs)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISensor::IsInPVS( Bounding Box )", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (cvar_navbot_skip_pvs.GetBool())
	{
		return true;
	}

	return engine->CheckBoxInPVS(mins, maxs, ISensor::s_pvs.data(), static_cast<int>(ISensor::s_pvs.size()));
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
	m_updateStatisticsTimer.Reset();
	m_primarythreatcache = nullptr;
	m_statsvisibleallies = 0;
	m_statsvisibleenemies = 0;
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
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISensor::IsAbleToSee( CBaseEntity )", "NavBot");
#endif // EXT_VPROF_ENABLED

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
		if (!IsInFieldOfView(pos))
		{
			return false;
		}
	}

	if (!IsLineOfSightClear(entity))
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
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISensor::IsAbleToSee( Vector )", "NavBot");
#endif // EXT_VPROF_ENABLED

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
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISensor::IsLineOfSightClear( Vector )", "NavBot");
#endif // EXT_VPROF_ENABLED

	auto start = GetBot()->GetEyeOrigin();

	BotSensorTraceFilter filter(COLLISION_GROUP_NONE);
	trace_t result;

	trace::line(start, pos, MASK_BLOCKLOS | CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, result);

	return result.fraction >= 1.0f && !result.startsolid;
}

bool ISensor::IsLineOfSightClear(CBaseExtPlayer& player)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISensor::IsLineOfSightClear( CBaseExtPlayer )", "NavBot");
#endif // EXT_VPROF_ENABLED

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
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISensor::IsLineOfSightClear( edict_t )", "NavBot");
#endif // EXT_VPROF_ENABLED

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
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISensor::IsLineOfSightClear( CBaseEntity )", "NavBot");
#endif // EXT_VPROF_ENABLED

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
			trace::line(start, UtilHelpers::getEntityOrigin(entity), MASK_BLOCKLOS | CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, result);
		}
	}

	return result.fraction >= 1.0f && !result.startsolid;
}

bool ISensor::IsInFieldOfView(const Vector& pos)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISensor::IsInFieldOfView( pos )", "NavBot");
#endif // EXT_VPROF_ENABLED

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
CKnownEntity* ISensor::AddKnownEntity(edict_t* entity)
{
	auto index = gamehelpers->IndexOfEdict(entity);

	if (index <= 0) // filter invalid edicts and worldspawn entity.
	{
		return nullptr;
	}

	CKnownEntity other(entity);

	for (auto& known : m_knownlist)
	{
		if (known == other)
		{
			return &known;
		}
	}

	auto& known = m_knownlist.emplace_back(entity);

	return &known;
}

CKnownEntity* ISensor::AddKnownEntity(CBaseEntity* entity)
{
	CKnownEntity other(entity);

	for (auto& known : m_knownlist)
	{
		if (known == other)
		{
			return &known;
		}
	}

	auto& known = m_knownlist.emplace_back(entity);

	return &known;
}

// Removes a entity from the known list
void ISensor::ForgetKnownEntity(edict_t* entity)
{
	CKnownEntity other(entity);

	m_knownlist.erase(std::remove_if(m_knownlist.begin(), m_knownlist.end(), [&other](CKnownEntity& obj) {
		return obj == other;
	}), m_knownlist.end());
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

const CKnownEntity* ISensor::GetKnown(CBaseEntity* entity)
{
	if (entity == nullptr)
	{
		return nullptr;
	}

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

/**
 * @brief Given an entity, gets a knownentity of it
 * @param entity Entity to search
 * @return Pointer to a Knownentity of the given entity or NULL if the bot doesn't known this entity
*/
const CKnownEntity* ISensor::GetKnown(edict_t* entity)
{
	if (entity == nullptr)
	{
		return nullptr;
	}

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

void ISensor::UpdateKnownEntity(edict_t* entity)
{
	auto known = AddKnownEntity(entity);

	if (known != nullptr)
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

const CKnownEntity* ISensor::GetPrimaryKnownThreat(const bool onlyvisible)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISensor::GetPrimaryKnownThreat", "NavBot");
#endif // EXT_VPROF_ENABLED

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

	const CKnownEntity* primarythreat = nullptr;

	// get the first valid threat

	for (auto& obj : m_knownlist)
	{
		CKnownEntity* known = &obj;

		if (!IsAwareOf(known))
			continue;

		if (known->IsObsolete())
			continue;

		if (IsIgnored(known->GetEntity()))
			continue;

		if (!IsEnemy(known->GetEntity()))
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
	for (auto& obj : m_knownlist)
	{
		CKnownEntity* known = &obj;

		if (!IsAwareOf(known))
			continue;

		if (known->IsObsolete())
			continue;

		if (IsIgnored(known->GetEntity()))
			continue;

		if (!IsEnemy(known->GetEntity()))
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

	for (auto& obj : m_knownlist)
	{
		CKnownEntity* known = &obj;

		if (!IsAwareOf(known))
			continue;

		if (known->IsObsolete())
			continue;

		if (IsIgnored(known->GetEntity()))
			continue;

		if (!IsEnemy(known->GetEntity()))
			continue;

		if (teamindex >= 0 && GetKnownEntityTeamIndex(known) != teamindex)
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

	for (auto& obj : m_knownlist)
	{
		CKnownEntity* known = &obj;

		if (!IsAwareOf(known))
			continue;

		if (known->IsObsolete())
			continue;

		if (IsIgnored(known->GetEntity()))
			continue;

		if (!IsEnemy(known->GetEntity()))
			continue;

		if (teamindex >= 0 && GetKnownEntityTeamIndex(known) != teamindex)
			continue;

		float distance = (origin - known->GetLastKnownPosition()).LengthSqr();

		if (distance < smallest)
		{
			nearest = known;
			smallest = distance;
		}
	}

	return nearest;
}

const CKnownEntity* ISensor::GetNearestHeardKnown(int teamIndex)
{
	const CKnownEntity* nearest = nullptr;
	float smallest = std::numeric_limits<float>::max();
	Vector origin = GetBot()->GetEyeOrigin();

	for (auto& obj : m_knownlist)
	{
		CKnownEntity* known = &obj;

		if (!IsAwareOf(known))
			continue;

		if (known->IsObsolete())
			continue;

		if (!known->WasEverHeard())
			continue;

		if (IsIgnored(known->GetEntity()))
			continue;

		if (!IsEnemy(known->GetEntity()))
			continue;

		if (teamIndex >= TEAM_UNASSIGNED && GetKnownEntityTeamIndex(known) != teamIndex)
			continue;

		float distance = (origin - known->GetLastKnownPosition()).LengthSqr();

		if (distance < smallest)
		{
			nearest = known;
			smallest = distance;
		}
	}

	return nearest;
}

void ISensor::UpdateKnownEntities()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISensor::UpdateKnownEntities", "NavBot");
#endif // EXT_VPROF_ENABLED

	std::vector<edict_t*> visibleVec;
	visibleVec.reserve(1024);

	if (cvar_navbot_notarget.GetInt() == 0)
	{
		ISensor::SetupPVS(GetBot<CBaseBot>());

		// Vision Update - Phase 1 - Collect entities
		CollectPlayers(visibleVec);

		if (m_updateNonPlayerTimer.IsElapsed())
		{
			m_updateNonPlayerTimer.Start(m_cachedNPCupdaterate);
			CollectNonPlayerEntities(visibleVec);
		}
	}

	// Vision Update - Phase 2 - Clean current database of known entities
	CleanKnownEntities();

	// Vision Update - Phase 3 - Update database
	CollectVisibleEntities(visibleVec);

	if (m_updateStatisticsTimer.IsElapsed())
	{
		m_updateStatisticsTimer.Reset();
		UpdateStatistics();
	}

	m_lastupdatetime = gpGlobals->curtime;
}

void ISensor::CollectVisibleEntities(std::vector<edict_t*>& visibleVec)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISensor::CollectVisibleEntities", "NavBot");
#endif // EXT_VPROF_ENABLED

	CBaseBot* me = GetBot<CBaseBot>();

	for (edict_t* edict : visibleVec)
	{
#ifdef EXT_VPROF_ENABLED
		VPROF_BUDGET("ISensor::CollectVisibleEntities( collect visible )", "NavBot");
#endif // EXT_VPROF_ENABLED

		// all entities inside visibleVec are visible to the bot RIGHT NOW!
		CKnownEntity* known = FindKnownEntity(edict);

		if (known == nullptr) // first time seening this entity
		{
			CKnownEntity& entry = m_knownlist.emplace_back(edict);
			entry.MarkAsFullyVisible();
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

	for (CKnownEntity& known : m_knownlist)
	{
#ifdef EXT_VPROF_ENABLED
		VPROF_BUDGET("ISensor::CollectVisibleEntities( update status )", "NavBot");
#endif // EXT_VPROF_ENABLED

		CBaseEntity* pEntity = known.GetEntity();

		if (known.GetTimeSinceLastInfo() < 0.2f)
		{
			// reaction time check
			if (known.GetTimeSinceLastVisible() >= GetMinRecognitionTime() && m_lastupdatetime - known.GetTimeWhenBecameVisible() < GetMinRecognitionTime())
			{
				me->OnSight(pEntity);
				m_threatvisibletimer.Start();

				if (me->IsDebugging(BOTDEBUG_SENSOR))
				{
					me->DebugPrintToConsole(0, 128, 0, "%s caught line of sight with entity %i (%s) \n", 
						me->GetDebugIdentifier(), gamehelpers->EntityToBCompatRef(pEntity), gamehelpers->GetEntityClassname(pEntity));

					NDebugOverlay::HorzArrow(me->GetEyeOrigin(), UtilHelpers::getWorldSpaceCenter(pEntity), 2.0f, 0, 255, 0, 255, false, 5.0f);
				}
			}

			continue; // this known entity was updated recently
		}
		else
		{
			if (IsAbleToSee(known.GetLastKnownPosition()) == true)
			{
				known.MarkLastKnownPositionAsSeen();
			}

			// this entity was visible, mark as not visible and notify the bot lost sight of it
			if (known.IsVisibleNow() == true)
			{
				known.MarkAsNotVisible();
				me->OnLostSight(pEntity);

				if (me->IsDebugging(BOTDEBUG_SENSOR))
				{
					me->DebugPrintToConsole(255, 0, 0, "%s lost line of sight with entity %i (%s) \n",
						me->GetDebugIdentifier(), gamehelpers->EntityToBCompatRef(pEntity), gamehelpers->GetEntityClassname(pEntity));

					NDebugOverlay::HorzArrow(me->GetEyeOrigin(), UtilHelpers::getWorldSpaceCenter(pEntity), 2.0f, 255, 0, 0, 255, false, 5.0f);
				}
			}
		}
	}
}

void ISensor::CollectPlayers(std::vector<edict_t*>& visibleVec)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISensor::CollectPlayers", "NavBot");
#endif // EXT_VPROF_ENABLED

	const int myindex = GetBot<CBaseBot>()->GetIndex();

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		// skip self
		if (i == myindex)
		{
			continue;
		}

		CBaseEntity* entity = gamehelpers->ReferenceToEntity(i);

		if (!entity || entityprops::GetEntityTeamNum(entity) <= TEAM_SPECTATOR || entityprops::GetEntityLifeState(entity) != LIFE_ALIVE)
		{
			continue;
		}

		if (!ISensor::IsInPVS(UtilHelpers::getEntityOrigin(entity)))
		{
			continue;
		}

		if (IsAbleToSee(entity))
		{
			visibleVec.push_back(UtilHelpers::BaseEntityToEdict(entity));
		}
	}
}

void ISensor::CollectNonPlayerEntities(std::vector<edict_t*>& visibleVec)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISensor::CollectNonPlayerEntities", "NavBot");
#endif // EXT_VPROF_ENABLED

	for (int i = gpGlobals->maxClients + 1; i < gpGlobals->maxEntities; i++)
	{
		auto edict = gamehelpers->EdictOfIndex(i);

		if (!UtilHelpers::IsValidEdict(edict))
		{
			continue;
		}

		CBaseEntity* entity = edict->GetIServerEntity()->GetBaseEntity();

		if (IsIgnored(entity)) 
		{
			continue;
		}

		// Use world space center here since the bot may be testing against a brush entity
		if (!ISensor::IsInPVS(UtilHelpers::getWorldSpaceCenter(edict)))
		{
			continue;
		}

		if (IsAbleToSee(entity))
		{
			visibleVec.push_back(edict);
		}
	}
}

// Removes obsolete known entities from the list
void ISensor::CleanKnownEntities()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISensor::CleanKnownEntities", "NavBot");
#endif // EXT_VPROF_ENABLED
	// Removes all obsoletes known entities
	auto iter = std::remove_if(m_knownlist.begin(), m_knownlist.end(),
		[](CKnownEntity& known) { return known.IsObsolete(); });

	m_knownlist.erase(iter, m_knownlist.end());
}

CKnownEntity* ISensor::FindKnownEntity(edict_t* edict)
{
	int index = gamehelpers->IndexOfEdict(edict);

	for (auto& known : m_knownlist)
	{
		if (gamehelpers->IndexOfEdict(known.GetEdict()) == index)
		{
			return &known;
		}
	}

	return nullptr;
}

void ISensor::UpdateStatistics()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISensor::UpdateStatistics", "NavBot");
#endif // EXT_VPROF_ENABLED

	m_statsvisibleallies = 0;
	m_statsvisibleenemies = 0;

	for (CKnownEntity& known : m_knownlist)
	{
		if (known.IsVisibleNow())
		{
			CBaseEntity* pEntity = known.GetEntity();

			if (IsFriendly(pEntity))
			{
				m_statsvisibleallies++;
			}
			else if (IsEnemy(pEntity))
			{
				m_statsvisibleenemies++;
			}
		}
	}
}

