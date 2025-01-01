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

class CBaseBot : public CBaseExtPlayer, public IEventListener
{
public:
	CBaseBot(edict_t* edict);
	~CBaseBot() override;

	static bool InitHooks(SourceMod::IGameConfig* gd_navbot, SourceMod::IGameConfig* gd_sdkhooks, SourceMod::IGameConfig* gd_sdktools);

	static int m_maxStringCommandsPerSecond;

	// Called when the bot is added to the game
	void PostAdd();

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

public:

	// Event propagation
	std::vector<IEventListener*>* GetListenerVector() override;

	// Called by the manager for all players every server frame.
	// Overriden to call bot functions
	void PlayerThink() final;

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

protected:
	bool m_isfirstspawn;

	// Adds a SourceHook Hook into the hook list to be removed when this is destroyed
	void AddSourceHookID(int hookID) { m_shhooks.push_back(hookID); }

private:
	int m_simulationtick;
	CountdownTimer m_nextupdatetime;
	int m_joingametime; // delay between joingame attempts
	IBotController* m_controller;
	std::list<IBotInterface*> m_interfaces;
	std::vector<IEventListener*> m_listeners; // Event listeners vector
	CBotCmd m_cmd; // User command to send
	QAngle m_viewangles; // The bot eye angles
	int m_weaponselect;
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
	CMeshNavigator* m_activeNavigator;
	CountdownTimer m_randomChatMessageTimer;

	void ExecuteQueuedCommands();
};

#endif // !EXT_BASE_BOT_H_
