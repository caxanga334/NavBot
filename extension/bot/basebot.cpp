#include NAVBOT_PCH_FILE
#include <extension.h>
#include <manager.h>
#include <extplayer.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/sdkcalls.h>
#include <mods/basemod.h>
#include <tier1/convar.h>
#include <sdkports/sdk_takedamageinfo.h>
#include <sdkports/sdk_traces.h>
#include <am-float.h>
#include "basebot_behavior.h"
#include "basebot.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

ConVar cvar_bot_difficulty("sm_navbot_skill_level", "0", FCVAR_NONE, "Skill level group to use when selecting bot difficulty. Setting this to negative to generate a random profile for each bot.");
ConVar cvar_bot_disable_behavior("sm_navbot_debug_disable_gamemode_ai", "0", FCVAR_CHEAT | FCVAR_GAMEDLL, "When set to 1, disables game mode behavior for the bot AI.");

CBaseBot::CBaseBot(edict_t* edict) : CBaseExtPlayer(edict),
	m_cmd(),
	m_viewangles(0.0f, 0.0f, 0.0f), m_homepos(0.0f, 0.0f, 0.0f)
{
	m_lastUpdateTime = 0.0f;
	m_lastUpdateDelta = 0.0f;
	m_spawnTime = -1.0f;
	m_simulationtick = -1;
	m_profile = extmanager->GetMod()->GetBotDifficultyManager()->GetProfileForSkillLevel(cvar_bot_difficulty.GetInt());
	m_isfirstspawn = false;
	m_nextupdatetime.Invalidate();
	m_joingametime = 64;
	m_controller = nullptr; // bot manager will return NULL if called here because the FL_FAKECLIENT flag isn't set yet
	m_listeners.reserve(8);
	m_basecontrol = nullptr;
	m_basemover = nullptr;
	m_basesensor = nullptr;
	m_basebehavior = nullptr;
	m_baseinventory = nullptr;
	m_basecombat = nullptr;
	m_weaponselect = 0;
	m_cmdtimer.Invalidate();
	m_cmdsents = 0;
	m_debugtextoffset = 0;
	m_shhooks.reserve(32);
	m_impulse = 0;
	m_lastPrerequisite = nullptr;
	m_activeNavigator = nullptr;

	AddHooks();
}

CBaseBot::~CBaseBot()
{
	m_interfaces.clear();
	m_listeners.clear();

	for (auto& hook : m_shhooks)
	{
		SH_REMOVE_HOOK_ID(hook);
	}

	m_shhooks.clear();
}

void CBaseBot::PostAdd()
{
	if (CExtManager::ShouldFixUpBotFlags())
	{
		entprops->SetEntProp(GetIndex(), Prop_Send, "m_fFlags", 0); // clear flags
		entprops->SetEntProp(GetIndex(), Prop_Send, "m_fFlags", FL_CLIENT | FL_FAKECLIENT); // set client and fakeclient flags
	}

	if (m_controller == nullptr)
	{
		m_controller = botmanager->GetBotController(GetEdict());
	}

#ifdef EXT_DEBUG
	META_CONPRINTF("CBaseBot::PostAdd m_controller = %p \n", m_controller);

	int flags = 0;
	entprops->GetEntProp(GetIndex(), Prop_Send, "m_fFlags", flags);

	if ((flags & FL_FAKECLIENT) == 0)
	{
		smutils->LogError(myself, "CBaseBot::PostAdd FL_FAKECLIENT not set for bot %s!", GetClientName());
	}
#endif // EXT_DEBUG
}

float CBaseBot::GetTimeSinceLastSpawn() const
{
	return gpGlobals->curtime - m_spawnTime;
}

std::vector<IEventListener*>* CBaseBot::GetListenerVector()
{
	return &m_listeners;
}

void CBaseBot::PlayerThink()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CBaseBot::PlayerThink", "NavBot");
#endif // EXT_VPROF_ENABLED

	CBaseExtPlayer::PlayerThink(); // Call base class function first

	m_debugtextoffset = 0;

	if (IsDebugging(BOTDEBUG_TASKS))
	{
		DebugFrame();
	}

	// this needs to be before the sleep check since spectator players are dead
	// bots created by plugins will have to be handled by the 
	if (--m_joingametime <= 0)
	{
		if (!HasJoinedGame() && CanJoinGame())
		{
			TryJoinGame();
			m_joingametime = TIME_TO_TICKS(1.0f);
		}
		else if (HasJoinedGame())
		{
			m_joingametime = TIME_TO_TICKS(20.0f); // if the bot is in-game already, run this check every 20 seconds
		}
	}

	if (SleepWhenDead() && GetPlayerInfo()->IsDead())
	{
		int deadbuttons = 0;
		DeadSleepThink(deadbuttons);
		BuildUserCommand(deadbuttons); // Still send empty usercommands while dead
		ExecuteQueuedCommands();
		return;
	}

	// Clear the move weight from the last frame
	GetMovementInterface()->ClearMoveWeight();

	Frame(); // Call bot frame

	int buttons = 0;
	auto control = GetControlInterface();

	if (m_nextupdatetime.IsElapsed())
	{
		m_nextupdatetime.Start(extmanager->GetMod()->GetModSettings()->GetUpdateRate());

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
	control->OnPostRunCommand();
	// Execute the command at the end of the tick since the BOT AI might queue commands on this tick
	ExecuteQueuedCommands(); // Run queue commands
}

void CBaseBot::NavAreaChanged(CNavArea* old, CNavArea* current)
{
	/* 
	 * Called by the base class when updating the last known nav area 
	 * Propagate the event to the behavior
	*/

	this->OnNavAreaChanged(old, current);
}

void CBaseBot::RefreshDifficulty(const CDifficultyManager* manager)
{
	m_profile = manager->GetProfileForSkillLevel(cvar_bot_difficulty.GetInt());

	for (auto iface : m_interfaces)
	{
		iface->OnDifficultyProfileChanged();
	}
}

void CBaseBot::Reset()
{
	m_nextupdatetime.Invalidate();
	m_lastPrerequisite = nullptr;
	m_clearLastPrerequisiteTimer.Invalidate();

	for (auto iface : m_interfaces)
	{
		iface->Reset();
	}
}

void CBaseBot::Update()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CBaseBot::Update", "NavBot");
#endif // EXT_VPROF_ENABLED

	const float curtime = gpGlobals->curtime;

	m_lastUpdateDelta = curtime - m_lastUpdateTime;

	if (m_clearLastPrerequisiteTimer.HasStarted() && m_clearLastPrerequisiteTimer.IsElapsed())
	{
		m_clearLastPrerequisiteTimer.Invalidate();
		m_lastPrerequisite = nullptr;
	}

	for (auto iface : m_interfaces)
	{
		iface->Update();
	}

	m_lastUpdateTime = curtime;
}

void CBaseBot::Frame()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CBaseBot::Frame", "NavBot");
#endif // EXT_VPROF_ENABLED

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

float CBaseBot::GetRangeTo(CBaseEntity* entity) const
{
	return (GetAbsOrigin() - reinterpret_cast<IServerEntity*>(entity)->GetCollideable()->GetCollisionOrigin()).Length();
}

float CBaseBot::GetRangeToSqr(const Vector& pos) const
{
	return (GetAbsOrigin() - pos).LengthSqr();
}

float CBaseBot::GetRangeToSqr(edict_t* edict) const
{
	return (GetAbsOrigin() - edict->GetCollideable()->GetCollisionOrigin()).LengthSqr();
}

float CBaseBot::GetRangeToSqr(CBaseEntity* entity) const
{
	return (GetAbsOrigin() - reinterpret_cast<IServerEntity*>(entity)->GetCollideable()->GetCollisionOrigin()).LengthSqr();
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

bool CBaseBot::IsRangeGreaterThan(CBaseEntity* entity, const float range) const
{
	Vector to = (GetAbsOrigin() - reinterpret_cast<IServerEntity*>(entity)->GetCollideable()->GetCollisionOrigin());
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

bool CBaseBot::IsRangeLessThan(CBaseEntity* entity, const float range) const
{
	Vector to = (GetAbsOrigin() - reinterpret_cast<IServerEntity*>(entity)->GetCollideable()->GetCollisionOrigin());
	return to.Length() < range;
}

/**
 * @brief Checks if the bot is able to break the given entity
 * @param entity Entity the bot needs to break
 * @return true if the bot can break this entity
*/
bool CBaseBot::IsAbleToBreak(CBaseEntity* entity)
{
	int index = UtilHelpers::IndexOfEntity(entity);
	int takedamage = 0;

	if (entprops->GetEntProp(index, Prop_Data, "m_takedamage", takedamage) == true)
	{
		switch (takedamage)
		{
		case DAMAGE_NO:
			[[fallthrough]];
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
	
	if (entprops->GetEntProp(index, Prop_Data, "m_iHealth", health))
	{
		if (health > MAX_HEALTH_TO_BREAK)
		{
			return false; // don't bother
		}
	}

	CBaseEntity* damageFilter = nullptr;

	if (entprops->GetEntPropEnt(entity, Prop_Data, "m_hDamageFilter", nullptr, &damageFilter))
	{
		if (damageFilter)
		{
			return false; // entity has a damage filter assigned to it, assume we can't break
		}
	}

	auto classname = gamehelpers->GetEntityClassname(entity);

	if (strncmp(classname, "func_breakable", 14) == 0)
	{
		return true;
	}

	if (strncmp(classname, "func_breakable_surf", 19) == 0)
	{
		return true;
	}

	if (UtilHelpers::StringMatchesPattern(classname, "prop_phys*", 0))
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
	if (!RunPlayerCommands())
	{
		return;
	}

	int commandnumber = m_cmd.command_number;
	auto mover = GetMovementInterface();
	auto control = GetControlInterface();

	m_cmd.Reset();

	commandnumber++;
	m_cmd.command_number = commandnumber;
	m_cmd.tick_count = gpGlobals->tickcount;
	m_cmd.impulse = static_cast<byte>(m_impulse);
	m_cmd.viewangles = m_viewangles; // set view angles
	m_cmd.weaponselect = m_weaponselect; // send weapon select
	// TO-DO: weaponsubtype
	m_cmd.buttons = buttons; // send buttons
	m_cmd.random_seed = CBaseBot::s_usercmdrng.GetRandomInt<int>(0, 0x7fffffff);

	float forwardspeed = 0.0f;
	float sidespeed = 0.0f;
	float upspeed = 0.0f;
	const float desiredspeed = mover->GetDesiredSpeed();

	if ((buttons & INPUT_FORWARD) != 0)
	{
		forwardspeed = desiredspeed;
	}
	else if ((buttons & INPUT_BACK) != 0)
	{
		forwardspeed = -desiredspeed;
	}

	if ((buttons & INPUT_MOVERIGHT) != 0)
	{
		sidespeed = desiredspeed;
	}
	else if ((buttons & INPUT_MOVELEFT) != 0)
	{
		sidespeed = -desiredspeed;
	}

	if (control->IsPressingMoveUpButton())
	{
		upspeed = desiredspeed;
	}
	else if (control->IsPressingMoveDownButton())
	{
		upspeed = -desiredspeed;
	}

	if (control->ShouldApplyScale())
	{
		forwardspeed = forwardspeed * control->GetForwardScale();
		sidespeed = sidespeed * control->GetSideScale();
	}

	m_cmd.forwardmove = forwardspeed;
	m_cmd.sidemove = sidespeed;
	m_cmd.upmove = upspeed;

	RunUserCommand(&m_cmd);

	m_weaponselect = 0;
	m_impulse = 0;
}

void CBaseBot::RunUserCommand(CBotCmd* ucmd)
{
#if EXT_DEBUG
	// Check the user command for invalid values

	if (ke::IsNaN<float>(ucmd->forwardmove) || ke::IsInfinite<float>(ucmd->forwardmove))
	{
		smutils->LogError(myself, "%s Bogus CBotCmd::forwardmove", GetClientName());
	}

	if (ke::IsNaN<float>(ucmd->sidemove) || ke::IsInfinite<float>(ucmd->sidemove))
	{
		smutils->LogError(myself, "%s Bogus CBotCmd::sidemove", GetClientName());
	}

#endif // EXT_DEBUG

	if (!extension->ShouldCallRunPlayerCommand()) // this mod already calls runplayermove on bots, we send the bot actual cmd on the hook
	{
		return;
	}

	if (sdkcalls->IsProcessUsercmdsAvailable())
	{
		sdkcalls->CBasePlayer_ProcessUsercmds(GetEntity(), ucmd);
		SnapEyeAngles(ucmd->viewangles);
	}
	else
	{
		m_controller->RunPlayerMove(ucmd);
	}
}

IPlayerController* CBaseBot::GetControlInterface() const
{
	if (!m_basecontrol)
	{
		m_basecontrol = std::make_unique<IPlayerController>(const_cast<CBaseBot*>(this));
	}

	return m_basecontrol.get();
}

IMovement* CBaseBot::GetMovementInterface() const
{
	if (!m_basemover)
	{
		m_basemover = std::make_unique<IMovement>(const_cast<CBaseBot*>(this));
	}

	return m_basemover.get();
}

ISensor* CBaseBot::GetSensorInterface() const
{
	if (!m_basesensor)
	{
		m_basesensor = std::make_unique<ISensor>(const_cast<CBaseBot*>(this));
	}

	return m_basesensor.get();
}

IBehavior* CBaseBot::GetBehaviorInterface() const
{
	if (!m_basebehavior)
	{
		m_basebehavior = std::make_unique<CBaseBotBehavior>(const_cast<CBaseBot*>(this));
	}

	return m_basebehavior.get();
}

IInventory* CBaseBot::GetInventoryInterface() const
{
	if (!m_baseinventory)
	{
		m_baseinventory = std::make_unique<IInventory>(const_cast<CBaseBot*>(this));
	}

	return m_baseinventory.get();
}

ISquad* CBaseBot::GetSquadInterface() const
{
	if (!m_basesquad)
	{
		m_basesquad = std::make_unique<ISquad>(const_cast<CBaseBot*>(this));
	}

	return m_basesquad.get();
}

ICombat* CBaseBot::GetCombatInterface() const
{
	if (!m_basecombat)
	{
		m_basecombat = std::make_unique<ICombat>(const_cast<CBaseBot*>(this));
	}

	return m_basecombat.get();
}

ISharedBotMemory* CBaseBot::GetSharedMemoryInterface() const
{
	int team = 0;
	entprops->GetEntProp(GetIndex(), Prop_Send, "m_iTeamNum", team);
	return extmanager->GetMod()->GetSharedBotMemory(team);
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

	m_spawnTime = gpGlobals->curtime;
	m_homepos = GetAbsOrigin();
	Reset();
}

void CBaseBot::FirstSpawn()
{
}

void CBaseBot::Killed()
{
}

void CBaseBot::OnTakeDamage_Alive(const CTakeDamageInfo& info)
{
}

void CBaseBot::TryJoinGame()
{
	SourceMod::IGameConfig* gamedata = extension->GetExtensionGameData();

	const char* command = gamedata->GetKeyValue("BotJoinGameCommand");

	if (command)
	{
		std::string szValue(command);
		std::stringstream stream(szValue);
		std::string token;

		while (std::getline(stream, token, ','))
		{
			DelayedFakeClientCommand(token.c_str());
		}
	}
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
	if (m_cmdqueue.empty())
	{
		return;
	}

	if (!m_cmdtimer.HasStarted() || m_cmdtimer.IsGreaterThen(1.1f))
	{
		m_cmdtimer.Start();
		m_cmdsents = 0; // reset the counter
	}

	if (m_cmdsents >= CBaseBot::m_maxStringCommandsPerSecond)
	{
		return;
	}

	while (m_cmdsents <= CBaseBot::m_maxStringCommandsPerSecond)
	{
		// Keep pushing commands until empty or we hit the limit per second

		auto& next = m_cmdqueue.front();
		serverpluginhelpers->ClientCommand(GetEdict(), next.c_str());
		m_cmdqueue.pop();
		m_cmdsents++;

		if (m_cmdqueue.empty())
		{
			return;
		}
	}

}

bool CBaseBot::IsLineOfFireClear(const Vector& to) const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CBaseBot::IsLineOfFireClean", "NavBot");
#endif // EXT_VPROF_ENABLED

#if 0
	CBaseBotTraceFilterLineOfFire filter{ this };
#else
	CTraceFilterWorldAndPropsOnly filter;
#endif // 0
	trace_t result;
	trace::line(GetEyeOrigin(), to, MASK_SHOT, &filter, result);
	return !result.DidHit();
}

void CBaseBot::SendChatMessage(const char* message)
{
	constexpr std::size_t MAX_SIZE = 255U;

	std::unique_ptr<char[]> cmd = std::make_unique<char[]>(MAX_SIZE);

	ke::SafeSprintf(cmd.get(), MAX_SIZE, "say %s", message);
	DelayedFakeClientCommand(cmd.get());
}

void CBaseBot::SendTeamChatMessage(const char* message)
{
	constexpr std::size_t MAX_SIZE = 255U;

	std::unique_ptr<char[]> cmd = std::make_unique<char[]>(MAX_SIZE);

	ke::SafeSprintf(cmd.get(), MAX_SIZE, "say_team %s", message);
	DelayedFakeClientCommand(cmd.get());
}

CBaseBotTraceFilterLineOfFire::CBaseBotTraceFilterLineOfFire(const CBaseBot* bot, const bool ignoreAllies) :
	trace::CTraceFilterSimple(bot->GetEntity(), COLLISION_GROUP_NONE)
{
	m_bot = bot;
	m_sensor = bot->GetSensorInterface();
	m_ignoreAllies = ignoreAllies;
}

bool CBaseBotTraceFilterLineOfFire::ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask)
{
	if (trace::CTraceFilterSimple::ShouldHitEntity(pHandleEntity, contentsMask))
	{
		CBaseEntity* pEntity = trace::EntityFromEntityHandle(pHandleEntity);

		if (pEntity)
		{
			// Don't collide with enemies
			if (m_sensor->IsEnemy(pEntity))
			{
				return false;
			}

			if (!m_ignoreAllies && m_sensor->IsFriendly(pEntity))
			{
				return false;
			}
		}

		return true;
	}

	return false;
}