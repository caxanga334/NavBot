#ifndef NAVBOT_DODSBOT_H_
#define NAVBOT_DODSBOT_H_

#include <bot/basebot.h>
#include "dodsbot_sensor.h"
#include "dodsbot_behavior.h"
#include <mods/dods/dods_shareddefs.h>
#include <bot/interfaces/path/basepath.h>

class CDoDSBot : public CBaseBot
{
public:
	CDoDSBot(edict_t* edict);
	~CDoDSBot() override;

	CDoDSBotSensor* GetSensorInterface() const override { return m_dodsensor.get(); }
	CDoDSBotBehavior* GetBehaviorInterface() const override { return m_dodbehavior.get(); }

	bool HasJoinedGame() override;
	void TryJoinGame() override;
	dayofdefeatsource::DoDTeam GetMyDoDTeam() const;
	dayofdefeatsource::DoDClassType GetMyClassType() const;


private:
	std::unique_ptr<CDoDSBotSensor> m_dodsensor;
	std::unique_ptr<CDoDSBotBehavior> m_dodbehavior;
};

class CDoDSBotPathCost : public IPathCost
{
public:
	CDoDSBotPathCost(CDoDSBot* bot, RouteType type = RouteType::FASTEST_ROUTE);

	float operator()(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator, float length) const override;

private:
	CDoDSBot* m_me;
	RouteType m_routetype;
	float m_stepheight;
	float m_maxjumpheight;
	float m_maxdropheight;
	float m_maxgapjumpdistance;
};

#endif // !NAVBOT_DODSBOT_H_
