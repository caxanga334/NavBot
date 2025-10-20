#ifndef __NAVBOT_ZPSBOT_H_
#define __NAVBOT_ZPSBOT_H_

#include <bot/basebot.h>
#include <bot/bot_pathcosts.h>
#include <mods/zps/zps_shareddefs.h>
#include "zpsbot_inventory.h"
#include "zpsbot_movement.h"
#include "zpsbot_sensor.h"
#include "tasks/zpsbot_main_task.h"

class CZPSBot : public CBaseBot
{
public:
	using CZPSBotBehavior = CDefaultBehaviorImplementation<CZPSBot, AITaskManager<CZPSBot>, CZPSBotMainTask>;

	CZPSBot(edict_t* edict);
	~CZPSBot() override;

	CZPSBotBehavior* GetBehaviorInterface() const override { return m_zpsbehavior.get(); }
	CZPSBotSensor* GetSensorInterface() const override { return m_zpssensor.get(); }
	CZPSBotInventory* GetInventoryInterface() const override { return m_zpsinventory.get(); }
	CZPSBotMovement* GetMovementInterface() const override { return m_zpsmovement.get(); }

	zps::ZPSTeam GetMyZPSTeam() const;

private:
	std::unique_ptr<CZPSBotBehavior> m_zpsbehavior;
	std::unique_ptr<CZPSBotSensor> m_zpssensor;
	std::unique_ptr<CZPSBotInventory> m_zpsinventory;
	std::unique_ptr<CZPSBotMovement> m_zpsmovement;
};

class CZPSBotPathCost : public IGroundPathCost
{
public:
	CZPSBotPathCost(CZPSBot* bot, RouteType type = DEFAULT_ROUTE);

	float operator()(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator, float length) const override;
	void SetRouteType(RouteType type) override { m_routetype = type; }
	RouteType GetRouteType() const override { return m_routetype; }

private:
	CZPSBot* m_me;
	RouteType m_routetype;
};

#endif // !__NAVBOT_ZPSBOT_H_
