#include <extension.h>
#include <manager.h>
#include <extplayer.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/librandom.h>
#include <util/sdkcalls.h>
#include <mods/basemod.h>
#include <tier1/convar.h>
#include <sdkports/sdk_takedamageinfo.h>
#include <sdkports/sdk_traces.h>
#include "basebot_behavior.h"
#include "basebot.h"

ConVar cvar_bot_difficulty("sm_navbot_skill_level", "0", FCVAR_NONE, "Skill level group to use when selecting bot difficulty.");

int CBaseBot::m_maxStringCommandsPerSecond = 20;

CBaseBot::CBaseBot(edict_t* edict) : CBaseExtPlayer(edict),
	m_cmd(),
	m_viewangles(0.0f, 0.0f, 0.0f)
{
	m_profile = extmanager->GetBotDifficultyProfileManager().GetProfileForSkillLevel(cvar_bot_difficulty.GetInt());
	m_isfirstspawn = false;
	m_nextupdatetime = 64;
	m_joingametime = 64;
	m_controller = nullptr; // Because the bot is now allocated at 'OnClientPutInServer' no bot controller was created yet.
	m_listeners.reserve(8);
	m_basecontrol = nullptr;
	m_basemover = nullptr;
	m_basesensor = nullptr;
	m_basebehavior = nullptr;
	m_baseinventory = nullptr;
	m_weaponselect = 0;
	m_cmdtimer.Invalidate();
	m_cmdsents = 0;
	m_debugtextoffset = 0;
	m_shhooks.reserve(32);

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
	if (m_controller == nullptr)
	{
		m_controller = botmanager->GetBotController(GetEdict());
	}

#ifdef EXT_DEBUG
	ConColorMsg(Color(0, 255, 60, 255), "CBaseBot::PostAdd m_controller = %p \n", m_controller);
#endif // EXT_DEBUG
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
	// Execute the command at the end of the tick since the BOT AI might queue commands on this tick
	ExecuteQueuedCommands(); // Run queue commands
}

void CBaseBot::Reset()
{
	m_nextupdatetime = 1;

	for (auto iface : m_interfaces)
	{
		iface->Reset();
	}
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

	if (strncmp(classname, "func_breakable_surf", 19) == 0)
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

	if ((buttons & INPUT_FORWARD) != 0)
	{
		forwardspeed = mover->GetMovementSpeed();
	}
	else if ((buttons & INPUT_BACK) != 0)
	{
		forwardspeed = -mover->GetMovementSpeed();
	}

	if ((buttons & INPUT_MOVERIGHT) != 0)
	{
		sidespeed = mover->GetMovementSpeed();
	}
	else if ((buttons & INPUT_MOVELEFT) != 0)
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

	m_controller->RunPlayerMove(ucmd);
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
	m_homepos = GetAbsOrigin();
}

void CBaseBot::FirstSpawn()
{
}

void CBaseBot::OnTakeDamage_Alive(const CTakeDamageInfo& info)
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
	trace::CTraceFilterNoNPCsOrPlayers filter(GetEntity(), COLLISION_GROUP_NONE);
	trace_t result;
	trace::line(GetEyeOrigin(), to, MASK_SHOT, &filter, result);
	return !result.DidHit();
}

