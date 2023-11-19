#ifndef SMNAV_BOT_KNOWN_ENTITY_H_
#define SMNAV_BOT_KNOWN_ENTITY_H_
#pragma once

#include <basehandle.h>
#include <util/helpers.h>

namespace NKnownEntity
{
	constexpr auto TIME_FOR_OBSOLETE = 20.0f;
}

// Known Entity. Represents an entity that is known to the bot
class CKnownEntity
{
public:
	CKnownEntity(edict_t* entity);
	CKnownEntity(int entity);

	virtual ~CKnownEntity();

	bool operator==(const CKnownEntity& other);
	inline bool operator!=(const CKnownEntity& other) { return !(*this == other); }

	// Returns true if the entity was completely visible to the bot at some point
	inline bool WasEvenFullyVisible() const { return m_timelastvisible > 0.0f; }
	// Returns true if the bot heard this entity at some point
	inline bool WasEverHeard() const { return m_volume > 0; }
	// Returns how many seconds have passed since this entity became known to the bot
	inline float GetTimeSinceBecomeKnown() const { return gpGlobals->curtime - m_timeknown; }
	// Returns how many seconds have passed since this entity was completely visible
	inline float GetTimeSinceLastVisible() const { return gpGlobals->curtime - m_timelastvisible; }
	// Returns how many seconds have passed since some info about this entity was received
	inline float GetTimeSinceLastInfo() const { return gpGlobals->curtime - m_timelastinfo; }
	// Returns the volume of the last sound made by this entity
	inline int GetVolume() const { return m_volume; }
	// Gets the entity last known position
	inline const Vector& GetLastKnownPosition() const { return m_lastknownposition; }
	// Gets the entity last known Nav Area
	inline const CNavArea* GetLastKnownArea() const { return m_lastknownarea; }

	inline bool IsObsolete() { return !m_handle.IsValid() || gamehelpers->GetHandleEntity(m_handle) == nullptr || GetTimeSinceLastInfo() > NKnownEntity::TIME_FOR_OBSOLETE; }
	// Updates the last known position of this entity
	inline void UpdatePosition(const Vector &newPos)
	{
		constexpr auto NAV_AREA_DIST = 128.0f;

		m_timelastinfo = gpGlobals->curtime;
		m_lastknownposition = newPos;
		m_lastknownarea = TheNavMesh->GetNearestNavArea(newPos, NAV_AREA_DIST);
	}
	// Notify this entity was heard
	inline void NotifyHeard(const int volume, const Vector &soundOrigin)
	{
		m_volume = volume;
		UpdatePosition(soundOrigin);
	}
	// Marks this entity as fully visible
	inline void MarkAsFullyVisible() { m_timelastvisible = gpGlobals->curtime; }

	// true if the given entity is stored on this handle
	bool IsEntity(edict_t* entity);
	// true if the given entity is stored on this handle
	bool IsEntity(const int entity);
private:
	CBaseHandle m_handle; // Handle to the actual entity
	Vector m_lastknownposition; // Last known position of this entity
	CNavArea* m_lastknownarea; // Nav area nearest to the LKP
	float m_timeknown; // Timestamp when this entity became known
	float m_timelastvisible; // Timestamp of when this entity was fully visible to the bot
	float m_timelastinfo; // Timestamp of the last time the bot received some info about this
	int m_volume; // How loud the sound made by this entity was
};

#endif // !SMNAV_BOT_KNOWN_ENTITY_H_
