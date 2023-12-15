#ifndef SMNAV_BOT_SENSOR_INTERFACE_H_
#define SMNAV_BOT_SENSOR_INTERFACE_H_

#include <vector>

#include <bot/interfaces/base_interface.h>
#include <bot/interfaces/knownentity.h>
#include <util/UtilTrace.h>

class BotSensorTraceFilter : public CTraceFilterSimple
{
public:
	BotSensorTraceFilter(int collisionGroup);

	virtual bool ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask);
};

// Sensor interface manages the bot perception (vision and hearing)
class ISensor : public IBotInterface
{
public:
	ISensor(CBaseBot* bot);
	virtual ~ISensor();

	virtual void OnDifficultyProfileChanged() override;

	// Reset the interface to it's initial state
	virtual void Reset();
	// Called at intervals
	virtual void Update();
	// Called every server frame
	virtual void Frame();
	// Is this entity ignored by the bot?
	virtual bool IsIgnored(edict_t* entity) { return false; }
	// Expensive function that checks if the bot is able to see a given entity, testing for vision blockers, conditions, smokes, etc
	virtual bool IsAbleToSee(edict_t* entity, const bool checkFOV = true);
	virtual bool IsAbleToSee(CBaseExtPlayer& player, const bool checkFOV = true);
	virtual bool IsAbleToSee(const Vector& pos, const bool checkFOV = true);
	// Is the bot able to hear this entity
	virtual bool IsAbleToHear(edict_t* entity);
	// Checks if there are obstructions between the bot and the given position
	virtual bool IsLineOfSightClear(const Vector& pos);
	virtual bool IsLineOfSightClear(CBaseExtPlayer& player);
	virtual bool IsInFieldOfView(const Vector& pos);
	// Is the entity hidden by fog, smoke, etc?
	virtual bool IsEntityHidden(edict_t* entity) { return false; }
	// Is the given position obscured by fog, smoke, etc?
	virtual bool IsPositionObscured(const Vector& pos) { return false; }
	virtual bool AddKnownEntity(edict_t* entity);
	virtual void ForgetKnownEntity(edict_t* entity);
	virtual void ForgetAllKnownEntities();

	// Returns true if the given entity is already known by the bot
	virtual bool IsKnown(edict_t* entity);
	virtual CKnownEntity* GetKnown(edict_t* entity);

	/**
	 * @brief Sets the bot field of view
	 * @param fov The FOV in degrees
	*/
	virtual void SetFieldOfView(const float fov);
	virtual float GetFieldOfView() const { return m_fieldofview; }
	virtual const float GetDefaultFieldOfView() const { return 90.0f; }
	virtual const float GetMaxVisionRange() const { return m_maxvisionrange; }
	virtual const float GetMaxHearingRange() const { return m_maxhearingrange; }
	// Time it takes for the bot to become aware of an entity
	virtual const float GetMinRecognitionTime() const { return m_minrecognitiontime; }

	// Events
	virtual void OnSound(edict_t* source, const Vector& position, SoundType type, const int volume) override;

protected:
	virtual void UpdateKnownEntities();
	virtual void CollectVisibleEntities(std::vector<edict_t*>& visibleVec);
	virtual void CollectPlayers(std::vector<edict_t*>& visibleVec);
	virtual void CollectNonPlayerEntities(std::vector<edict_t*>& visibleVec);
	virtual void CleanKnownEntities();

private:
	std::vector<CKnownEntity> m_knownlist;
	float m_fieldofview;
	float m_coshalfFOV;
	float m_maxvisionrange;
	float m_maxhearingrange;
	float m_minrecognitiontime;
	float m_lastupdatetime;

	inline bool IsAwareOf(const CKnownEntity& known) const
	{
		return known.GetTimeSinceLastVisible() >= GetMinRecognitionTime();
	}
};

#endif // !SMNAV_BOT_SENSOR_INTERFACE_H_

