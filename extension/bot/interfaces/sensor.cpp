#include NAVBOT_PCH_FILE
#include <limits>

#include <extension.h>
#include <manager.h>
#include <mods/basemod.h>
#include <mods/modhelpers.h>
#include <bot/basebot.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_area.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <sdkports/sdk_traces.h>
#include <sdkports/debugoverlay_shared.h>
#include <sdkports/sdk_takedamageinfo.h>
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
	m_cachedNPCupdaterate = extmanager->GetMod()->GetModSettings()->GetVisionNPCUpdateRate();
	m_updateStatisticsTimer.Start(extmanager->GetMod()->GetModSettings()->GetVisionStatisticsUpdateRate());
	m_statsvisibleallies = 0;
	m_statsvisibleenemies = 0;
	m_statsknownallies = 0;
}

ISensor::~ISensor()
{
}

void ISensor::OnInjured(const CTakeDamageInfo& info)
{
	CBaseEntity* subject = info.GetInflictor();

	if (subject && !IsIgnored(subject))
	{
		// Become aware of anyone who attacks me
		AddKnownEntity(subject);
	}
	else
	{
		subject = info.GetAttacker();

		if (subject && !IsIgnored(subject))
		{
			AddKnownEntity(subject);
		}
	}
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
	m_reportKnownsTimer.Start(ISensor::UPDATE_SHARED_MEMORY_FREQ);
	m_updateStatisticsTimer.Reset();
	m_primarythreatoverride.reset(nullptr);
	m_primarythreatcache = nullptr;
	m_statsvisibleallies = 0;
	m_statsvisibleenemies = 0;
	m_statsknownallies = 0;

	std::for_each(m_notVisibleTimer.begin(), m_notVisibleTimer.end(), [](IntervalTimer& timer) { timer.Invalidate(); });
}

void ISensor::Update()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISensor::Update", "NavBot");
#endif // EXT_VPROF_ENABLED

	UpdateKnownEntities();

	if (m_reportKnownsTimer.IsElapsed())
	{
		m_reportKnownsTimer.Start(ISensor::UPDATE_SHARED_MEMORY_FREQ);
		ReportVisibleEntities();
	}
}

void ISensor::Frame()
{
	// frame gets called before update, clear the primary threat cache
	m_primarythreatcache = nullptr;
}

void ISensor::ShowDebugInformation() const
{
	META_CONPRINTF("Known Entities: %zu\n", m_knownlist.size());

	for (auto& known : m_knownlist)
	{
		if (known.IsObsolete())
		{
			continue;
		}

		known.DebugDraw(10.0f);
	}
}

/**
 * @brief Expensive function that checks if the bot is able to see a given entity, testing for vision blockers, conditions, smokes, etc
 * @param player Player to test for visibility
 * @param checkFOV if true, also check if the given player is in the bot field of view
 * @return true if visible, false otherwise
*/
bool ISensor::IsAbleToSee(const CBaseExtPlayer* player, const bool checkFOV) const
{
	return IsAbleToSee(player->GetEntity(), checkFOV);
}

bool ISensor::IsAbleToSee(CBaseEntity* entity, const bool checkFOV) const
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
bool ISensor::IsAbleToSee(const Vector& pos, const bool checkFOV) const
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

bool ISensor::IsAbleToSee(const CNavArea* area, const bool checkFOV, const bool checkCorners) const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISensor::IsAbleToSee( CNavArea )", "NavBot");
#endif // EXT_VPROF_ENABLED

	CBaseBot* me = GetBot<CBaseBot>();
	const Vector& center = area->GetCenter();
	const Vector eyePos = me->GetEyeOrigin();
	const auto maxdist = GetMaxVisionRange() * GetMaxVisionRange();
	float distance = (center - eyePos).LengthSqr();
	const float crouchHeight = me->GetMovementInterface()->GetCrouchedHullHeight();
	Vector testPos = center;
	testPos.z += crouchHeight;

	// not visible, outside the bot's max vision range
	if (distance > maxdist)
	{
		return false;
	}

	if (checkFOV)
	{
		if (!IsInFieldOfView(testPos))
		{
			return false;
		}
	}

	if (!IsLineOfSightClear(testPos))
	{
		return false;
	}

	if (IsPositionObscured(testPos))
	{
		return false; // obstructed by smoke/fog
	}

	// skipping corners, center is visible
	if (!checkCorners)
	{
		return true;
	}

	for (int c1 = 0; c1 < static_cast<int>(NavCornerType::NUM_CORNERS); c1++)
	{
		testPos = area->GetCorner(static_cast<NavCornerType>(c1));
		testPos.z += crouchHeight;

		if (!IsLineOfSightClear(testPos))
		{
			return false;
		}
	}

	return true;
}

bool ISensor::IsAbleToHear(CBaseEntity* entity) const
{
	// only player and combat chars (NPCs) are interesting for bots
	return modhelpers->IsCombatCharacter(entity);
}

bool ISensor::IsLineOfSightClear(const Vector& pos) const
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

bool ISensor::IsLineOfSightClear(CBaseExtPlayer& player) const
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

bool ISensor::IsLineOfSightClear(CBaseEntity* entity) const
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

bool ISensor::IsInFieldOfView(const Vector& pos) const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISensor::IsInFieldOfView( pos )", "NavBot");
#endif // EXT_VPROF_ENABLED

	Vector forward;
	GetBot()->EyeVectors(&forward);
	return UtilHelpers::PointWithinViewAngle(GetBot()->GetEyeOrigin(), pos, forward, m_coshalfFOV);
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

void ISensor::ForgetKnownEntity(CBaseEntity* entity)
{
	const CKnownEntity other(entity);

	m_knownlist.erase(std::remove_if(m_knownlist.begin(), m_knownlist.end(), [&other](const CKnownEntity& obj) {
		return obj == other;
	}), m_knownlist.end());

	m_primarythreatcache = nullptr;
}

void ISensor::ForgetAllKnownEntities()
{
	m_knownlist.clear();
	m_primarythreatcache = nullptr;
}

bool ISensor::IsKnown(CBaseEntity* entity)
{
	if (!entity) { return false; }

	for (auto& known : m_knownlist)
	{
		if (known.GetEntity() == entity)
		{
			return true;
		}
	}

	return false;
}

const CKnownEntity* ISensor::GetKnown(CBaseEntity* entity) const
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

void ISensor::UpdateKnownEntity(CBaseEntity* entity)
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

const CKnownEntity* ISensor::GetPrimaryKnownThreat(const bool onlyvisible)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISensor::GetPrimaryKnownThreat", "NavBot");
#endif // EXT_VPROF_ENABLED

	CKnownEntity* primaryoverride = GetPrimaryThreatOverride();
	// use primary override if we have one.
	if (primaryoverride && primaryoverride->IsValid()) { return primaryoverride; }

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

		if (!onlyvisible) // allow non visible and we have a cached threat.
		{
			return m_primarythreatcache;
		}

		// if we want only visible threat and the cache is not visible, allow the code below to run and update the cache
	}

	const CKnownEntity* primarythreat = nullptr;
	bool assigned = false;

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

		// assign the first valid instance.
		if (!assigned)
		{
			primarythreat = known;
			assigned = true;
			continue;
		}
		
		// select between two valid instances
		primarythreat = GetBot()->GetBehaviorInterface()->SelectTargetThreat(GetBot(), primarythreat, known);
	}

	if (!primarythreat)
	{
		return nullptr;
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

		if (known->IsObsolete())
			continue;

		if (!IsAwareOf(known))
			continue;

		// NULL check is done in IsObsolete
		CBaseEntity* entity = known->GetEntity();

		if (IsIgnored(entity))
			continue;

		if (!IsEnemy(entity))
			continue;

		if (teamindex >= 0 && modhelpers->GetEntityTeamNumber(entity) != teamindex)
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

const CKnownEntity* ISensor::GetNearestKnown(const int teamindex)
{
	const CKnownEntity* nearest = nullptr;
	float smallest = std::numeric_limits<float>::max();
	auto origin = GetBot()->GetEyeOrigin();

	for (auto& obj : m_knownlist)
	{
		CKnownEntity* known = &obj;

		if (known->IsObsolete())
			continue;

		if (!IsAwareOf(known))
			continue;

		// NULL check is done in IsObsolete
		CBaseEntity* entity = known->GetEntity();

		if (IsIgnored(entity))
			continue;

		if (!IsEnemy(entity))
			continue;

		if (teamindex >= 0 && modhelpers->GetEntityTeamNumber(entity) != teamindex)
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

const CKnownEntity* ISensor::GetNearestHeardKnown(int teamIndex) const
{
	const CKnownEntity* nearest = nullptr;
	float smallest = std::numeric_limits<float>::max();
	Vector origin = GetBot()->GetEyeOrigin();

	for (auto& obj : m_knownlist)
	{
		const CKnownEntity* known = &obj;

		if (!IsAwareOf(known))
			continue;

		if (known->IsObsolete())
			continue;

		if (!known->WasEverHeard())
			continue;

		// NULL check is done in IsObsolete
		CBaseEntity* entity = known->GetEntity();

		if (IsIgnored(entity))
			continue;

		if (!IsEnemy(entity))
			continue;

		if (teamIndex >= TEAM_UNASSIGNED && modhelpers->GetEntityTeamNumber(entity) != teamIndex)
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

const CKnownEntity* ISensor::GetNearestUnseenKnownEntity() const
{
	const CKnownEntity* selected = nullptr;
	float dist_s = std::numeric_limits<float>::max(); // distance selected
	const CBaseBot* bot = GetBot<CBaseBot>();

	for (const CKnownEntity& known : m_knownlist)
	{
		if (known.IsObsolete()) { continue; }
		if (known.WasEverFullyVisible()) { continue; }
		if (known.WasLastKnownPositionSeen()) { continue; }

		// distance current
		float dist_c = (bot->GetRangeTo(known.GetLastKnownPosition()));

		if (dist_c < dist_s)
		{
			dist_s = dist_c;
			selected = &known;
		}
	}

	return selected;
}

void ISensor::ShareKnownEntityList(ISensor* other)
{
	CKnownEntity thisknown{ GetBot<CBaseBot>()->GetEntity() };

	for (CKnownEntity& known : other->m_knownlist)
	{
		if (!known.IsValid()) { continue; }

		if (known == thisknown) { continue; }

		CKnownEntity* newknown = AddKnownEntity(known.GetEntity());
		newknown->UpdatePosition();
	}
}

void ISensor::UpdateKnownEntities()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISensor::UpdateKnownEntities", "NavBot");
#endif // EXT_VPROF_ENABLED

	std::vector<CBaseEntity*> potentiallyVisible;
	potentiallyVisible.reserve(1024);

	if (cvar_navbot_notarget.GetInt() == 0)
	{
		CollectPlayers(potentiallyVisible);
		CollectNonPlayerEntities(potentiallyVisible);
	}

	UpdateVisibleEntities(potentiallyVisible);

	if (m_updateStatisticsTimer.IsElapsed())
	{
		m_updateStatisticsTimer.Reset();
		UpdateStatistics();
	}

	// Update the primary override if we have one.
	CKnownEntity* primaryoverride = GetPrimaryThreatOverride();

	if (primaryoverride && primaryoverride->IsValid())
	{
		if (IsAbleToSee(primaryoverride->GetEntity()))
		{
			primaryoverride->UpdatePosition();
			primaryoverride->UpdateVisibilityStatus(true);
			primaryoverride->MarkLastKnownPositionAsSeen();
		}
		else
		{
			primaryoverride->UpdateVisibilityStatus(false);
		}
	}

	m_lastupdatetime = gpGlobals->curtime;
}

void ISensor::UpdateVisibleEntities(const std::vector<CBaseEntity*>& potentiallyVisible)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISensor::UpdateVisibleEntities", "NavBot");
#endif // EXT_VPROF_ENABLED

	CBaseBot* me = GetBot<CBaseBot>();

	// Determine which entities are visible right now.
	CollectVisible visibleNow(this);
	std::unordered_set<CBaseEntity*> knownEntities;
	knownEntities.reserve(512);
	
	for (CBaseEntity* entity : potentiallyVisible)
	{
		visibleNow(entity);
	}

	for (auto it = m_knownlist.begin(); it != m_knownlist.end();)
	{
		CKnownEntity& known = *it;

		// remove obsolete instances
		if (known.IsObsolete())
		{
			it = m_knownlist.erase(it);
			continue;
		}

		CBaseEntity* pEntity = known.GetEntity();

		if (visibleNow.Contains(pEntity))
		{
			// entity is visible right now
			known.UpdatePosition();
			known.UpdateVisibilityStatus(true);
			knownEntities.insert(pEntity); // remember that this entity is already known by the bot

			// reaction time check
			if (known.GetTimeSinceBecameVisible() >= GetMinRecognitionTime() && m_lastupdatetime - known.GetTimeWhenBecameVisible() < GetMinRecognitionTime())
			{
				me->OnSight(pEntity);

				if (me->IsDebugging(BOTDEBUG_SENSOR))
				{
					me->DebugPrintToConsole(0, 128, 0, "%s CAUGHT LINE OF SIGHT WITH %s\n",
						me->GetDebugIdentifier(), UtilHelpers::textformat::FormatEntity(pEntity));

					NDebugOverlay::HorzArrow(me->GetEyeOrigin(), UtilHelpers::getWorldSpaceCenter(pEntity), 2.0f, 0, 255, 0, 255, false, 5.0f);
				}
			}

			UpdateSinceVisibleTime(modhelpers->GetEntityTeamNumber(pEntity));
		}
		else
		{
			// entity is not visible
			if (known.IsVisibleNow())
			{
				known.UpdateVisibilityStatus(false);
				me->OnLostSight(pEntity);

				if (me->IsDebugging(BOTDEBUG_SENSOR))
				{
					me->DebugPrintToConsole(255, 0, 0, "%s LOST LINE OF SIGHT WITH ENTITY %s \n",
						me->GetDebugIdentifier(), UtilHelpers::textformat::FormatEntity(pEntity));

					NDebugOverlay::HorzArrow(me->GetEyeOrigin(), UtilHelpers::getWorldSpaceCenter(pEntity), 2.0f, 255, 0, 0, 255, false, 5.0f);
				}

				if (IsAbleToSee(known.GetLastKnownPosition()))
				{
					known.MarkLastKnownPositionAsSeen();
				}
			}
		}

		it++;
	}

	// update entities the bot doesn't known about but are visible now
	visibleNow.ForEachVisible([&knownEntities, this](CBaseEntity* entity) {
		if (knownEntities.find(entity) == knownEntities.end())
		{
			// first time seeing this entity
			CKnownEntity* known = this->AddKnownEntity(entity);
			known->UpdatePosition();
			known->UpdateVisibilityStatus(true);
			// OnSight is called after reaction time has passed
		}
	});
}

void ISensor::CollectPlayers(std::vector<CBaseEntity*>& visibleVec)
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

		if (!entity) { continue; }

		if (!modhelpers->IsPlayableTeam(modhelpers->GetEntityTeamNumber(entity))) { continue; }

		if (IsIgnored(entity)) { continue; }

		visibleVec.push_back(entity);
	}
}

void ISensor::CollectNonPlayerEntities(std::vector<CBaseEntity*>& visibleVec)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISensor::CollectNonPlayerEntities", "NavBot");
#endif // EXT_VPROF_ENABLED

	for (auto& handle : ISensor::s_npcentities)
	{
		CBaseEntity* pEntity = handle.Get();

		if (pEntity)
		{
			visibleVec.push_back(pEntity);
		}
	}
}

void ISensor::ReportVisibleEntities()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISensor::ReportVisibleEntities", "NavBot");
#endif // EXT_VPROF_ENABLED

	CBaseBot* bot = GetBot<CBaseBot>();
	const DifficultyProfile* profile = bot->GetDifficultyProfile();
	ISharedBotMemory* sbm = bot->GetSharedMemoryInterface();

	// low skill bots don't use shared information.
	if (profile->GetTeamwork() < 20 || profile->GetGameAwareness() < 20) { return; }

	for (CKnownEntity& known : m_knownlist)
	{
		if (known.IsVisibleNow() && known && IsEnemy(known.GetEntity()))
		{
			sbm->ReportEntityVisible(known.GetEntity());
		}
	}
}

void ISensor::UpdateStatistics()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISensor::UpdateStatistics", "NavBot");
#endif // EXT_VPROF_ENABLED

	m_statsvisibleallies = 0;
	m_statsvisibleenemies = 0;
	m_statsknownallies = 0;

	for (CKnownEntity& known : m_knownlist)
	{
		if (!known.IsValid()) { continue; }

		CBaseEntity* pEntity = known.GetEntity();

		if (known.IsVisibleNow())
		{
			if (IsFriendly(pEntity))
			{
				m_statsvisibleallies++;
			}
			else if (IsEnemy(pEntity))
			{
				m_statsvisibleenemies++;
			}
		}

		if (IsFriendly(pEntity))
		{
			m_statsknownallies++;
		}
	}
}

void ISensor::CollectVisible::operator()(CBaseEntity* entity)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISensor::CollectVisible::operator()", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (!ISensor::IsInPVS(UtilHelpers::getWorldSpaceCenter(entity))) { return; }

	if (modhelpers->IsDead(entity)) { return; }

	if (m_sensor->IsIgnored(entity)) { return; }

	if (m_sensor->IsAbleToSee(entity))
	{
		m_visibleset.insert(entity);
	}
}