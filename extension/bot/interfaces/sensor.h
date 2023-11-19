#ifndef SMNAV_BOT_SENSOR_INTERFACE_H_
#define SMNAV_BOT_SENSOR_INTERFACE_H_

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

	// Reset the interface to it's initial state
	virtual void Reset();
	// Called at intervals
	virtual void Update();
	// Called every server frame
	virtual void Frame();
	// Is this entity ignored by the bot?
	virtual bool IsIgnored(edict_t* entity) { return false; }
	// Expensive function that checks if the bot is able to see a given entity, testing for vision blockers, conditions, smokes, etc
	virtual bool IsAbleToSee(edict_t* entity);
	// Is the bot able to hear this entity
	virtual bool IsAbleToHear(edict_t* entity);
	// Checks if there are obstructions between the bot and the given position
	virtual bool IsLineOfSightClear(const Vector& pos);

	virtual bool IsInFieldOfView(const Vector& pos);

	/**
	 * @brief Sets the bot field of view
	 * @param fov The FOV in degrees
	*/
	virtual void SetFieldOfView(const float fov);
protected:
	virtual void ScanForEntities();

private:
	std::vector<CKnownEntity> m_knownlist;
	float m_fieldofview;
	float m_coshalfFOV;
};

#endif // !SMNAV_BOT_SENSOR_INTERFACE_H_

