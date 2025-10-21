#ifndef NAVBOT_TEAM_FORTRESS_2_BOT_H_
#define NAVBOT_TEAM_FORTRESS_2_BOT_H_
#pragma once

#include <memory>

#include <sdkports/sdk_timers.h>
#include <sdkports/sdk_ehandle.h>
#include <mods/tf2/teamfortress2_shareddefs.h>
#include <bot/basebot.h>
#include <bot/bot_pathcosts.h>

#include "tf2bot_behavior.h"
#include "tf2bot_controller.h"
#include "tf2bot_movement.h"
#include "tf2bot_sensor.h"
#include "tf2bot_inventory.h"
#include "tf2bot_combat.h"
#include "tf2bot_spymonitor.h"
#include "tf2bot_upgrades.h"

struct edict_t;
class CTFWaypoint;

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
	CTF2BotInventory* GetInventoryInterface() const { return m_tf2inventory.get(); }
	CTF2BotCombat* GetCombatInterface() const { return m_tf2combat.get(); }
	int GetMaxHealth() const override;

protected:
	bool CanBeAutoBalanced(bool& useOriginal) override;
public:

	TeamFortress2::TFClassType GetMyClassType() const;
	TeamFortress2::TFTeam GetMyTFTeam() const;
	void JoinClass(TeamFortress2::TFClassType tfclass) const;
	void JoinTeam() const;
	edict_t* GetItem() const;
	bool IsCarryingAFlag() const;
	edict_t* GetFlagToFetch() const;
	edict_t* GetFlagToDefend(bool stolenOnly = false, bool ignoreHome = false) const;
	edict_t* GetFlagCaptureZoreToDeliver() const;
	void FindMyBuildings();
	bool IsDisguised() const;
	bool IsCloaked() const;
	bool IsInvisible() const;
	int GetCurrency() const;
	bool IsInUpgradeZone() const;
	bool IsUsingSniperScope() const;
	/**
	 * @brief Checks if the bot line of fire is clear by performing a trace from the bot's eye origin to the given position.
	 * 
	 * Overriden for TF2 because TF2 uses MASK_SOLID instead of MASK_SHOT for hitscan bullets.
	 * @param to Trace end position.
	 * @return True if there are no obstructions that blocks bullets. False otherwise.
	 */
	bool IsLineOfFireClear(const Vector& to) const override;

	inline void BeginBuilding(TeamFortress2::TFObjectType type, TeamFortress2::TFObjectMode mode)
	{
		switch (type)
		{
		case TeamFortress2::TFObject_Dispenser:
			DelayedFakeClientCommand("build 0 0");
			break;
		case TeamFortress2::TFObject_Teleporter:
		{
			if (mode == TeamFortress2::TFObjectMode::TFObjectMode_Entrance)
			{
				DelayedFakeClientCommand("build 1 0");
			}
			else
			{
				DelayedFakeClientCommand("build 1 1");
			}

			break;
		}
		case TeamFortress2::TFObject_Sentry:
			DelayedFakeClientCommand("build 2 0");
			break;
		default:
			break;
		}
	}

	/**
	 * @brief Makes engineers destroy a building.
	 * @param type Object type
	 * @param mode Object mode (for teleporters)
	 */
	inline void DestroyBuilding(TeamFortress2::TFObjectType type, TeamFortress2::TFObjectMode mode)
	{
		switch (type)
		{
		case TeamFortress2::TFObject_Dispenser:
			DelayedFakeClientCommand("destroy 0 0");
			break;
		case TeamFortress2::TFObject_Teleporter:
		{
			if (mode == TeamFortress2::TFObjectMode::TFObjectMode_Entrance)
			{
				DelayedFakeClientCommand("destroy 1 0");
			}
			else
			{
				DelayedFakeClientCommand("destroy 1 1");
			}

			break;
		}
		case TeamFortress2::TFObject_Sentry:
			DelayedFakeClientCommand("destroy 2 0");
			break;
		default:
			break;
		}
	}

	// Selects a class to disguese as
	void Disguise(bool myTeam = false);
	void DisguiseAs(TeamFortress2::TFClassType classtype, bool myTeam = false);
	inline void DisguiseAs(TeamFortress2::TFClassType classtype, TeamFortress2::TFTeam team)
	{
		if (team == TeamFortress2::TFTeam_Red)
		{
			SetImpulseCommand(220 + static_cast<int>(classtype));
		}
		else
		{
			SetImpulseCommand(230 + static_cast<int>(classtype));
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

	CBaseEntity* GetMySentryGun() const { return m_mySentryGun.Get(); }
	CBaseEntity* GetMyDispenser() const { return m_myDispenser.Get(); }
	CBaseEntity* GetMyTeleporterEntrance() const { return m_myTeleporterEntrance.Get(); }
	CBaseEntity* GetMyTeleporterExit() const { return m_myTeleporterExit.Get(); }

	// Retrieves the bot's buildings. All parameters must be used.
	void GetMyBuildings(CBaseEntity** sentry, CBaseEntity** dispenser, CBaseEntity** entrance, CBaseEntity** exit) const
	{
		*sentry = m_mySentryGun.Get();
		*dispenser = m_myDispenser.Get();
		*entrance = m_myTeleporterEntrance.Get();
		*exit = m_myTeleporterExit.Get();
	}

	void SetMySentryGun(CBaseEntity* entity) { m_mySentryGun = entity; }
	void SetMyDispenser(CBaseEntity* entity) { m_myDispenser = entity; }
	void SetMyTeleporterEntrance(CBaseEntity* entity) { m_myTeleporterEntrance = entity; }
	void SetMyTeleporterExit(CBaseEntity* entity) { m_myTeleporterExit = entity; }
	// Returns true if this entity was built by this bot
	bool IsMyBuilding(CBaseEntity* entity);

	/**
	 * @brief Gets the spy cloak meter percentage retrieved by reading the 'm_flCloakMeter' property.
	 * @return Cloak percentage (range: 0 - 100) or -1 if we don't have a valid pointer to the property.
	 */
	float GetCloakPercentage() const
	{
		if (m_cloakMeter)
		{
			return *m_cloakMeter;
		}
		
		return -1.0f;
	}

	bool IsCarryingObject() const;
	CBaseEntity* GetObjectBeingCarriedByMe() const;
	// Makes the bot run the upgrade logic on the next Update call
	void DoMvMUpgrade() { m_doMvMUpgrade = true; }
	void SendVoiceCommand(TeamFortress2::VoiceCommandsID id);
	/**
	 * @brief Gets the time left to capture the point before we lose the game.
	 * 
	 * Returns a very large number on game modes without timer (or if the timer wasn't found).
	 * @return Time left on the timer.
	 */
	float GetTimeLeftToCapture() const;
	bool IsCarryingThePassTimeJack() const;
	// Makes the bot use the weapon's taunt
	void DoWeaponTaunt() { DelayedFakeClientCommand("weapon_taunt"); }
private:
	std::unique_ptr<CTF2BotMovement> m_tf2movement;
	std::unique_ptr<CTF2BotPlayerController> m_tf2controller;
	std::unique_ptr<CTF2BotSensor> m_tf2sensor;
	std::unique_ptr<CTF2BotBehavior> m_tf2behavior;
	std::unique_ptr<CTF2BotSpyMonitor> m_tf2spymonitor;
	std::unique_ptr<CTF2BotInventory> m_tf2inventory;
	std::unique_ptr<CTF2BotCombat> m_tf2combat;
	CountdownTimer m_classswitchtimer; // class switch cooldown so we don't spam things
	IntervalTimer m_voicecmdtimer;
	CHandle<CBaseEntity> m_mySentryGun;
	CHandle<CBaseEntity> m_myDispenser;
	CHandle<CBaseEntity> m_myTeleporterEntrance;
	CHandle<CBaseEntity> m_myTeleporterExit;
	CTF2BotUpgradeManager m_upgrademan;
	float* m_cloakMeter;
	bool m_doMvMUpgrade;

	void SelectNewClass();
};

class CTF2BotPathCost : public IGroundPathCost
{
public:
	CTF2BotPathCost(CTF2Bot* bot, RouteType routetype = DEFAULT_ROUTE);

	float operator()(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator, float length) const override;

	void SetRouteType(RouteType type) override { m_routetype = type; }
	RouteType GetRouteType() const override { return m_routetype; }
private:
	CTF2Bot* m_me;
	RouteType m_routetype;
};

/**
 * @brief NavBot implementation of TF2's CTraceFilterIgnoreFriendlyCombatItems.
 * 
 * https://github.com/ValveSoftware/source-sdk-2013/blob/68c8b82fdcb41b8ad5abde9fe1f0654254217b8e/src/game/shared/tf/tf_weaponbase.h#L170
 */
class CTF2TraceFilterIgnoreFriendlyCombatItems : public trace::CTraceFilterSimple
{
public:
	CTF2TraceFilterIgnoreFriendlyCombatItems(CBaseEntity* passEnt, int collisionGroup, const int ignoreTeam);

	bool ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask) override;

private:
	int m_ignoreTeam;
};

#endif // !NAVBOT_TEAM_FORTRESS_2_BOT_H_
