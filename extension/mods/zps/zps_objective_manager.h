#ifndef __NAVBOT_MOD_ZPS_OBJECTIVE_MANAGER_H_
#define __NAVBOT_MOD_ZPS_OBJECTIVE_MANAGER_H_

class CZPSObjectiveManager
{
public:
	CZPSObjectiveManager();
	~CZPSObjectiveManager();

	enum ObjectiveTypes
	{
		OBJECTIVE_NONE = 0,
		OBJECTIVE_MOVETO, // move to a position
		OBJECTIVE_USE_BUTTON, // press a button
		OBJECTIVE_FIND_ITEM, // find item_generic
		OBJECTIVE_USE_ITEM, // use item_generic
		OBJECTIVE_DROP_ITEM, // drop item_generic at another entity
		OBJECTIVE_DESTROY_ENTITY, // destroy an entity

		MAX_OBJECTIVE_TYPES
	};

	void Reset();

	ObjectiveTypes GetCurrentObjective() const { return m_currentObjective; }
	void SetCurrentObjective(ObjectiveTypes objective) { m_currentObjective = objective; }
	bool IsObjectiveSet() const { return m_currentObjective != ObjectiveTypes::OBJECTIVE_NONE; }
	void SetMoveGoal(const Vector& goal) { m_moveTo = goal; }
	const Vector& GetMoveGoal() const { return m_moveTo; }
	CBaseEntity* GetUseButton() const { return m_useButton.Get(); }
	void SetUseButton(CBaseEntity* entity) { m_useButton = entity; }
	const std::string& GetItemSearchID() const { return m_itemID; }
	void SetItemSearchID(const char* str) { m_itemID.assign(str); }
	CBaseEntity* GetUseItemTarget() const { return m_itemUseTarget.Get(); }
	void SetUseItemTarget(CBaseEntity* entity) { m_itemUseTarget = entity; }
	float GetDetectionRadius() const { return m_detectionRadius; }
	void SetDetectionRadius(float v) { m_detectionRadius = v; }
	CBaseEntity* GetGenericTargetEntity() const { return m_genericTarget.Get(); }
	void SetGenericTargetEntity(CBaseEntity* entity) { m_genericTarget = entity; }

private:
	ObjectiveTypes m_currentObjective;
	Vector m_moveTo;
	CHandle<CBaseEntity> m_useButton;
	std::string m_itemID; // id of the item_generic to find
	CHandle<CBaseEntity> m_itemUseTarget; // entity to use the item_generic at
	CHandle<CBaseEntity> m_genericTarget; // general purpose generic target entity
	float m_detectionRadius;
};


#endif // !__NAVBOT_MOD_ZPS_OBJECTIVE_MANAGER_H_
