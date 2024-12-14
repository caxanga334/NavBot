#ifndef NAVBOT_SENSOR_INTERFACE_H_
#define NAVBOT_SENSOR_INTERFACE_H_

#include <vector>
#include <memory>

#include <sdkports/sdk_timers.h>
#include <bot/interfaces/base_interface.h>
#include <bot/interfaces/knownentity.h>

// Sensor interface manages the bot perception (vision and hearing)
class ISensor : public IBotInterface
{
public:
	ISensor(CBaseBot* bot);
	~ISensor() override;

	void OnDifficultyProfileChanged() override;

	// Reset the interface to it's initial state
	void Reset() override;
	// Called at intervals
	void Update() override;
	// Called every server frame
	void Frame() override;
	// Is this entity ignored by the bot?
	virtual bool IsIgnored(edict_t* entity) { return false; }
	virtual bool IsFriendly(edict_t* entity) { return false; }
	virtual bool IsEnemy(edict_t* entity) { return false; }
	// Expensive function that checks if the bot is able to see a given entity, testing for vision blockers, conditions, smokes, etc
	bool IsAbleToSee(edict_t* entity, const bool checkFOV = true);
	bool IsAbleToSee(CBaseExtPlayer& player, const bool checkFOV = true);
	virtual bool IsAbleToSee(CBaseEntity* entity, const bool checkFOV = true);
	virtual bool IsAbleToSee(const Vector& pos, const bool checkFOV = true);
	// Is the bot able to hear this entity
	virtual bool IsAbleToHear(edict_t* entity);
	// Checks if there are obstructions between the bot and the given position
	virtual bool IsLineOfSightClear(const Vector& pos);
	bool IsLineOfSightClear(CBaseExtPlayer& player);
	bool IsLineOfSightClear(edict_t* entity);
	virtual bool IsLineOfSightClear(CBaseEntity* entity);
	virtual bool IsInFieldOfView(const Vector& pos);
	// Is the entity hidden by fog, smoke, etc?
	bool IsEntityHidden(edict_t* entity);
	virtual bool IsEntityHidden(CBaseEntity* entity) { return false; }
	// Is the given position obscured by fog, smoke, etc?
	virtual bool IsPositionObscured(const Vector& pos) { return false; }
	// Adds a known entity to the list
	virtual bool AddKnownEntity(edict_t* entity);
	// Removes a specific entity from the known entity list
	virtual void ForgetKnownEntity(edict_t* entity);
	// Removes all known entities from the list
	virtual void ForgetAllKnownEntities();

	// Returns true if the given entity is already known by the bot
	virtual bool IsKnown(edict_t* entity);
	// Gets the Known entity of the given entity or NULL if not known by the bot
	virtual std::shared_ptr<const CKnownEntity> GetKnown(CBaseEntity* entity);
	std::shared_ptr<const CKnownEntity> GetKnown(edict_t* edict);
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

	// Gets the primary known threat to the bot or NULL if none
	virtual std::shared_ptr<const CKnownEntity> GetPrimaryKnownThreat(const bool onlyvisible = false);
	/**
	 * @brief Gets the quantity of known entities
	 * @param teamindex Filter known entities by team or negative number if don't care
	 * @param onlyvisible Filter known entities by visibility status
	 * @param rangelimit Filter known entities by distance (SQUARED)
	 * @return Quantity of known entities
	*/
	virtual int GetKnownCount(const int teamindex = -1, const bool onlyvisible = false, const float rangelimit = -1.0f);
	// Gets the team index of a known entity since we don't have access to the CBaseEntity functions. Override per mod needs.
	virtual int GetKnownEntityTeamIndex(CKnownEntity* known);
	virtual const CKnownEntity* GetNearestKnown(const int teamindex);

	// Events
	// virtual void OnSound(edict_t* source, const Vector& position, SoundType type, const int volume) override;

	/**
	 * @brief Runs a function on every known entity.
	 * @tparam T A class with void operator() overload with the following parameters: (const CKnownEntity* known)
	 * @param functor Function to run on every known entity
	 */
	template <typename T>
	inline void ForEveryKnownEntity(T functor)
	{
		for (auto& i : m_knownlist)
		{
			const CKnownEntity* known = i.get();
			functor(known);
		}
	}

	/**
	 * @brief Collects filtered known entities into a vector
	 * @tparam T A class with bool operator() overload with the following parameters: (const CKnownEntity* known), return true to add the known entity into the vector
	 * @param outvector Vector to store the known entities
	 * @param functor Function to filter the known entities
	 */
	template <typename T>
	inline void CollectKnownEntities(std::vector<const CKnownEntity*>& outvector, T functor)
	{
		for (auto& ptr : m_knownlist)
		{
			const CKnownEntity* known = ptr.get();
			
			if (functor(known))
			{
				outvector.push_back(known);
			}
		}
	}

protected:
	virtual void UpdateKnownEntities();
	virtual void CollectVisibleEntities(std::vector<edict_t*>& visibleVec);
	virtual void CollectPlayers(std::vector<edict_t*>& visibleVec);
	virtual void CollectNonPlayerEntities(std::vector<edict_t*>& visibleVec);
	virtual void CleanKnownEntities();
	// Same as GetKnown but it's not const, use this internally when updating known entities
	CKnownEntity* FindKnownEntity(edict_t* edict);
	inline std::vector<std::shared_ptr<CKnownEntity>>& GetKnownEntityList() { return m_knownlist; }

	static constexpr auto UPDATE_NONPLAYERS_DELAY = 0.7f;
	static constexpr auto UPDATE_VISION_DELAY = 0.3f;

private:
	std::vector<std::shared_ptr<CKnownEntity>> m_knownlist;
	std::shared_ptr<const CKnownEntity> m_primarythreatcache;
	CountdownTimer m_updateVisibleTimer;
	CountdownTimer m_updateNonPlayerTimer;
	float m_fieldofview;
	float m_coshalfFOV;
	float m_maxvisionrange;
	float m_maxhearingrange;
	float m_minrecognitiontime;
	float m_lastupdatetime;
	IntervalTimer m_threatvisibletimer;

	inline bool IsAwareOf(const std::shared_ptr<CKnownEntity>& known) const
	{
		return known->GetTimeSinceLastVisible() >= GetMinRecognitionTime();
	}
};

#endif // !NAVBOT_SENSOR_INTERFACE_H_

