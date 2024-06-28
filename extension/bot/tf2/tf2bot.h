#ifndef NAVBOT_TEAM_FORTRESS_2_BOT_H_
#define NAVBOT_TEAM_FORTRESS_2_BOT_H_
#pragma once

#include <memory>

#include <sdkports/sdk_timers.h>
#include <mods/tf2/teamfortress2_shareddefs.h>
#include <bot/basebot.h>
#include <bot/interfaces/path/basepath.h>
#include <sdkports/sdk_ehandle.h>

#include "tf2bot_behavior.h"
#include "tf2bot_controller.h"
#include "tf2bot_movement.h"
#include "tf2bot_sensor.h"
#include "tf2bot_spymonitor.h"
#include "tf2bot_upgrades.h"

struct edict_t;

class CTF2Bot : public CBaseBot
{
public:
	CTF2Bot(edict_t* edict);
	~CTF2Bot() override;

	// Reset the bot to it's initial state
	void Reset() override;
	// Function called at intervals to run the AI 
	void Update() override;
	// Function called every server frame to run the AI
	void Frame() override;

	void TryJoinGame() override;
	void Spawn() override;
	void FirstSpawn() override;

	CTF2BotPlayerController* GetControlInterface() const override { return m_tf2controller.get(); }
	CTF2BotMovement* GetMovementInterface() const override { return m_tf2movement.get(); }
	CTF2BotSensor* GetSensorInterface() const override { return m_tf2sensor.get(); }
	CTF2BotBehavior* GetBehaviorInterface() const override { return m_tf2behavior.get(); }
	CTF2BotSpyMonitor* GetSpyMonitorInterface() const { return m_tf2spymonitor.get(); }
	int GetMaxHealth() const override;

	TeamFortress2::TFClassType GetMyClassType() const;
	TeamFortress2::TFTeam GetMyTFTeam() const;
	void JoinClass(TeamFortress2::TFClassType tfclass) const;
	void JoinTeam() const;
	edict_t* GetItem() const;
	bool IsCarryingAFlag() const;
	edict_t* GetFlagToFetch() const;
	edict_t* GetFlagCaptureZoreToDeliver() const;
	bool IsAmmoLow() const;
	edict_t* MedicFindBestPatient() const;
	edict_t* GetMySentryGun() const;
	edict_t* GetMyDispenser() const;
	edict_t* GetMyTeleporterEntrance() const;
	edict_t* GetMyTeleporterExit() const;
	void SetMySentryGun(edict_t* entity);
	void SetMyDispenser(edict_t* entity);
	void SetMyTeleporterEntrance(edict_t* entity);
	void SetMyTeleporterExit(edict_t* entity);
	void FindMyBuildings();
	int GetCurrency() const;
	bool IsInUpgradeZone() const;

	inline void BeginBuilding(TeamFortress2::TFObjectType type, TeamFortress2::TFObjectMode mode)
	{
		switch (type)
		{
		case TeamFortress2::TFObject_Dispenser:
			DelayedFakeClientCommand("build 0");
			break;
		case TeamFortress2::TFObject_Teleporter:
		{
			if (mode == TeamFortress2::TFObjectMode::TFObjectMode_Entrance)
			{
				DelayedFakeClientCommand("build 1");
			}
			else
			{
				DelayedFakeClientCommand("build 3");
			}

			break;
		}
		case TeamFortress2::TFObject_Sentry:
			DelayedFakeClientCommand("build 2");
			break;
		default:
			break;
		}
	}

	/**
	 * @brief Toggles the ready status in Tournament modes. Also used in Mann vs Machine.
	 * @param isready Is the bot ready?
	 */
	void ToggleTournamentReadyStatus(bool isready = true) const;
	bool TournamentIsReady() const;

	CTF2BotUpgradeManager& GetUpgradeManager() { return m_upgrademan; }

	bool IsInsideSpawnRoom() const;

private:
	std::unique_ptr<CTF2BotMovement> m_tf2movement;
	std::unique_ptr<CTF2BotPlayerController> m_tf2controller;
	std::unique_ptr<CTF2BotSensor> m_tf2sensor;
	std::unique_ptr<CTF2BotBehavior> m_tf2behavior;
	std::unique_ptr<CTF2BotSpyMonitor> m_tf2spymonitor;
	TeamFortress2::TFClassType m_desiredclass; // class the bot wants
	IntervalTimer m_classswitchtimer;
	CBaseHandle m_mySentryGun;
	CBaseHandle m_myDispenser;
	CBaseHandle m_myTeleporterEntrance;
	CBaseHandle m_myTeleporterExit;
	CTF2BotUpgradeManager m_upgrademan;

	static constexpr float medic_patient_health_critical_level() { return 0.3f; }
	static constexpr float medic_patient_health_low_level() { return 0.6f; }
};

class CTF2BotPathCost : public IPathCost
{
public:
	CTF2BotPathCost(CTF2Bot* bot, RouteType routetype = FASTEST_ROUTE);

	float operator()(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const NavSpecialLink* link, const CFuncElevator* elevator, float length) const override;

private:
	CTF2Bot* m_me;
	RouteType m_routetype;
	float m_stepheight;
	float m_maxjumpheight;
	float m_maxdropheight;
	float m_maxdjheight; // max double jump height
	float m_maxgapjumpdistance;
	bool m_candoublejump;
};

#endif // !NAVBOT_TEAM_FORTRESS_2_BOT_H_
