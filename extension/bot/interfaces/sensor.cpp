#include <limits>

#include <extension.h>
#include <bot/basebot.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_area.h>
#include <util/helpers.h>
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
	auto& profile = bot->GetDifficultyProfile();

	SetFieldOfView(profile.GetFOV());
	m_maxvisionrange = static_cast<float>(profile.GetMaxVisionRange());
	m_maxhearingrange = static_cast<float>(profile.GetMaxHearingRange());
	m_minrecognitiontime = profile.GetMinRecognitionTime();
	m_lastupdatetime = 0.0f;
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
	m_minrecognitiontime = profile.GetMinRecognitionTime();
}

void ISensor::Reset()
{
	m_knownlist.clear();
	m_lastupdatetime = 0.0f;
	m_threatvisibletimer.Invalidate();
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
		if (IsInFieldOfView(pos) == false)
		{
			return false;
		}
	}

	if (IsLineOfSightClear(pos) == false)
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
		if (IsInFieldOfView(pos) == false)
		{
			return false;
		}
	}

	if (IsLineOfSightClear(player) == false)
	{
		return false;
	}

	// Test for smoke, fog, ...
	if (IsEntityHidden(player.GetEdict()))
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

	if (index <= 0) // filter invalid edicts and worldspawn entity.
	{
		return false;
	}

	CKnownEntity other(entity);

	if (std::find(m_knownlist.begin(), m_knownlist.end(), other) != m_knownlist.end())
	{
		return true; // Entity is already in the list
	}

	m_knownlist.emplace_back(entity);

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
const CKnownEntity* ISensor::GetKnown(edict_t* entity)
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

const CKnownEntity* ISensor::GetPrimaryKnownThreat(const bool onlyvisible)
{
	if (m_knownlist.empty())
		return nullptr;

	const CKnownEntity* primarythreat = nullptr;

	// get the first valid threat

	for (auto& known : m_knownlist)
	{
		if (!IsAwareOf(known))
			continue;

		if (known.IsObsolete())
			continue;

		if (IsIgnored(known.GetEdict()))
			continue;

		if (!IsEnemy(known.GetEdict()))
			continue;

		if (onlyvisible && !known.WasRecentlyVisible())
			continue;

		primarythreat = &known;
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

		if (known.IsObsolete())
			continue;

		if (IsIgnored(known.GetEdict()))
			continue;

		if (!IsEnemy(known.GetEdict()))
			continue;

		if (onlyvisible && !known.WasRecentlyVisible())
			continue;

		// Ask behavior for the best threat
		primarythreat = GetBot()->GetBehaviorInterface()->SelectTargetThreat(GetBot(), primarythreat, &known);
	}

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

		if (known.IsObsolete())
			continue;

		if (IsIgnored(known.GetEdict()))
			continue;

		if (!IsEnemy(known.GetEdict()))
			continue;

		if (teamindex >= 0 && GetKnownEntityTeamIndex(&known) != teamindex)
			continue;

		if (onlyvisible && !known.WasRecentlyVisible())
			continue;

		float dist = GetBot()->GetRangeToSqr(known.GetLastKnownPosition());

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

	for (auto& known : m_knownlist)
	{
		if (!IsAwareOf(known))
			continue;

		if (known.IsObsolete())
			continue;

		if (IsIgnored(known.GetEdict()))
			continue;

		if (!IsEnemy(known.GetEdict()))
			continue;

		if (teamindex >= 0 && GetKnownEntityTeamIndex(&known) != teamindex)
			continue;

		float distance = (origin - known.GetLastKnownPosition()).LengthSqr();

		if (distance < smallest)
		{
			nearest = &known;
			smallest = distance;
		}
	}

	return nearest;
}

void ISensor::OnSound(edict_t* source, const Vector& position, SoundType type, const int volume)
{
	Vector origin = GetBot()->GetAbsOrigin();
	float distance = (origin - position).Length();
	float maxdistance = GetMaxHearingRange();
	constexpr auto GUNFIRE_MULTIPLIER = 1.5f;
	
	if (type == IEventListener::SoundType::SOUND_WEAPON)
	{
		maxdistance = maxdistance * GUNFIRE_MULTIPLIER; // weapons are loud
	}

	if (distance > maxdistance)
	{
		return; // outside hearing range
	}

	auto known = FindKnownEntity(source);

	if (known == nullptr)
	{
		AddKnownEntity(source);
		known = FindKnownEntity(source);
		known->NotifyHeard(volume, position);
	}
	else
	{
		known->NotifyHeard(volume, position);
	}
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

	m_lastupdatetime = gpGlobals->curtime;
}

void ISensor::CollectVisibleEntities(std::vector<edict_t*>& visibleVec)
{
	auto me = GetBot();

	for (auto edict : visibleVec)
	{
		// all entities inside visibleVec are visible to the bot RIGHT NOW!
		auto known = FindKnownEntity(edict);
		auto pos = UtilHelpers::getWorldSpaceCenter(edict);

		if (known == nullptr) // first time seening this entity
		{
			auto& entry = m_knownlist.emplace_back(edict);
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

	for (auto& known : m_knownlist)
	{
		if (known.GetTimeSinceLastInfo() < 0.2f)
		{
			// reaction time check
			if (known.GetTimeSinceLastVisible() >= GetMinRecognitionTime() && m_lastupdatetime - known.GetTimeWhenBecameVisible() < GetMinRecognitionTime())
			{
				me->OnSight(known.GetEdict());
				m_threatvisibletimer.Start();

				if (me->IsDebugging(BOTDEBUG_SENSOR))
				{
					auto edict = known.GetEdict();
					rootconsole->ConsolePrint("%s caught line of sight with entity %i (%s)", me->GetDebugIdentifier(), 
						gamehelpers->IndexOfEdict(edict), gamehelpers->GetEntityClassname(edict));

					NDebugOverlay::HorzArrow(me->GetEyeOrigin(), UtilHelpers::getWorldSpaceCenter(edict), 4.0f, 0, 255, 0, 255, false, 5.0f);
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
				me->OnLostSight(known.GetEdict());

				if (me->IsDebugging(BOTDEBUG_SENSOR))
				{
					auto edict = known.GetEdict();
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

		if (edict == nullptr) {
			continue;
		}

		if (edict->IsFree())
		{
			continue;
		}

		if (edict->GetUnknown() == nullptr) {
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

		if (edict == nullptr) {
			continue;
		}

		if (edict->IsFree())
		{
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

