#ifndef __NAVBOT_ZPSBOT_TASKS_PICKUP_ITEM_H_
#define __NAVBOT_ZPSBOT_TASKS_PICKUP_ITEM_H_

class CZPSBotPickupItemTask : public AITask<CZPSBot>
{
public:
	CZPSBotPickupItemTask(CBaseEntity* item);

	TaskResult<CZPSBot> OnTaskUpdate(CZPSBot* bot) override;

	const char* GetName() const override { return "PickupItem"; }

private:
	CMeshNavigator m_nav;
	CPathFailCounter m_counter;
	CHandle<CBaseEntity> m_item;
	bool m_inventoryCheckDone;
};

#endif // !__NAVBOT_ZPSBOT_TASKS_PICKUP_ITEM_H_
