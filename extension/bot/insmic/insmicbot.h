#ifndef __NAVBOT_INSMIC_BOT_H_
#define __NAVBOT_INSMIC_BOT_H_

#include <bot/basebot.h>
#include <bot/bot_pathcosts.h>
#include <mods/insmic/insmic_shareddefs.h>
#include "insmicbot_combat.h"
#include "insmicbot_inventory.h"
#include "insmicbot_movement.h"
#include "insmicbot_playercontrol.h"
#include "insmicbot_sensor.h"
#include "tasks/insmicbot_main_task.h"

class CInsMICBot : public CBaseBot
{
public:
	using CInsMICBotBehavior = CDefaultBehaviorImplementation<CInsMICBot, AITaskManager<CInsMICBot>, CInsMICBotMainTask>;

	CInsMICBot(edict_t* edict);

	// Returns the bot's insurgency team
	insmic::InsMICTeam GetMyInsurgencyTeam() const
	{
		return static_cast<insmic::InsMICTeam>(GetCurrentTeamIndex());
	}

	CInsMICBotMovement* GetMovementInterface() const override { return m_insmicmovement.get(); }
	CInsMICBotSensor* GetSensorInterface() const override { return m_insmicsensor.get(); }
	CInsMICBotBehavior* GetBehaviorInterface() const override { return m_insmicbehavior.get(); }
	CInsMICBotInventory* GetInventoryInterface() const override { return m_insmicinventory.get(); }
	CInsMICBotCombat* GetCombatInterface() const override { return m_insmiccombat.get(); }
	// Overriden for Insurgency since m_flModelScale doesn't exists.
	float GetModelScale() const override { return 1.0f; }
	CBaseEntity* GetActiveWeapon() const override;
	bool HasJoinedGame() override;
	void TryJoinGame() override;

private:
	std::unique_ptr<CInsMICBotSensor> m_insmicsensor;
	std::unique_ptr<CInsMICBotBehavior> m_insmicbehavior;
	std::unique_ptr<CInsMICBotInventory> m_insmicinventory;
	std::unique_ptr<CInsMICBotMovement> m_insmicmovement;
	std::unique_ptr<CInsMICBotCombat> m_insmiccombat;
};

class CInsMICBotPathCost : public IGroundPathCost
{
public:
	CInsMICBotPathCost(CInsMICBot* bot, RouteType type = RouteType::DEFAULT_ROUTE);

	float operator()(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator, float length) const override;
	void SetRouteType(RouteType type) override { m_routetype = type; }
	RouteType GetRouteType() const override { return m_routetype; }

private:
	CInsMICBot* m_me;
	RouteType m_routetype;
};

#endif // !__NAVBOT_INSMIC_BOT_H_
