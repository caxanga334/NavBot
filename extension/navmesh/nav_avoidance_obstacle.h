#ifndef __NAVBOT_NAV_AVOIDANCE_OBSTACLE_H_
#define __NAVBOT_NAV_AVOIDANCE_OBSTACLE_H_

#include "nav.h"

class CBaseEntity;
class CNavArea;

/**
 * @brief Base interface for avoidance obstacles of the nav mesh.
 */
class INavAvoidanceObstacle
{
public:
	virtual ~INavAvoidanceObstacle() = default;
	/**
	 * @brief Called to determine if this instance is enabled. Disabled instances don't obstruct areas.
	 * @return True if enabled, false otherwise.
	 */
	virtual bool IsEnabled() const = 0;
	// Called to check if this instance is valid. Invalid instances are removed from the nav mesh.
	virtual bool IsValid() const = 0;
	// Called to update this instance state.
	virtual void Update() = 0;
	// Called when the round restarts
	virtual void OnRoundRestart() = 0;
	// Called when the nav mesh's internal data is recomputed
	virtual void OnRecomputeInternalData() = 0;
	/**
	 * @brief Called to determine if the given area is getting obstructed by this obstacle.
	 * @param area Area being tested.
	 * @return True if the given area is obstructed, false otherwise.
	 */
	virtual bool IsObstructing(const CNavArea* area) const = 0;
	virtual float GetObstructionHeight() const = 0;
	virtual const Extent& GetObstructionExtent() const = 0;
};

/**
 * @brief Interface for entity linked avoidance obstacles
 */
class INavEntityAvoidanceObstacle : public INavAvoidanceObstacle
{
public:
	// Returns the entity linked to this avoidance obstacle
	virtual CBaseEntity* GetLinkedEntity() const = 0;
};

class CNavEntityAvoidanceObstacle : public INavEntityAvoidanceObstacle
{
public:
	CNavEntityAvoidanceObstacle(CBaseEntity* entity);
	~CNavEntityAvoidanceObstacle() override;

	bool IsEnabled() const override;
	bool IsValid() const override;
	void Update() override;
	void OnRoundRestart() override;
	virtual void OnRecomputeInternalData() override;
	bool IsObstructing(const CNavArea* area) const override;
	float GetObstructionHeight() const override;
	const Extent& GetObstructionExtent() const override;
	CBaseEntity* GetLinkedEntity() const override;

protected:
	void UpdateAreas() const;

private:
	Extent m_bounds;
	std::uintptr_t m_key; // the nav mesh uses the entity's address as a hash key to avoid duplicate avoidance obstacles
	CHandle<CBaseEntity> m_entity;
	float m_height;
	Vector m_oldPos;
	
	void UpdateEntityData(CBaseEntity* entity);
};

#endif // !__NAVBOT_NAV_AVOIDANCE_OBSTACLE_H_
