#ifndef NAVBOT_DODSBOT_H_
#define NAVBOT_DODSBOT_H_

#include <bot/basebot.h>
#include "dodsbot_sensor.h"
#include "dodsbot_behavior.h"
#include "dodsbot_inventory.h"
#include "dodsbot_movement.h"
#include "dodsbot_combat.h"
#include "dodsbot_playercontrol.h"
#include <mods/dods/dods_shareddefs.h>
#include <bot/interfaces/path/basepath.h>

class CDoDSBot : public CBaseBot
{
public:
	CDoDSBot(edict_t* edict);
	~CDoDSBot() override;

	CDoDSBotPlayerController* GetControlInterface() const override { return m_dodcontrol.get(); }
	CDoDSBotSensor* GetSensorInterface() const override { return m_dodsensor.get(); }
	CDoDSBotBehavior* GetBehaviorInterface() const override { return m_dodbehavior.get(); }
	CDoDSBotMovement* GetMovementInterface() const override { return m_dodmovement.get(); }
	CDoDSBotInventory* GetInventoryInterface() const override { return m_dodinventory.get(); }
	CDoDSBotCombat* GetCombatInterface() const override { return m_dodcombat.get(); }

	bool HasJoinedGame() override;
	void TryJoinGame() override;
	void Spawn() override;
	void FirstSpawn() override;
	dayofdefeatsource::DoDTeam GetMyDoDTeam() const;
	dayofdefeatsource::DoDClassType GetMyClassType() const;
	// Control point index the bot is currently at. returns -1 if not at one.
	int GetControlPointIndex() const;
	// True if the bot is touching a capture area.
	bool IsCapturingAPoint() const;
	bool IsProne() const;
	bool IsSprinting() const;
	bool IsPlantingBomb() const;
	bool IsDefusingBomb() const;
	bool CanDropAmmo() const { return !m_droppedAmmo; } // ammo can only be dropped once per life
private:
	std::unique_ptr<CDoDSBotSensor> m_dodsensor;
	std::unique_ptr<CDoDSBotBehavior> m_dodbehavior;
	std::unique_ptr<CDoDSBotMovement> m_dodmovement;
	std::unique_ptr<CDoDSBotInventory> m_dodinventory;
	std::unique_ptr<CDoDSBotCombat> m_dodcombat;
	std::unique_ptr<CDoDSBotPlayerController> m_dodcontrol;
	bool m_droppedAmmo; // has the bot dropped ammo?
};

class CDoDSBotPathCost : public IPathCost
{
public:
	CDoDSBotPathCost(CDoDSBot* bot, RouteType type = RouteType::FASTEST_ROUTE);

	float operator()(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator, float length) const override;

	void SetRouteType(RouteType type) override { m_routetype = type; }
private:
	CDoDSBot* m_me;
	RouteType m_routetype;
	float m_stepheight;
	float m_maxjumpheight;
	float m_maxdropheight;
	float m_maxgapjumpdistance;
	bool m_hasbomb;
};

#endif // !NAVBOT_DODSBOT_H_
