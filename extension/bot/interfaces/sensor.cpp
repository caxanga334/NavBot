#include <vector>

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
	m_fieldofview = 0.0f;
	m_coshalfFOV = 0.0f;
}

ISensor::~ISensor()
{
}

void ISensor::Reset()
{
	m_knownlist.clear();
}

void ISensor::Update()
{
}

void ISensor::Frame()
{
}

bool ISensor::IsAbleToSee(edict_t* entity)
{
	return false;
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

bool ISensor::IsInFieldOfView(const Vector& pos)
{
	Vector forward;
	GetBot()->EyeVectors(&forward);
	return UtilHelpers::PointWithinViewAngle(GetBot()->GetEyeOrigin(), pos, forward, m_coshalfFOV);
}

void ISensor::SetFieldOfView(const float fov)
{
	m_fieldofview = fov;
	m_coshalfFOV = cosf(0.5f * fov * M_PI / 180.0f);
}
