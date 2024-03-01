#ifndef EXT_BASE_BOT_H_
#define EXT_BASE_BOT_H_

#include <list>
#include <queue>
#include <string>

#include <extplayer.h>
#include <bot/interfaces/playercontrol.h>
#include <bot/interfaces/movement.h>
#include <bot/interfaces/sensor.h>
#include <bot/interfaces/event_listener.h>
#include <bot/interfaces/behavior.h>
#include <bot/interfaces/profile.h>
#include <bot/interfaces/weapon.h>
#include <util/UtilTrace.h>
#include <sdkports/sdk_timers.h>

// Interval between calls to Update()
constexpr auto BOT_UPDATE_INTERVAL = 0.07f;

// TO-DO: Add a convar to control update interval

class IBotController;
class IBotInterface;

class CBaseBot : public CBaseExtPlayer, public IEventListener
{
public:
	CBaseBot(edict_t* edict);
	~CBaseBot() override;

	// Event propagation
	std::vector<IEventListener*>* GetListenerVector() override;

	// Called by the manager for all players every server frame.
	// Overriden to call bot functions
	void PlayerThink() final;

	// true if this is a bot managed by this extension
	bool IsExtensionBot() const final { return true; }
	// Pointer to the extension bot class
	CBaseBot* MyBotPointer() final { return this; }

	// Reset the bot to it's initial state
	virtual void Reset();
	// Function called at intervals to run the AI 
	virtual void Update();
	// Function called every server frame to run the AI
	virtual void Frame();

	virtual float GetRangeTo(const Vector& pos) const;
	virtual float GetRangeTo(edict_t* edict) const;
	virtual float GetRangeToSqr(const Vector& pos) const;
	virtual float GetRangeToSqr(edict_t* edict) const;

	virtual bool IsRangeGreaterThan(const Vector& pos, const float range) const;
	virtual bool IsRangeGreaterThan(edict_t* edict, const float range) const;
	virtual bool IsRangeLessThan(const Vector& pos, const float range) const;
	virtual bool IsRangeLessThan(edict_t* edict, const float range) const;

	virtual bool IsAbleToBreak(edict_t* entity);

	IBotController* GetController() const { return m_controller; }

	void RegisterInterface(IBotInterface* iface);
	void BuildUserCommand(const int buttons);
	void RunUserCommand(CBotCmd* ucmd);
	inline CBotCmd* GetUserCommand() { return &m_cmd; }
	// Sets the view angles to be sent on the next User Command
	inline void SetViewAngles(QAngle& angle) { m_viewangles = angle; }
	virtual IPlayerController* GetControlInterface() const;
	virtual IMovement* GetMovementInterface() const;
	virtual ISensor* GetSensorInterface() const;
	virtual IBehavior* GetBehaviorInterface() const;

	inline const std::list<IBotInterface*>& GetRegisteredInterfaces() const { return m_interfaces; }

	// Called every the player_spawn event is fired for this bot
	virtual void Spawn();
	// Called on the first spawn call
	virtual void FirstSpawn();
	// Called when the player_death event is fired for this bot
	virtual void Killed() {}
	// Called to check if the bot can join the game
	virtual bool CanJoinGame() { return true; }
	// Called to check if the bot has joined a team and should be considerated as playing
	virtual bool HasJoinedGame()
	{
		// for most games, team 0 is unassigned and team 1 is spectator
		return GetCurrentTeamIndex() > 1;
	}

	// Do what is necessary to join the game
	virtual void TryJoinGame() {}
	// Don't update the bot when dead if this returns true
	virtual bool SleepWhenDead() const { return true; }
	
	/**
	 * @brief Makes the bot switch weapons via user command.
	 * Weapon switch will happen when the server receives the next user command.
	 * Set to 0 to cancel a pending switch.
	 * @param weapon_entity Entity index of the weapon the bot wants to switch to
	*/
	inline void SelectWeaponByUserCmd(int weapon_entity) { m_weaponselect = weapon_entity; }
	void SelectWeaponByClassname(const char* szclassname);
	virtual void SafeWeaponSelectByClassname(const char* szclassname);
	void SelectWeaponByCommand(const char* szclassname) const;

	inline const DifficultyProfile& GetDifficultyProfile() const { return m_profile; }
	inline void SetDifficultyProfile(const DifficultyProfile& profile)
	{
		m_profile = profile;

		for (auto iface : m_interfaces)
		{
			iface->OnDifficultyProfileChanged();
		}
	}

	/**
	 * @brief Inserts a console command in the bot queue, prevents the bot flooding the server with commands
	 * @param command command to send
	*/
	inline void DelayedFakeClientCommand(const char* command)
	{
		m_cmdqueue.emplace(command);
	}

	void FakeClientCommand(const char* command) const;
	bool IsDebugging(int bits) const;
	virtual const char* GetDebugIdentifier() const;
	void DebugPrintToConsole(const int bits,const int red,const int green,const int blue, const char* fmt, ...) const;
	void DebugDisplayText(const char* text);
	void DebugFrame();

	virtual bool WantsToShootAtEnemies()
	{
		if (m_holdfire_time.HasStarted() && !m_holdfire_time.IsElapsed())
		{
			return false;
		}
		
		return true;
	}

	// makes the bot hold their fire for the given time in seconds
	inline void DontAttackEnemies(const float time) { m_holdfire_time.Start(time); }
	bool IsLineOfFireClear(const Vector& to) const;

	void UpdateMyWeapons();
	inline void SetWeaponUpdateTime(int ticks) { m_weaponupdatetimer = ticks; }
	inline size_t GetMyWeaponsCount() const { return m_weapons.size(); }
	// gets a CBotWeapon pointer of the weapon the bot is currently using, NULL if no weapon
	const CBotWeapon* GetActiveBotWeapon() const;

	/**
	 * @brief Runs a function on every valid bot weapon
	 * @tparam T a class with operator() overload with 1 parameter: (const CBotWeapon& weapon)
	 * @param functor function to run on every valid weapon
	 */
	template <typename T>
	inline void ForEveryWeapon(T functor) const
	{
		for (const auto& weapon : m_weapons)
		{
			if (!weapon.IsValid())
				continue;

			functor(weapon);
		}
	}

	inline const Vector& GetHomePos() const { return m_homepos; }
	void SetHomePos(const Vector& home) { m_homepos = home; }

protected:
	bool m_isfirstspawn;

private:
	int m_nextupdatetime;
	int m_joingametime; // delay between joingame attempts
	IBotController* m_controller;
	std::list<IBotInterface*> m_interfaces;
	std::vector<IEventListener*> m_listeners; // Event listeners vector
	CBotCmd m_cmd; // User command to send
	QAngle m_viewangles; // The bot eye angles
	int m_weaponselect;
	mutable IPlayerController* m_basecontrol; // Base controller interface
	mutable IMovement* m_basemover; // Base movement interface
	mutable ISensor* m_basesensor; // Base vision and hearing interface
	mutable IBehavior* m_basebehavior; // Base AI Behavior interface
	DifficultyProfile m_profile;
	CountdownTimer m_cmdtimer; // Delay between commands
	std::queue<std::string> m_cmdqueue; // Queue of commands to send
	int m_debugtextoffset;
	CountdownTimer m_holdfire_time; // Timer for the bot to not attack enemies
	std::vector<CBotWeapon> m_weapons;
	int m_weaponupdatetimer;
	Vector m_homepos; // Position where the bot spawned

	void ExecuteQueuedCommands();
};

#endif // !EXT_BASE_BOT_H_
