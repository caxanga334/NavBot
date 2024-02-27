#include <extension.h>
#include <manager.h>
#include <extplayer.h>
#include <util/entprops.h>
#include <util/librandom.h>
#include <bot/interfaces/base_interface.h>
#include <bot/interfaces/knownentity.h>
#include <bot/interfaces/playerinput.h>
#include <bot/interfaces/tasks.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <mods/basemod.h>
#include <tier1/convar.h>
#include "basebot.h"

extern CGlobalVars* gpGlobals;

ConVar cvar_bot_difficulty("sm_navbot_skill_level", "0", FCVAR_NONE, "Skill level group to use when selecting bot difficulty.");

class CBaseBotTestTask : public AITask<CBaseBot>
{
public:
	virtual TaskResult<CBaseBot> OnTaskUpdate(CBaseBot* bot) override;
	virtual TaskResult<CBaseBot> OnTaskResume(CBaseBot* bot, AITask<CBaseBot>* pastTask) override;
	virtual TaskEventResponseResult<CBaseBot> OnTestEventPropagation(CBaseBot* bot) override;
	virtual QueryAnswerType ShouldFreeRoam(CBaseBot* me) override;
	virtual const char* GetName() const { return "CBaseBotTestTask"; }
};

class CBaseBotPathTestTask : public AITask<CBaseBot>
{
public:
	virtual TaskResult<CBaseBot> OnTaskStart(CBaseBot* bot, AITask<CBaseBot>* pastTask) override;
	virtual TaskResult<CBaseBot> OnTaskUpdate(CBaseBot* bot) override;
	virtual TaskEventResponseResult<CBaseBot> OnMoveToSuccess(CBaseBot* bot, CPath* path) override;
	virtual const char* GetName() const { return "CBaseBotPathTestTask"; }

private:
	CMeshNavigator m_nav;
	Vector m_goal;
};

class CBaseBotSwitchTestTask : public AITask<CBaseBot>
{
public:
	virtual TaskResult<CBaseBot> OnTaskStart(CBaseBot* bot, AITask<CBaseBot>* pastTask) override;
	virtual TaskResult<CBaseBot> OnTaskUpdate(CBaseBot* bot) override;
	virtual const char* GetName() const { return "CBaseBotSwitchTestTask"; }

};

TaskResult<CBaseBot> CBaseBotTestTask::OnTaskUpdate(CBaseBot* bot)
{
	// rootconsole->ConsolePrint("AI Task -- Update");
	return Continue();
}

TaskResult<CBaseBot> CBaseBotTestTask::OnTaskResume(CBaseBot* bot, AITask<CBaseBot>* pastTask)
{
	rootconsole->ConsolePrint("CBaseBotTestTask::OnTaskResume");
	return SwitchTo(new CBaseBotSwitchTestTask, "Testing Task Switch!");
}

TaskEventResponseResult<CBaseBot> CBaseBotTestTask::OnTestEventPropagation(CBaseBot* bot)
{
	rootconsole->ConsolePrint("AI Event -- OnTestEventPropagation");
	return TryPauseFor(new CBaseBotPathTestTask, PRIORITY_HIGH, "Event pause test!");
}

QueryAnswerType CBaseBotTestTask::ShouldFreeRoam(CBaseBot* me)
{
	rootconsole->ConsolePrint("AI Query -- ShouldFreeRoam");
	return ANSWER_YES;
}

TaskResult<CBaseBot> CBaseBotPathTestTask::OnTaskStart(CBaseBot* bot, AITask<CBaseBot>* pastTask)
{
	edict_t* host = gamehelpers->EdictOfIndex(1); // get listen server host
	CBaseExtPlayer player(host);
	ShortestPathCost cost;

	m_goal = player.GetAbsOrigin();
	m_nav.SetSkipAheadDistance(350.0f);

	bool result = m_nav.ComputePathToPosition(bot, m_goal, cost);

	if (result == false)
	{
		rootconsole->ConsolePrint("CBaseBotPathTestTask::OnTaskStart path failed!");
		return Done("Failed to find a path!");
	}

	return Continue();
}

TaskResult<CBaseBot> CBaseBotPathTestTask::OnTaskUpdate(CBaseBot* bot)
{
	if (m_nav.IsValid() == false)
	{
		return Done("My Path is invalid!");
	}

	if (m_nav.GetAge() > 1.0f)
	{
		ShortestPathCost cost;
		bool result = m_nav.ComputePathToPosition(bot, m_goal, cost);

		if (result == false)
		{
			return Done("No Path!");
		}
	}

	m_nav.Update(bot);
	return Continue();
}

TaskEventResponseResult<CBaseBot> CBaseBotPathTestTask::OnMoveToSuccess(CBaseBot* bot, CPath* path)
{
	return TryDone(PRIORITY_HIGH, "Returning to previous task!");
}

class CBaseBotBehavior : public IBehavior
{
public:
	CBaseBotBehavior(CBaseBot* bot);
	virtual ~CBaseBotBehavior();

	virtual void Reset();
	virtual void Update();

	virtual IDecisionQuery* GetDecisionQueryResponder() override { return m_manager; }
	virtual std::vector<IEventListener*>* GetListenerVector();

private:
	AITaskManager<CBaseBot>* m_manager;
	std::vector<IEventListener*> m_listeners;
};

TaskResult<CBaseBot> CBaseBotSwitchTestTask::OnTaskStart(CBaseBot* bot, AITask<CBaseBot>* pastTask)
{
	rootconsole->ConsolePrint("CBaseBotSwitchTestTask::OnTaskStart");
	
	if (pastTask != nullptr)
	{
		rootconsole->ConsolePrint("%p", pastTask);
	}

	return Continue();
}

TaskResult<CBaseBot> CBaseBotSwitchTestTask::OnTaskUpdate(CBaseBot* bot)
{
	rootconsole->ConsolePrint("Hello from CBaseBotSwitchTestTask::OnTaskUpdate!");
	return SwitchTo(new CBaseBotTestTask, "Returning back to main task!");
}

CBaseBotBehavior::CBaseBotBehavior(CBaseBot* bot) : IBehavior(bot)
{
	m_manager = new AITaskManager<CBaseBot>(new CBaseBotTestTask);
	m_listeners.reserve(2);
	m_listeners.push_back(m_manager);
}

CBaseBotBehavior::~CBaseBotBehavior()
{
	delete m_manager;
}

void CBaseBotBehavior::Reset()
{
	m_listeners.clear();

	delete m_manager;
	m_manager = new AITaskManager<CBaseBot>(new CBaseBotTestTask);

	m_listeners.push_back(m_manager);
}

void CBaseBotBehavior::Update()
{
	m_manager->Update(GetBot());
}

std::vector<IEventListener*>* CBaseBotBehavior::GetListenerVector()
{
	if (m_manager == nullptr)
	{
		return nullptr;
	}

	static std::vector<IEventListener*> listeners;
	listeners.clear();
	listeners.push_back(m_manager);
	return &listeners;
}


CBaseBot::CBaseBot(edict_t* edict) : CBaseExtPlayer(edict),
	m_cmd(),
	m_viewangles(0.0f, 0.0f, 0.0f)
{
	m_profile = extmanager->GetBotDifficultyProfileManager().GetProfileForSkillLevel(cvar_bot_difficulty.GetInt());
	m_isfirstspawn = false;
	m_nextupdatetime = 64;
	m_joingametime = 64;
	m_controller = botmanager->GetBotController(edict);
	m_listeners.reserve(8);
	m_basecontrol = nullptr;
	m_basemover = nullptr;
	m_basesensor = nullptr;
	m_basebehavior = nullptr;
	m_weaponselect = 0;
	m_cmdtimer.Invalidate();
	m_debugtextoffset = 0;
	m_weapons.reserve(MAX_WEAPONS);
}

CBaseBot::~CBaseBot()
{
	if (m_basecontrol)
	{
		delete m_basecontrol;
	}

	if (m_basemover)
	{
		delete m_basemover;
	}

	if (m_basesensor)
	{
		delete m_basesensor;
	}
	
	if (m_basebehavior)
	{
		delete m_basebehavior;
	}

	m_interfaces.clear();
	m_listeners.clear();
}

std::vector<IEventListener*>* CBaseBot::GetListenerVector()
{
	return &m_listeners;
}

void CBaseBot::PlayerThink()
{
	CBaseExtPlayer::PlayerThink(); // Call base class function first

	m_debugtextoffset = 0;

	if (IsDebugging(BOTDEBUG_TASKS))
	{
		DebugFrame();
	}

	// this needs to be before the sleep check since spectator players are dead
	if (--m_joingametime <= 0)
	{
		if (!HasJoinedGame() && CanJoinGame())
		{
			TryJoinGame();
			m_joingametime = TIME_TO_TICKS(librandom::generate_random_float(1.0f, 5.0f));
		}
		else if (HasJoinedGame())
		{
			m_joingametime = TIME_TO_TICKS(20.0f); // if the bot is in-game already, run this check every 20 seconds
		}
	}

	if (SleepWhenDead() && GetPlayerInfo()->IsDead())
	{
		BuildUserCommand(0); // Still send empty usercommands while dead
		return;
	}

	ExecuteQueuedCommands(); // Run queue commands
	Frame(); // Call bot frame

	int buttons = 0;
	auto control = GetControlInterface();

	if (--m_nextupdatetime <= 0)
	{
		m_nextupdatetime = TIME_TO_TICKS(BOT_UPDATE_INTERVAL);

		Update(); // Run period update

		// Process all buttons during updates
		control->ProcessButtons(buttons);
	}
	else // Not running a full update
	{
		// Keep last cmd buttons pressed + any new button pressed
		buttons = control->GetOldButtonsToSend();
	}

	// This needs to be the last call on a bot think cycle
	BuildUserCommand(buttons);
}

void CBaseBot::Reset()
{
	m_nextupdatetime = 1;

	for (auto iface : m_interfaces)
	{
		iface->Reset();
	}

	m_weapons.clear();
	m_weaponupdatetimer = 2; // delay next weapon update
}

void CBaseBot::Update()
{
	for (auto iface : m_interfaces)
	{
		iface->Update();
	}
}

void CBaseBot::Frame()
{
	if (--m_weaponupdatetimer <= 0)
	{
		m_weaponupdatetimer = TIME_TO_TICKS(60.0f); // by default, update weapons every minute
		UpdateMyWeapons();
	}

	for (auto iface : m_interfaces)
	{
		iface->Frame();
	}
}

float CBaseBot::GetRangeTo(const Vector& pos) const
{
	return (GetAbsOrigin() - pos).Length();
}

float CBaseBot::GetRangeTo(edict_t* edict) const
{
	return (GetAbsOrigin() - edict->GetCollideable()->GetCollisionOrigin()).Length();
}

float CBaseBot::GetRangeToSqr(const Vector& pos) const
{
	return (GetAbsOrigin() - pos).LengthSqr();
}

float CBaseBot::GetRangeToSqr(edict_t* edict) const
{
	return (GetAbsOrigin() - edict->GetCollideable()->GetCollisionOrigin()).LengthSqr();
}

bool CBaseBot::IsRangeGreaterThan(const Vector& pos, const float range) const
{
	Vector to = (pos - GetAbsOrigin());
	return to.Length() > range;
}

bool CBaseBot::IsRangeGreaterThan(edict_t* edict, const float range) const
{
	Vector to = (edict->GetCollideable()->GetCollisionOrigin() - GetAbsOrigin());
	return to.Length() > range;
}

bool CBaseBot::IsRangeLessThan(const Vector& pos, const float range) const
{
	Vector to = (pos - GetAbsOrigin());
	return to.Length() < range;
}

bool CBaseBot::IsRangeLessThan(edict_t* edict, const float range) const
{
	Vector to = (edict->GetCollideable()->GetCollisionOrigin() - GetAbsOrigin());
	return to.Length() < range;
}

/**
 * @brief Checks if the bot is able to break the given entity
 * @param entity Entity the bot needs to break
 * @return true if the bot can break this entity
*/
bool CBaseBot::IsAbleToBreak(edict_t* entity)
{
	int index = gamehelpers->IndexOfEdict(entity);

	if (index == -1)
	{
		return false;
	}

	int takedamage = 0;

	if (entprops->GetEntProp(index, Prop_Data, "m_takedamage", takedamage) == true)
	{
		switch (takedamage)
		{
		case DAMAGE_NO:
		case DAMAGE_EVENTS_ONLY:
		{
			return false; // entity doesn't take damage
		}
		default:
			break;
		}
	}

	int health = 0;
	constexpr auto MAX_HEALTH_TO_BREAK = 1000; // if the entity health is greater than this, don't bother trying to break it
	
	if (entprops->GetEntProp(index, Prop_Data, "m_iHealth", health) == true)
	{
		if (health > MAX_HEALTH_TO_BREAK)
		{
			return false; // don't bother
		}
	}

	auto classname = gamehelpers->GetEntityClassname(entity);

	if (strncmp(classname, "func_breakable", 14) == 0)
	{
		return true;
	}
	else if (strncmp(classname, "func_breakable_surf", 19) == 0)
	{
		return true;
	}

	return false;
}

void CBaseBot::RegisterInterface(IBotInterface* iface)
{
	m_interfaces.push_back(iface);
	m_listeners.push_back(iface);
}

void CBaseBot::BuildUserCommand(const int buttons)
{
	int commandnumber = m_cmd.command_number;
	auto mover = GetMovementInterface();
	auto control = GetControlInterface();

	m_cmd.Reset();

	commandnumber++;
	m_cmd.command_number = commandnumber;
	m_cmd.tick_count = gpGlobals->tickcount;
	m_cmd.impulse = 0;
	m_cmd.viewangles = m_viewangles; // set view angles
	m_cmd.weaponselect = m_weaponselect; // send weapon select
	// TO-DO: weaponsubtype
	m_cmd.buttons = buttons; // send buttons

	float forwardspeed = 0.0f;
	float sidespeed = 0.0f;

	if (buttons & INPUT_FORWARD)
	{
		forwardspeed = mover->GetMovementSpeed();
	}
	else if (buttons & INPUT_BACK)
	{
		forwardspeed = -mover->GetMovementSpeed();
	}

	if (buttons & INPUT_MOVERIGHT)
	{
		sidespeed = mover->GetMovementSpeed();
	}
	else if (buttons & INPUT_MOVELEFT)
	{
		sidespeed = -mover->GetMovementSpeed();
	}

	// TO-DO: Up speed

	if (control->ShouldApplyScale())
	{
		forwardspeed = forwardspeed * control->GetForwardScale();
		sidespeed = sidespeed * control->GetSideScale();
	}

	m_cmd.forwardmove = forwardspeed;
	m_cmd.sidemove = sidespeed;
	m_cmd.upmove = 0.0f;

	RunUserCommand(&m_cmd);

	m_weaponselect = 0;
}

void CBaseBot::RunUserCommand(CBotCmd* ucmd)
{
	if (!extension->ShouldCallRunPlayerCommand()) // this mod already calls runplayermove on bots, we send the bot actual cmd on the hook
	{
		return;
	}
	else
	{
		m_controller->RunPlayerMove(ucmd);
	}
}

IPlayerController* CBaseBot::GetControlInterface()
{
	if (m_basecontrol == nullptr)
	{
		m_basecontrol = new IPlayerController(this);
	}

	return m_basecontrol;
}

IMovement* CBaseBot::GetMovementInterface()
{
	if (m_basemover == nullptr)
	{
		m_basemover = new IMovement(this);
	}

	return m_basemover;
}

ISensor* CBaseBot::GetSensorInterface()
{
	if (m_basesensor == nullptr)
	{
		m_basesensor = new ISensor(this);
	}

	return m_basesensor;
}

IBehavior* CBaseBot::GetBehaviorInterface()
{
	if (m_basebehavior == nullptr)
	{
		m_basebehavior = new CBaseBotBehavior(this);
	}

	return m_basebehavior;
}

void CBaseBot::Spawn()
{
	if (m_isfirstspawn == false)
	{
		m_isfirstspawn = true;
		FirstSpawn();
	}

#ifdef EXT_DEBUG

	auto speed = GetMaxSpeed();

	if (speed <= 0.1f)
	{
		// Warn developers if this is 0
		smutils->LogError(myself, "CBasePlayer::m_flMaxspeed == %4.8f", speed);
	}

#endif // EXT_DEBUG

	Reset();
}

void CBaseBot::FirstSpawn()
{
}

/**
 * @brief Selects weapon by using the IBotController::SetActiveWeapon function.
 * This will call BCC's Weapon_Create, make sure the bot actually owns the weapon you want or the bot will magically get one.
 * @param szclassname Weapon classname to select
*/
void CBaseBot::SelectWeaponByClassname(const char* szclassname)
{
	m_controller->SetActiveWeapon(szclassname);
}

/**
 * @brief Safe version of 'CBaseBot::SelectWeaponByClassname'. Adds an addition check to see if the bot actually owns the weapon
 * @param szclassname Weapon classname to select
*/
void CBaseBot::SafeWeaponSelectByClassname(const char* szclassname)
{
	if (Weapon_OwnsThisType(szclassname))
	{
		m_controller->SetActiveWeapon(szclassname);
	}
}

/**
 * @brief Makes the bot select a weapon via console command 'use'.
 * @param szclassname Classname of the weapon to select
*/
void CBaseBot::SelectWeaponByCommand(const char* szclassname) const
{
	char command[128];
	ke::SafeSprintf(command, sizeof(command), "use %s", szclassname);
	serverpluginhelpers->ClientCommand(GetEdict(), command);
}

void CBaseBot::FakeClientCommand(const char* command) const
{
	serverpluginhelpers->ClientCommand(GetEdict(), command);
}

void CBaseBot::ExecuteQueuedCommands()
{
	if (m_cmdqueue.size() == 0)
	{
		return;
	}

	if (!m_cmdtimer.HasStarted() || m_cmdtimer.IsElapsed())
	{
		constexpr auto NEXT_COMMAND_DELAY = 0.25f;
		
		auto& next = m_cmdqueue.front();
		serverpluginhelpers->ClientCommand(GetEdict(), next.c_str());
		m_cmdqueue.pop();

		if (m_cmdqueue.size() == 0)
		{
			m_cmdtimer.Invalidate();
		}
	}
}

bool CBaseBot::IsLineOfFireClear(const Vector& to) const
{
	CTraceFilterNoNPCsOrPlayer filter(GetEdict()->GetIServerEntity(), COLLISION_GROUP_NONE);
	trace_t result;
	UTIL_TraceLine(GetEyeOrigin(), to, MASK_SHOT, &filter, &result);
	return !result.DidHit();
}

void CBaseBot::UpdateMyWeapons()
{
	m_weapons.clear();

	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		int index = INVALID_EHANDLE_INDEX;
		entprops->GetEntPropEnt(GetIndex(), Prop_Send, "m_hMyWeapons", index, i);

		if (index == INVALID_EHANDLE_INDEX)
			continue;

		edict_t* weapon = gamehelpers->EdictOfIndex(index);

		if (!weapon || weapon->IsFree())
			continue;

		m_weapons.emplace_back(weapon);
	}
}

const CBotWeapon* CBaseBot::GetActiveBotWeapon() const
{
	auto weapon = GetActiveWeapon();
	
	if (!weapon || weapon->IsFree())
		return nullptr;

	int index = gamehelpers->IndexOfEdict(weapon);

	for (const auto& botweapon : m_weapons)
	{
		if (botweapon.GetBaseCombatWeapon().GetIndex() == index)
		{
			return &botweapon;
		}
	}

	return nullptr;
}
