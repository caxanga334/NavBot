#ifndef NAVBOT_TEAM_FORTRESS_2_BOT_H_
#define NAVBOT_TEAM_FORTRESS_2_BOT_H_
#pragma once

#include <memory>

#include <sdkports/sdk_timers.h>
#include <mods/tf2/teamfortress2_shareddefs.h>
#include <bot/basebot.h>
#include <bot/interfaces/path/basepath.h>

#include "tf2bot_behavior.h"
#include "tf2bot_controller.h"
#include "tf2bot_movement.h"
#include "tf2bot_sensor.h"

struct edict_t;

class CTF2Bot : public CBaseBot
{
public:
	CTF2Bot(edict_t* edict);
	virtual ~CTF2Bot();

	virtual void TryJoinGame() override;
	virtual void Spawn() override;
	virtual void FirstSpawn() override;

	virtual CTF2BotPlayerController* GetControlInterface() override { return m_tf2controller.get(); }
	virtual CTF2BotMovement* GetMovementInterface() override { return m_tf2movement.get(); }
	virtual CTF2BotSensor* GetSensorInterface() override { return m_tf2sensor.get(); }
	virtual CTF2BotBehavior* GetBehaviorInterface() override { return m_tf2behavior.get(); }
	virtual int GetMaxHealth() const override;
	TeamFortress2::TFClassType GetMyClassType() const;
	TeamFortress2::TFTeam GetMyTFTeam() const;
	void JoinClass(TeamFortress2::TFClassType tfclass) const;
	void JoinTeam() const;
	edict_t* GetItem() const;
	bool IsCarryingAFlag() const;
	edict_t* GetFlagToFetch() const;
	edict_t* GetFlagCaptureZoreToDeliver() const;
	bool IsAmmoLow() const;
private:
	std::unique_ptr<CTF2BotMovement> m_tf2movement;
	std::unique_ptr<CTF2BotPlayerController> m_tf2controller;
	std::unique_ptr<CTF2BotSensor> m_tf2sensor;
	std::unique_ptr<CTF2BotBehavior> m_tf2behavior;
	TeamFortress2::TFClassType m_desiredclass; // class the bot wants
	IntervalTimer m_classswitchtimer;
};

class CTF2BotPathCost : public IPathCost
{
public:
	CTF2BotPathCost(CTF2Bot* bot, RouteType routetype = FASTEST_ROUTE);

	virtual float operator()(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const CFuncElevator* elevator, float length) const override;

private:
	CTF2Bot* m_me;
	RouteType m_routetype;
	float m_stepheight;
	float m_maxjumpheight;
	float m_maxdropheight;
};

#endif // !NAVBOT_TEAM_FORTRESS_2_BOT_H_
