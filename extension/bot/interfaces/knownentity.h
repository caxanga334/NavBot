#ifndef SMNAV_BOT_KNOWN_ENTITY_H_
#define SMNAV_BOT_KNOWN_ENTITY_H_
#pragma once

#include <string>
#include <sdkports/sdk_ehandle.h>

// Known Entity. Represents an entity that is known to the bot
class CKnownEntity
{
public:
	CKnownEntity(edict_t* entity);
	CKnownEntity(int entity);
	CKnownEntity(CBaseEntity* entity);

	~CKnownEntity();

	static constexpr float time_to_become_obsolete() { return 30.0f; }

	bool operator==(const CKnownEntity& other);
	bool operator==(const CKnownEntity* other);
	inline bool operator!=(const CKnownEntity& other) { return !(*this == other); }

	// Returns true if the entity was completely visible to the bot at some point
	inline bool WasEverFullyVisible() const { return m_timelastvisible > 0.0f; }
	// Returns true if the entity was heard by the bot at some point
	inline bool WasEverHeard() const { return m_timesincelastnoise > 0.0f; }
	// Returns how many seconds have passed since this entity became known to the bot
	float GetTimeSinceBecomeKnown() const;
	// Returns how many seconds have passed since this entity was completely visible
	float GetTimeSinceLastVisible() const;
	// Returns how many seconds have passed since some info about this entity was received
	float GetTimeSinceLastInfo() const;
	// Returns how many seconds have passed since this entity was heard for the last time
	float GetTimeSinceLastHeard() const;
	// Returns the timestamp of the last time this entity was visible
	inline float GetTimeWhenBecameVisible() const { return m_timelastvisible; }
	// Gets the entity last known position
	inline const Vector& GetLastKnownPosition() const { return m_lastknownposition; }
	// Gets the entity last known velocity
	inline const Vector& GetLastKnownVelocity() const { return m_lastknownvelocity; }
	// Gets the entity last known Nav Area
	inline const CNavArea* GetLastKnownArea() const { return m_lastknownarea; }
	// A known entity is obsolete if the stored entity is invalid or if enought time has passed
	bool IsObsolete() const;
	// checks if the actual entity is valid
	bool IsValid() const;
	// Updates the last known position of this entity
	void UpdatePosition();
	// Bot heard this entity
	void UpdateHeard();
	// Marks this entity as fully visible
	void MarkAsFullyVisible();
	// true if the given entity is stored on this handle
	bool IsEntity(edict_t* entity) const;
	// true if the given entity is stored on this handle
	bool IsEntity(const int entity) const;
	// true if the given entity is stored on this handle
	bool IsEntity(CBaseEntity* entity) const { return m_handle.Get() == entity; }

	inline bool WasLastKnownPositionSeen() const { return m_lkpwasseen; }
	inline void MarkLastKnownPositionAsSeen() { m_lkpwasseen = true; }
	inline bool IsVisibleNow() const { return m_visible; }
	inline void MarkAsNotVisible() { m_visible = false; }
	// Returns true if the entity was visible recently
	inline bool WasRecentlyVisible(const float recenttime = 3.0f) const
	{
		if (m_visible)
			return true;

		if (WasEverFullyVisible() && GetTimeSinceLastVisible() < recenttime)
			return true;

		return false;
	}

	edict_t* GetEdict() const { return m_handle.ToEdict(); }
	CBaseEntity* GetEntity() const { return m_handle.Get(); }
	int GetIndex() const { return m_handle.GetEntryIndex(); }
	const std::string& GetEntityClassname() const { return m_classname; }
	bool IsPlayer() const;
	// Draws this known entity instance (debug)
	void DebugDraw(const float duration = 1.0f) const;
	/**
	 * @brief Checks if the known entity classname matches. Supports patterns with '*'.
	 * 
	 * Example: IsEntityOfClassname("npc_*")
	 * @param classname Classname pattern to match.
	 * @return True if the string pattern matches with this known entity classname. False otherwise.
	 */
	const bool IsEntityOfClassname(const char* classname) const;
private:
	CHandle<CBaseEntity> m_handle; // Handle to the actual entity
	std::string m_classname; // Entity classname (cached)
	Vector m_lastknownposition; // Last known position of this entity
	Vector m_lastknownvelocity; // Last known velocity of this entity
	CNavArea* m_lastknownarea; // Nav area nearest to the LKP
	float m_timeknown; // Timestamp when this entity became known
	float m_timelastvisible; // Timestamp of when this entity was fully visible to the bot
	float m_timelastinfo; // Timestamp of the last time the bot received some info about this
	float m_timesincelastnoise; // Timestamp of the last time the bot heard this entity
	bool m_visible; // This entity is visible right now
	bool m_lkpwasseen; // Last known position was seen by the bot
	
	void Init();
};

#endif // !SMNAV_BOT_KNOWN_ENTITY_H_
