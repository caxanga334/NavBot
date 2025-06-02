#ifndef NAVBOT_BLACKMESA_BOT_H_
#define NAVBOT_BLACKMESA_BOT_H_

#include <memory>
#include <mods/blackmesa/blackmesa_shareddefs.h>
#include <bot/basebot.h>
#include <bot/interfaces/path/basepath.h>
#include "bmbot_movement.h"
#include "bmbot_sensor.h"
#include "bmbot_inventory.h"
#include "bmbot_behavior.h"

class CBlackMesaBot : public CBaseBot
{
public:
	CBlackMesaBot(edict_t* edict);
	~CBlackMesaBot() override;

	bool HasJoinedGame() override;
	void TryJoinGame() override;

	void FirstSpawn() override;

	CBlackMesaBotMovement* GetMovementInterface() const override { return m_bmmovement.get(); }
	CBlackMesaBotSensor* GetSensorInterface() const override { return m_bmsensor.get(); }
	CBlackMesaBotBehavior* GetBehaviorInterface() const override { return m_bmbehavior.get(); }
	CBlackMesaBotInventory* GetInventoryInterface() const override { return m_bminventory.get(); }
	// The bot's team
	blackmesa::BMTeam GetMyBMTeam() const;
	// Amount of armor this bot have
	int GetArmor() const;
	static constexpr int GetMaxArmor() { return 100; }
	float GetArmorPercentage() const { return static_cast<float>(GetArmor()) / static_cast<float>(GetMaxArmor()); }
	bool HasLongJump() const;

private:
	std::unique_ptr<CBlackMesaBotMovement> m_bmmovement;
	std::unique_ptr<CBlackMesaBotSensor> m_bmsensor;
	std::unique_ptr<CBlackMesaBotBehavior> m_bmbehavior;
	std::unique_ptr<CBlackMesaBotInventory> m_bminventory;
};

class CBlackMesaBotPathCost : public IPathCost
{
public:
	CBlackMesaBotPathCost(CBlackMesaBot* bot, RouteType routetype = FASTEST_ROUTE);

	float operator()(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator, float length) const override;

	void SetRouteType(RouteType type) override { m_routetype = type; }
private:
	CBlackMesaBot* m_me;
	RouteType m_routetype;
	float m_stepheight;
	float m_maxjumpheight;
	float m_maxdropheight;
	float m_maxdjheight; // max double jump height
	float m_maxgapjumpdistance;
	bool m_candoublejump;
	bool m_canblastjump;
};

#endif // !NAVBOT_BLACKMESA_BOT_H_
