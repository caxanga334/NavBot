#ifndef EXT_BASE_BOT_H_
#define EXT_BASE_BOT_H_

#include <list>
#include <queue>
#include <string>
#include <memory>

#include <IGameConfigs.h>
#include <extplayer.h>
#include <util/librandom.h>
#include <bot/interfaces/playercontrol.h>
#include <bot/interfaces/movement.h>
#include <bot/interfaces/sensor.h>
#include <bot/interfaces/event_listener.h>
#include <bot/interfaces/behavior.h>
#include <bot/interfaces/profile.h>
#include <bot/interfaces/weapon.h>
#include <bot/interfaces/inventory.h>
#include <bot/interfaces/squads.h>
#include <sdkports/sdk_timers.h>

// Interval between calls to Update()
constexpr auto BOT_UPDATE_INTERVAL = 0.07f;

class IBotController;
class IBotInterface;
class CUserCmd;
class IMoveHelper;
class CMeshNavigator;
class CNavPrerequisite;

class CBaseBot : public CBaseExtPlayer, public IEventListener
{
public:
	CBaseBot(edict_t* edict);
	~CBaseBot() override;

	static bool InitHooks(SourceMod::IGameConfig* gd_navbot, SourceMod::IGameConfig* gd_sdkhooks, SourceMod::IGameConfig* gd_sdktools);
	inline static int m_maxStringCommandsPerSecond{ 20 };

	// Called when the bot is added to the game
	void PostAdd();
	/**
	 * @brief Called post constructor while still inside the OnClientPutInServer callback.
	 */
	virtual void PostConstruct() {}

	// Returns the time the bot has spawned (Spawn() function called)
	inline float GetLastSpawnTime() const { return m_spawnTime; }
	// How many seconds have passed since the bot last spawned.
	float GetTimeSinceLastSpawn() const;

private:
	void AddHooks();

	void Hook_Spawn();
	void Hook_Spawn_Post();
	void Hook_Touch(CBaseEntity* pOther);
	int Hook_OnTakeDamage_Alive(const CTakeDamageInfo& info);
	void Hook_Event_Killed(const CTakeDamageInfo& info);
	void Hook_Event_KilledOther(CBaseEntity* pVictim, const CTakeDamageInfo& info);
	void Hook_PhysicsSimulate();
	void Hook_PlayerRunCommand(CUserCmd* usercmd, IMoveHelper* movehelper);
	void Hook_Weapon_Equip_Post(CBaseEntity* weapon);

protected:
	/**
	 * @brief Adds a SourceHook's hook ID to the list of hooks for this bot.
	 * 
	 * Hooks are automatically removed on the destructor.
	 * @param id Hook ID.
	 */
	inline void AddHookID(int id) { m_shhooks.push_back(id); }

public:

	// Event propagation
	std::vector<IEventListener*>* GetListenerVector() override;

	// Called by the manager for all players every server frame.
	// Overriden to call bot functions
	void PlayerThink() final;
	void NavAreaChanged(CNavArea* old, CNavArea* current) final;
	// true if this is a bot managed by this extension
	bool IsExtensionBot() const final { return true; }
	// Was this bot created by a sourcemod plugin?
	virtual bool IsPluginBot() { return false; }
	// Will PlayerRunCommand be called by the extesion? (Allow plugin bots to manually run them). Always true for base.
	virtual bool RunPlayerCommands() { return true; }
	// Pointer to the extension bot class
	CBaseBot* MyBotPointer() final { return this; }

	void RefreshDifficulty(const CDifficultyManager* manager);

	// Reset the bot to it's initial state
	virtual void Reset();
	// Function called at intervals to run the AI 
	virtual void Update();
	// Function called every server frame to run the AI
	virtual void Frame();
	/**
	 * @brief Called at every think while dead and sleeping.
	 * @param buttons Buttons to send.
	 */
	virtual void DeadSleepThink(int& buttons) {}

	virtual float GetRangeTo(const Vector& pos) const;
	virtual float GetRangeTo(edict_t* edict) const;
	virtual float GetRangeTo(CBaseEntity* entity) const;
	virtual float GetRangeToSqr(const Vector& pos) const;
	virtual float GetRangeToSqr(edict_t* edict) const;
	virtual float GetRangeToSqr(CBaseEntity* entity) const;

	virtual bool IsRangeGreaterThan(const Vector& pos, const float range) const;
	virtual bool IsRangeGreaterThan(edict_t* edict, const float range) const;
	virtual bool IsRangeGreaterThan(CBaseEntity* entity, const float range) const;
	virtual bool IsRangeLessThan(const Vector& pos, const float range) const;
	virtual bool IsRangeLessThan(edict_t* edict, const float range) const;
	virtual bool IsRangeLessThan(CBaseEntity* entity, const float range) const;

	virtual bool IsAbleToBreak(edict_t* entity);
	virtual bool IsAlive() const;

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
	virtual IInventory* GetInventoryInterface() const;
	virtual ISquad* GetSquadInterface() const;

	inline const std::list<IBotInterface*>& GetRegisteredInterfaces() const { return m_interfaces; }

	// Called every the player_spawn event is fired for this bot
	virtual void Spawn();
	// Called on the first spawn call
	virtual void FirstSpawn();
	// Called when this bot touches another entity
	virtual void Touch(CBaseEntity* pOther) {}
	// Called when this bot dies
	virtual void Killed();
	// Called by the OnTakeDamage_Alive hook.
	virtual void OnTakeDamage_Alive(const CTakeDamageInfo& info);
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

	inline const DifficultyProfile* GetDifficultyProfile() const { return m_profile.get(); }
	inline void SetDifficultyProfile(std::shared_ptr<DifficultyProfile> profile)
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
	// Gets the bot client (player) name.
	const char* GetClientName() const;
	void DebugPrintToConsole(const int bits, const int red, const int green, const int blue, const char* fmt, ...) const;
	void DebugPrintToConsole(const int red, const int green, const int blue, const char* fmt, ...) const;
	void DebugDisplayText(const char* text);
	void DebugFrame();

	virtual bool IsLineOfFireClear(const Vector& to) const;

	inline const Vector& GetHomePos() const { return m_homepos; }
	void SetHomePos(const Vector& home) { m_homepos = home; }

	void SetActiveNavigator(CMeshNavigator* nav) { m_activeNavigator = nav; }
	// Gets the last nav mesh navigator used by the bot. NULL if not pathing
	CMeshNavigator* GetActiveNavigator() const { return m_activeNavigator; }
	void NotifyNavigatorDestruction(CMeshNavigator* nav, const bool force = false)
	{
		if (force || m_activeNavigator == nav)
		{
			m_activeNavigator = nullptr;
		}
	}

	int GetWaterLevel() const;
	bool IsUnderWater() const;

	void SendChatMessage(const char* message);
	void SendTeamChatMessage(const char* message);
	void SetImpulseCommand(int impulse)
	{
		if (impulse > 0 && impulse < 256)
		{
			m_impulse = impulse;
		}
	}

	/**
	 * @brief Fires the currently held weapon at the given enemy.
	 * @param enemy The enemy the bot will fire at.
	 * @param doAim If set to true, this function will also make the bot aim at the current enemy.
	 */
	virtual void FireWeaponAtEnemy(const CKnownEntity* enemy, const bool doAim = true);
	/**
	 * @brief If the bot was unable to fire their weapon, this function will be called to handle a possible need for reloads.
	 * @param weapon Weapon that the bot was unable to fire.
	 */
	virtual void ReloadIfNeeded(CBotWeapon* weapon);

	const CNavPrerequisite* GetLastUsedPrerequisite() const { return m_lastPrerequisite; }
	void SetLastUsedPrerequisite(const CNavPrerequisite* prereq) { m_lastPrerequisite = prereq; m_clearLastPrerequisiteTimer.Start(60.0f); }

	static inline librandom::RandomNumberGenerator<std::mt19937, unsigned int> s_usercmdrng{}; // random number generator bot user commands
	static inline librandom::RandomNumberGenerator<std::mt19937, unsigned int> s_botrng{}; // random number generator for bot stuff
	// Is the bot using a weapon with scopes and is currently scoped in
	virtual bool IsScopedIn() const { return false; }

	/**
	 * @brief Checks if the bot took some damage recently.
	 * @param time How long since the bot took damage
	 * @return true if the bot has taken some damage within the last 'time' seconds.
	 */
	inline bool HasTakenDamageRecently(const float time = 1.0f)
	{
		return m_lasthurttimer.IsLessThen(time);
	}

	const IntervalTimer& GetLastKilledTimer() const { return m_lastkilledtimer; }
protected:
	bool m_isfirstspawn;

	// Adds a SourceHook Hook into the hook list to be removed when this is destroyed
	void AddSourceHookID(int hookID) { m_shhooks.push_back(hookID); }

	/**
	 * @brief Determines if the weapon can be fired for the given range and determines which attack type is possible to use.
	 * @param weapon Weapon that the bot will fire.
	 * @param range Range to the target.
	 * @param allowSecondary Allow secondary attacks?
	 * @param doPrimary Set to true to perform a primary attack or false for secondary.
	 * @return true if the bot can fire their weapon. false otherwise.
	 */
	virtual bool CanFireWeapon(CBotWeapon* weapon, const float range, const bool allowSecondary, bool& doPrimary);
	/**
	 * @brief Called to handle the given weapon.
	 * @param weapon Weapon that the bot wants to fire.
	 * @return true if the weapon can be fired, false if not.
	 */
	virtual bool HandleWeapon(CBotWeapon* weapon);
	/**
	 * @brief This function has two purposes:
	 * 
	 * 1. Check if the line of fire is clear to the enemy. Return false if the line of fire is blocked.
	 * 
	 * 2. If 'doAim' is true, instruct the player controller interface to aim at the current enemy. Determine the best spot to aim at.
	 * @param enemy Current enemy.
	 * @param weapon Weapon that the bot will be aiming with.
	 * @param doAim If true, send AimAt commands to the control interface.
	 * @param range Distance between the bot and the enemy.
	 * @param isPrimary True if using primary attack, false if secondary attack.
	 * @return True if the line of fire is clear. False if not.
	 */
	virtual bool AimWeaponAtEnemy(const CKnownEntity* enemy, CBotWeapon* weapon, const bool doAim, const float range, const bool isPrimary);

	inline void SetJustFiredMyWeapon(bool v = true) { m_justfiredmyweapon = v; }
	bool HasJustFiredMyWeapon() const { return m_justfiredmyweapon; }

private:
	bool m_justfiredmyweapon;
	float m_spawnTime;
	int m_simulationtick;
	CountdownTimer m_nextupdatetime;
	int m_joingametime; // delay between joingame attempts
	IBotController* m_controller;
	std::list<IBotInterface*> m_interfaces;
	std::vector<IEventListener*> m_listeners; // Event listeners vector
	CBotCmd m_cmd; // User command to send
	QAngle m_viewangles; // The bot eye angles
	int m_weaponselect;
	int m_impulse; // impulse to send on the next PlayerRunCommand
	mutable std::unique_ptr<IPlayerController> m_basecontrol; // Base controller interface
	mutable std::unique_ptr<IMovement> m_basemover; // Base movement interface
	mutable std::unique_ptr<ISensor> m_basesensor; // Base vision and hearing interface
	mutable std::unique_ptr<IBehavior> m_basebehavior; // Base AI Behavior interface
	mutable std::unique_ptr<IInventory> m_baseinventory; // Base inventory interface
	mutable std::unique_ptr<ISquad> m_basesquad; // Base Squad interface
	std::shared_ptr<DifficultyProfile> m_profile;
	IntervalTimer m_cmdtimer; // Timer to prevent sending more than the string commands per second limit
	std::queue<std::string> m_cmdqueue; // Queue of commands to send
	int m_cmdsents; // How many string commands this bot has sent within 1 second
	int m_debugtextoffset;
	Vector m_homepos; // Position where the bot spawned
	std::vector<int> m_shhooks; // IDs of SourceHook's hooks
	IntervalTimer m_burningtimer;
	IntervalTimer m_lasthurttimer; // timer for tracking the last time the bot took damage while alive
	IntervalTimer m_lastkilledtimer; // timer for tracing the last time the bot was killed
	CMeshNavigator* m_activeNavigator;
	CountdownTimer m_reloadCheckDelay;
	const CNavPrerequisite* m_lastPrerequisite; // Last prerequisite this bot used
	CountdownTimer m_clearLastPrerequisiteTimer;

	void ExecuteQueuedCommands();
};

extern ConVar cvar_bot_disable_behavior;

#endif // !EXT_BASE_BOT_H_
