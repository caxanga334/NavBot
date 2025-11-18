#ifndef NAVBOT_SENSOR_INTERFACE_H_
#define NAVBOT_SENSOR_INTERFACE_H_

#include <array>
#include <vector>
#include <memory>

#include <sdkports/sdk_timers.h>
#include <bot/interfaces/base_interface.h>
#include <bot/interfaces/knownentity.h>
#include <bspfile.h>

// Sensor interface manages the bot perception (vision and hearing)
class ISensor : public IBotInterface
{
public:
	ISensor(CBaseBot* bot);
	~ISensor() override;

	static inline std::array<byte, MAX_MAP_CLUSTERS / 8> s_pvs{};

	// Setups PVS for a given bot.
	static void SetupPVS(CBaseBot* bot);
	// Tests an origin for PVS.
	static bool IsInPVS(const Vector& origin);
	// Tests a bounding box for PVS.
	static bool IsInPVS(const Vector& mins, const Vector& maxs);

	void OnDifficultyProfileChanged() override;

	// Reset the interface to it's initial state
	void Reset() override;
	// Called at intervals
	void Update() override;
	// Called every server frame
	void Frame() override;
	void ShowDebugInformation() const;
	// Is this entity ignored by the bot?
	virtual bool IsIgnored(CBaseEntity* entity) const { return false; }
	virtual bool IsFriendly(CBaseEntity* entity) const { return false; }
	virtual bool IsEnemy(CBaseEntity* entity) const { return false; }
	// Expensive function that checks if the bot is able to see a given entity, testing for vision blockers, conditions, smokes, etc
	bool IsAbleToSee(edict_t* entity, const bool checkFOV = true) const;
	bool IsAbleToSee(const CBaseExtPlayer* player, const bool checkFOV = true) const;
	virtual bool IsAbleToSee(CBaseEntity* entity, const bool checkFOV = true) const;
	virtual bool IsAbleToSee(const Vector& pos, const bool checkFOV = true) const;
	// Is the bot able to hear this entity
	virtual bool IsAbleToHear(edict_t* entity) const;
	// Checks if there are obstructions between the bot and the given position
	virtual bool IsLineOfSightClear(const Vector& pos) const;
	bool IsLineOfSightClear(CBaseExtPlayer& player) const;
	bool IsLineOfSightClear(edict_t* entity) const;
	virtual bool IsLineOfSightClear(CBaseEntity* entity) const;
	virtual bool IsInFieldOfView(const Vector& pos) const;
	// Is the entity hidden by fog, smoke, etc?
	bool IsEntityHidden(edict_t* entity) const;
	virtual bool IsEntityHidden(CBaseEntity* entity) const { return false; }
	// Is the given position obscured by fog, smoke, etc?
	virtual bool IsPositionObscured(const Vector& pos) const { return false; }
	// Adds a known entity to the list
	virtual CKnownEntity* AddKnownEntity(edict_t* entity);
	virtual CKnownEntity* AddKnownEntity(CBaseEntity* entity);
	// Removes a specific entity from the known entity list
	virtual void ForgetKnownEntity(edict_t* entity);
	// Removes all known entities from the list
	virtual void ForgetAllKnownEntities();

	// Returns true if the given entity is already known by the bot
	virtual bool IsKnown(edict_t* entity);
	// Gets the Known entity of the given entity or NULL if not known by the bot
	virtual const CKnownEntity* GetKnown(CBaseEntity* entity);
	const CKnownEntity* GetKnown(edict_t* edict);
	// Updates the position of a known entity or adds it to the list if not known
	virtual void UpdateKnownEntity(edict_t* entity);

	/**
	 * @brief Sets the bot field of view
	 * @param fov The FOV in degrees
	*/
	void SetFieldOfView(const float fov);
	float GetFieldOfView() const { return m_fieldofview; }
	const float GetMaxVisionRange() const { return m_maxvisionrange; }
	const float GetMaxHearingRange() const { return m_maxhearingrange; }
	// Time it takes for the bot to become aware of an entity
	const float GetMinRecognitionTime() const { return m_minrecognitiontime; }
	// Time since a threat was visible
	virtual const float GetTimeSinceVisibleThreat() const;

	static constexpr bool ONLY_VISIBLE_THREATS = true;

	// Gets the primary known threat to the bot or NULL if none
	virtual const CKnownEntity* GetPrimaryKnownThreat(const bool onlyvisible = false);
	/**
	 * @brief Gets the quantity of known entities
	 * @param teamindex Filter known entities by team or negative number if don't care
	 * @param onlyvisible Filter known entities by visibility status
	 * @param rangelimit Filter known entities by distance (SQUARED)
	 * @return Quantity of known entities
	*/
	virtual int GetKnownCount(const int teamindex = -1, const bool onlyvisible = false, const float rangelimit = -1.0f);
	virtual const CKnownEntity* GetNearestKnown(const int teamindex);
	virtual const CKnownEntity* GetNearestHeardKnown(int teamIndex);
	/**
	 * @brief Runs a function on every known entity.
	 * 
	 * Note: This is unfiltered and includes invalid and obsolete known entities.
	 * @tparam T A class with void operator() overload with the following parameters: (const CKnownEntity* known)
	 * @param functor Function to run on every known entity
	 */
	template <typename T>
	inline void ForEveryKnownEntity(T& functor)
	{
		for (auto& i : m_knownlist)
		{
			const CKnownEntity* known = &i;
			functor(known);
		}
	}

	/**
	 * @brief Runs a function on every enemy known entity.
	 * @tparam T A class with void operator() overload with the following parameters: (const CKnownEntity* known)
	 * @param functor Function to run on every known entity
	 */
	template <typename T>
	inline void ForEveryKnownEnemy(T& functor)
	{
		for (auto& i : m_knownlist)
		{
			const CKnownEntity* known = &i;

			if (known->IsValid() && !IsIgnored(known->GetEntity()) && IsEnemy(known->GetEntity()))
			{
				functor(known);
			}
		}
	}

	/**
	 * @brief Runs a function on every allied known entity.
	 * @tparam T A class with void operator() overload with the following parameters: (const CKnownEntity* known)
	 * @param functor Function to run on every known entity
	 */
	template <typename T>
	inline void ForEveryKnownAlly(T& functor)
	{
		for (auto& i : m_knownlist)
		{
			const CKnownEntity* known = &i;

			if (known->IsValid() && !IsIgnored(known->GetEntity()) && IsFriendly(known->GetEntity()))
			{
				functor(known);
			}
		}
	}

	/**
	 * @brief Collects filtered known entities into a vector
	 * @tparam T A class with bool operator() overload with the following parameters: (const CKnownEntity* known), return true to add the known entity into the vector
	 * @param outvector Vector to store the known entities
	 * @param functor Function to filter the known entities
	 */
	template <typename T>
	inline void CollectKnownEntities(std::vector<const CKnownEntity*>& outvector, T& functor)
	{
		for (auto& obj : m_knownlist)
		{
			const CKnownEntity* known = &obj;
			
			if (functor(known))
			{
				outvector.push_back(known);
			}
		}
	}

	// Number of visible allies. The information is updated periodically and cached, it may be out of date.
	inline int GetVisibleAlliesCount() const { return m_statsvisibleallies; }
	// Number of visible enemies. The information is updated periodically and cached, it may be out of date.
	inline int GetVisibleEnemiesCount() const { return m_statsvisibleenemies; }
	// Number of known allies. The information is updated periodically and cached, it may be out of date.
	inline int GetKnownAllyCount() const { return m_statsknownallies; }
	// Shares known entity information another sensor interface
	void ShareKnownEntityList(ISensor* other);
protected:
	virtual void UpdateKnownEntities();
	virtual void CollectVisibleEntities(std::vector<edict_t*>& visibleVec);
	virtual void CollectPlayers(std::vector<edict_t*>& visibleVec);
	virtual void CollectNonPlayerEntities(std::vector<edict_t*>& visibleVec);
	virtual void CleanKnownEntities();
	// Same as GetKnown but it's not const, use this internally when updating known entities
	CKnownEntity* FindKnownEntity(edict_t* edict);
	inline std::vector<CKnownEntity>& GetKnownEntityList() { return m_knownlist; }

	inline bool IsAwareOf(const CKnownEntity* known) const
	{
		return known->GetTimeSinceBecomeKnown() >= GetMinRecognitionTime();
	}

	void UpdateStatistics();
private:
	std::vector<CKnownEntity> m_knownlist;
	const CKnownEntity* m_primarythreatcache;
	CountdownTimer m_updateNonPlayerTimer;
	CountdownTimer m_updateStatisticsTimer;
	float m_cachedNPCupdaterate;
	float m_fieldofview;
	float m_coshalfFOV;
	float m_maxvisionrange;
	float m_maxhearingrange;
	float m_minrecognitiontime;
	float m_lastupdatetime;
	IntervalTimer m_threatvisibletimer;
	int m_statsvisibleallies;
	int m_statsvisibleenemies;
	int m_statsknownallies; // total number of known allies (visible or not)
};

#endif // !NAVBOT_SENSOR_INTERFACE_H_

