#ifndef __NAVMESH_NAV_BLOCKER_H_
#define __NAVMESH_NAV_BLOCKER_H_

#include <vector>

class CNavArea;

/**
 * @brief Interface for dynamic run-time nav area blockers
 */
class INavBlocker
{
public:
	virtual ~INavBlocker() = default;

	template <typename NavAreaClass>
	class NotifyBlockerDestruction
	{
	public:
		NotifyBlockerDestruction(INavBlocker* me)
		{
			m_me = me;
		}

		bool operator()(NavAreaClass* area)
		{
			area->UnregisterNavBlocker(m_me);
			return true;
		}

		INavBlocker* m_me;
	};

	// Called to register this instance to the nav mesh. This "actives" your blocker and adds it to the nav mesh list.
	void Register();
	// Called to unregister this instance to the nav mesh. This will also unlink this blockers from the nav areas.
	void Unregister();
	// Notifies nav areas that we are being deleted.
	void NotifyDestruction();
	// Called by the nav mesh after the blocker is registered to it.
	virtual void PostRegister() = 0;
	// Called to determine if this nav blocker instance is valid. If this return false, the nav mesh will destroy this instance.
	virtual bool IsValid() = 0;
	// Called to update the state of this nav blocker
	virtual void Update() = 0;
	// Called when the round has restarted
	virtual void OnRoundRestart() = 0;
	// Called to get the blocked/unblocked state
	virtual bool IsBlocked(int teamID) = 0;
	// Called when the nav mesh is recomputing internal data
	virtual void OnRecomputeInternalData() = 0;
	// Return true if this blocker should be deleted when the nav mesh internal data is recomputed
	virtual bool RemoveOnRecompute() = 0;
	// The name of this blocker, for debugging purposes
	virtual const char* GetName() = 0;
	// Prints debug information to the console.
	virtual void PrintDebugInfo() = 0;
};

/**
 * @brief Basic implementation of the nav blocker.
 * 
 * PostRegister automatically registers the blocker with the nav areas added to the 'm_areas' vector.
 * 
 * Automatically unregistered by the destructor.
 * @tparam AreaType Nav area class.
 */
template <typename AreaType>
class CNavBlocker : public INavBlocker
{
public:
	CNavBlocker() {}
	~CNavBlocker() override
	{
		NotifyDestruction();
	}

	void PostRegister() override
	{
		for (AreaType* area : m_areas)
		{
			area->RegisterNavBlocker(this);
		}
	}
	bool IsValid() override { return true; }
	void Update() override {}
	void OnRoundRestart() override {}
	bool IsBlocked(int teamID) { return true; }
	void OnRecomputeInternalData() override {}
	bool RemoveOnRecompute() override { return false; }
	const char* GetName() override { return "CNavBlocker"; }
	void PrintDebugInfo() override {}
	bool IsAreaVectorEmpty() const { return m_areas.empty(); }

protected:
	std::vector<AreaType*> m_areas;

	/**
	 * @brief Add areas that overlaps an entity's extent to the areas vector.
	 * @param entity Entity to get the extent of.
	 * @param z_offset Offset to apply on the Z axis.
	 * @param xy_offset Offset to apply on the X/Y axis.
	 */
	void AddAreasFromEntityExtent(CBaseEntity* entity, const float z_offset = 0.0f, const float xy_offset = 0.0f)
	{
		Extent entityExtent(entity);
		entityExtent.lo.x -= xy_offset;
		entityExtent.lo.y -= xy_offset;
		entityExtent.lo.z -= z_offset;
		entityExtent.hi.x += xy_offset;
		entityExtent.hi.y += xy_offset;
		entityExtent.hi.z += z_offset;
		TheNavMesh->CollectAreasOverlappingExtent(entityExtent, m_areas);
	}
	/**
	 * @brief Add areas that overlaps an entity's extent and intersects with the entity (determined via raycast) to the area vector.
	 * @param entity Entity to get the extent of.
	 * @param z_offset Offset to apply on the Z axis.
	 * @param xy_offset Offset to apply on the X/Y axis.
	 */
	void AddAreasTouchingEntity(CBaseEntity* entity, const float z_offset = 0.0f, const float xy_offset = 0.0f)
	{
		Extent entityExtent(entity);
		entityExtent.lo.x -= xy_offset;
		entityExtent.lo.y -= xy_offset;
		entityExtent.lo.z -= z_offset;
		entityExtent.hi.x += xy_offset;
		entityExtent.hi.y += xy_offset;
		entityExtent.hi.z += z_offset;
		TheNavMesh->CollectAreasTouchingEntity(entity, m_areas);
	}
};

#endif