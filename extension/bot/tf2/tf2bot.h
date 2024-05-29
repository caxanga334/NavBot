#ifndef NAVBOT_TEAM_FORTRESS_2_BOT_H_
#define NAVBOT_TEAM_FORTRESS_2_BOT_H_
#pragma once

#include <memory>

#include <sdkports/sdk_timers.h>
#include <mods/tf2/teamfortress2_shareddefs.h>
#include <bot/basebot.h>
#include <bot/interfaces/path/basepath.h>
#include <basehandle.h>

#include "tf2bot_behavior.h"
#include "tf2bot_controller.h"
#include "tf2bot_movement.h"
#include "tf2bot_sensor.h"
#include "tf2bot_upgrades.h"

struct edict_t;

class CTF2Bot : public CBaseBot
{
public:
	CTF2Bot(edict_t* edict);
	~CTF2Bot() override;

	enum KnownSpyInfo
	{
		KNOWNSPY_NOT_SUSPICIOUS = 0, // Enemy spy has fooled me
		KNOWNSPY_SUSPICIOUS, // I don't trust the enemy spy
		KNOWNSPY_FOUND // I Know the enemy spy is an enemy spy
	};

	class KnownSpy
	{
	public:
		KnownSpy(edict_t* spy, KnownSpyInfo info = KnownSpyInfo::KNOWNSPY_NOT_SUSPICIOUS);

		inline bool operator==(const KnownSpy& other)
		{
			return this->m_handle == other.m_handle;
		}

		void Update(KnownSpyInfo newinfo, const bool updateinfo = true, const bool updateclass = true);
		bool IsValid() const;
		inline float GetTimeSinceLastInfoUpdate() const { return m_time.GetElapsedTime(); }
		edict_t* GetEdict() const;
		int GetIndex() const { return m_handle.GetEntryIndex(); }
		inline bool ShouldForget() const { return m_time.IsGreaterThen(time_to_forget()); }
		inline bool ShouldLoseAggression() const { return m_time.IsGreaterThen(time_to_lose_aggro()); }
		inline bool ShouldGoCalm() const { return m_time.IsGreaterThen(time_to_stay_calm()); }
		inline KnownSpyInfo GetInfo() const { return m_info; }
		bool IsSuspicious(CTF2Bot* me) const;

	private:
		CBaseHandle m_handle;
		KnownSpyInfo m_info;
		IntervalTimer m_time;
		TeamFortress2::TFClassType m_lastclass;
		// memorize info for 30 seconds
		static constexpr float time_to_forget() { return 30.0f; }
		static constexpr float time_to_lose_aggro() { return 5.0f; }
		static constexpr float time_to_stay_calm() { return 20.0f; }
		// if a suspicious spy gets this close to the bot, detect it
		static constexpr float sus_too_close_range() { return 300.0f; }
		// distance to consider a spy 'touching' the bot, used in detection
		static constexpr float touch_range() { return 75.0f; }
	};

	// Reset the bot to it's initial state
	void Reset() override;
	// Function called at intervals to run the AI 
	// virtual void Update() override;
	// Function called every server frame to run the AI
	void Frame() override;

	void TryJoinGame() override;
	void Spawn() override;
	void FirstSpawn() override;

	CTF2BotPlayerController* GetControlInterface() const override { return m_tf2controller.get(); }
	CTF2BotMovement* GetMovementInterface() const override { return m_tf2movement.get(); }
	CTF2BotSensor* GetSensorInterface() const override { return m_tf2sensor.get(); }
	CTF2BotBehavior* GetBehaviorInterface() const override { return m_tf2behavior.get(); }
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
	/**
	 * @brief Toggles the ready status in Tournament modes. Also used in Mann vs Machine.
	 * @param isready Is the bot ready?
	 */
	void ToggleTournamentReadyStatus(bool isready = true) const;
	bool TournamentIsReady() const;

	/**
	 * @brief Gets a known spy information about a spy or NULL if none
	 * @param spy Spy to search for
	 * @return KnownSpy pointer or NULL if not known
	 */
	const KnownSpy* GetKnownSpy(edict_t* spy) const;
	const KnownSpy* UpdateOrCreateKnownSpy(edict_t* spy, KnownSpyInfo info, const bool updateinfo = false, const bool updateclass = false);

	CTF2BotUpgradeManager& GetUpgradeManager() { return m_upgrademan; }

private:
	std::unique_ptr<CTF2BotMovement> m_tf2movement;
	std::unique_ptr<CTF2BotPlayerController> m_tf2controller;
	std::unique_ptr<CTF2BotSensor> m_tf2sensor;
	std::unique_ptr<CTF2BotBehavior> m_tf2behavior;
	TeamFortress2::TFClassType m_desiredclass; // class the bot wants
	IntervalTimer m_classswitchtimer;
	CBaseHandle m_mySentryGun;
	CBaseHandle m_myDispenser;
	CBaseHandle m_myTeleporterEntrance;
	CBaseHandle m_myTeleporterExit;
	std::vector<KnownSpy> m_knownspies;
	int m_knownspythink;
	CTF2BotUpgradeManager m_upgrademan;

	inline void UpdateKnownSpies()
	{
		m_knownspies.erase(std::remove_if(m_knownspies.begin(), m_knownspies.end(), [](const KnownSpy& knownspy) {
			if (!knownspy.IsValid())
			{
				return true; // remove information of spies that no longer exists
			}

			return knownspy.ShouldForget();
		}), m_knownspies.end());

		for (auto& knownspy : m_knownspies)
		{
			switch (knownspy.GetInfo())
			{
			case CTF2Bot::KNOWNSPY_FOUND:
			{
				if (knownspy.ShouldLoseAggression())
				{
					// after some time without info on the spy, go back to suspicious
					knownspy.Update(KNOWNSPY_SUSPICIOUS, true, false);
				}

				break;
			}
			case CTF2Bot::KNOWNSPY_SUSPICIOUS:
			{
				if (knownspy.ShouldGoCalm())
				{
					// after a longer time without info on the spy, go back to not suspicious
					knownspy.Update(KNOWNSPY_NOT_SUSPICIOUS, true, false);
				}
				break;
			}
			default:
				break;
			}
		}
	}

	static constexpr float medic_patient_health_critical_level() { return 0.3f; }
	static constexpr float medic_patient_health_low_level() { return 0.6f; }
	static constexpr float knownspy_think_interval() { return 0.5f; }
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
