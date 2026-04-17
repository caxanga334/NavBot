#ifndef __NAVBOT_HL1MP_BOT_H_
#define __NAVBOT_HL1MP_BOT_H_

#include "../basebot.h"
#include "../bot_pathcosts.h"
#include "hl1mp_bot_movement.h"
#include "hl1mp_bot_sensor.h"
#include "hl1mp_bot_squad.h"
#include "tasks/hl1mp_bot_main_task.h"

class CHL1MPBot : public CBaseBot
{
public:
	CHL1MPBot(edict_t* edict);
	~CHL1MPBot() override;

	using CHL1MPBotBehavior = CDefaultBehaviorImplementation<CHL1MPBot, AITaskManager<CHL1MPBot>, CHL1MPBotMainTask>;

	CHL1MPBotSensor* GetSensorInterface() const override { return m_hl1mpsensor.get(); }
	CHL1MPBotBehavior* GetBehaviorInterface() const override { return m_hl1mpbehavior.get(); }
	CHL1MPBotMovement* GetMovementInterface() const override { return m_hl1mpmovement.get(); }
	CHL1MPBotSquad* GetSquadInterface() const override { return m_hl1mpsquad.get(); }
	bool HasJoinedGame() override { return true; } // HL1DM doesn't appear to have team selection or spec
	bool HasLongJumpModule() const;

private:
	std::unique_ptr<CHL1MPBotSensor> m_hl1mpsensor;
	std::unique_ptr<CHL1MPBotBehavior> m_hl1mpbehavior;
	std::unique_ptr<CHL1MPBotMovement> m_hl1mpmovement;
	std::unique_ptr<CHL1MPBotSquad> m_hl1mpsquad;
};

class CHL1MPBotPathCost : public CBasicPathCost<CHL1MPBot>
{
public:
	CHL1MPBotPathCost(CHL1MPBot* bot, RouteType type = RouteType::DEFAULT_ROUTE) :
		CBasicPathCost<CHL1MPBot>(bot, type)
	{
		SetIgnoreDanger(true); // danger is shared per team and all bots will be in the same team
	}

};

#endif // !__NAVBOT_HL1MP_BOT_H_
