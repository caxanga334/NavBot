#include NAVBOT_PCH_FILE
#include <extension.h>
#include <sdkports/sdk_takedamageinfo.h>
#include "basebot.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED


#if SOURCE_ENGINE == SE_TF2 || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM
class CBasePlayer;
#endif // SOURCE_ENGINE == SE_TF2 || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM

// Need this for CUserCmd class definition
#include <usercmd.h>

#ifdef EXT_DEBUG
static ConVar cvar_print_bot_hooks("sm_navbot_debug_bot_hooks", "0", FCVAR_GAMEDLL, "Prints console messages when the hooked game func callbacks are called.");
static ConVar cvar_print_bot_touch("sm_navbot_debug_bot_touch", "0", FCVAR_GAMEDLL, "Prints entities touched by the bot.");
#endif // EXT_DEBUG

namespace BotHookHelpers
{
	inline static void CopyBotCmdtoUserCmd(CUserCmd* ucmd, CBotCmd* bcmd)
	{
		ucmd->command_number = bcmd->command_number;
		ucmd->tick_count = bcmd->tick_count;
		ucmd->viewangles = bcmd->viewangles;
		ucmd->forwardmove = bcmd->forwardmove;
		ucmd->sidemove = bcmd->sidemove;
		ucmd->upmove = bcmd->upmove;
		ucmd->buttons = bcmd->buttons;
		ucmd->impulse = bcmd->impulse;
		ucmd->weaponselect = bcmd->weaponselect;
		ucmd->weaponsubtype = bcmd->weaponsubtype;
		ucmd->random_seed = bcmd->random_seed;
	}
}

#if METAMOD_PLAPI_VERSION < 18
SH_DECL_MANUALHOOK0_void(CBaseBot_Spawn, 0, 0, 0)
SH_DECL_MANUALHOOK1_void(CBaseBot_Touch, 0, 0, 0, CBaseEntity*);
SH_DECL_MANUALHOOK1(CBaseBot_OnTakeDamage_Alive, 0, 0, 0, int, const CTakeDamageInfo&)
SH_DECL_MANUALHOOK1_void(CBaseBot_Event_Killed, 0, 0, 0, const CTakeDamageInfo&)
SH_DECL_MANUALHOOK2_void(CBaseBot_Event_KilledOther, 0, 0, 0, CBaseEntity*, const CTakeDamageInfo&)
SH_DECL_MANUALHOOK0_void(CBaseBot_PhysicsSimulate, 0, 0, 0)
SH_DECL_MANUALHOOK2_void(CBaseBot_PlayerRunCommand, 0, 0, 0, CUserCmd*, IMoveHelper*)
SH_DECL_MANUALHOOK1_void(CBaseBot_Weapon_Equip, 0, 0, 0, CBaseEntity*)
SH_DECL_MANUALHOOK0(CBaseBot_CanBeAutobalanced, 0, 0, 0, bool)
#endif // METAMOD_PLAPI_VERSION < 18

bool CBaseBot::InitHooks(SourceMod::IGameConfig* gd_navbot, SourceMod::IGameConfig* gd_sdkhooks, SourceMod::IGameConfig* gd_sdktools)
{
	int offset = 0;
	CBaseBot::s_hookCanBeAutobalanced = false;
	CBaseBot::s_hookBCC = true;
	const char* szValue = gd_navbot->GetKeyValue("NoBaseCombatCharacter");

	if (szValue)
	{
		// if NoBaseCombatCharacter is true, then we don't hook CBaseCombatCharacters functions.
		CBaseBot::s_hookBCC = !UtilHelpers::StringToBoolean(szValue);
	}

#if METAMOD_PLAPI_VERSION >= 18
	if (!UtilHelpers::gamedata::GetOffset(gd_sdkhooks, gd_navbot, offset, "Spawn")) { return false; }
	CBaseBot::s_offsetSpawn = offset;

	if (!UtilHelpers::gamedata::GetOffset(gd_sdkhooks, gd_navbot, offset, "Touch")) { return false; }
	CBaseBot::s_offsetTouch = offset;

	if (CBaseBot::s_hookBCC)
	{
		if (!UtilHelpers::gamedata::GetOffset(gd_sdkhooks, gd_navbot, offset, "OnTakeDamage_Alive")) { return false; }
		CBaseBot::s_offsetOnTakeDamageAlive = offset;
	}
	else
	{
		// fallback to OnTakeDamage
		if (!UtilHelpers::gamedata::GetOffset(gd_sdkhooks, gd_navbot, offset, "OnTakeDamage")) { return false; }
		CBaseBot::s_offsetOnTakeDamageAlive = offset;
	}

	if (!gd_navbot->GetOffset("Event_Killed", &offset)) { smutils->LogError(myself, "Failed to setup hook CBasePlayer::Event_Killed"); return false; }
	CBaseBot::s_offsetEventKilled = offset;

	if (CBaseBot::s_hookBCC)
	{
		if (!gd_navbot->GetOffset("Event_KilledOther", &offset)) { smutils->LogError(myself, "Failed to setup hook CBasePlayer::Event_KilledOther"); return false; }
		CBaseBot::s_offsetEventKilledOther = offset;
	}

	if (!gd_navbot->GetOffset("PhysicsSimulate", &offset)) { smutils->LogError(myself, "Failed to setup hook CBasePlayer::PhysicsSimulate"); return false; }
	CBaseBot::s_offsetPhysicsSimulate = offset;

	if (!UtilHelpers::gamedata::GetOffset(gd_sdkhooks, gd_navbot, offset, "Weapon_Equip")) { return false; }
	CBaseBot::s_offsetWeaponEquip = offset;

	// This mod needs to hook CBasePlayer::PlayerRunCommand
	if (extension->ShouldHookRunPlayerCommand())
	{
		if (!UtilHelpers::gamedata::GetOffset(gd_sdktools, gd_navbot, offset, "PlayerRunCmd")) { return false; }
		CBaseBot::s_offsetRunPlayerCommand = offset;
	}

	szValue = gd_navbot->GetKeyValue("HookCanBeAutobalanced");

	if (szValue && UtilHelpers::StringToBoolean(szValue))
	{
		if (UtilHelpers::gamedata::GetOffset(gd_sdkhooks, gd_navbot, offset, "CanBeAutobalanced"))
		{
			CBaseBot::s_hookCanBeAutobalanced = true;
			CBaseBot::s_offsetCanBeAutoBalanced = offset;
		}
	}

#else
	if (!UtilHelpers::gamedata::GetOffset(gd_sdkhooks, gd_navbot, offset, "Spawn")) { return false; }
	SH_MANUALHOOK_RECONFIGURE(CBaseBot_Spawn, offset, 0, 0);

	if (!UtilHelpers::gamedata::GetOffset(gd_sdkhooks, gd_navbot, offset, "Touch")) { return false; }
	SH_MANUALHOOK_RECONFIGURE(CBaseBot_Touch, offset, 0, 0);

	if (CBaseBot::s_hookBCC)
	{
		if (!UtilHelpers::gamedata::GetOffset(gd_sdkhooks, gd_navbot, offset, "OnTakeDamage_Alive")) { return false; }
		SH_MANUALHOOK_RECONFIGURE(CBaseBot_OnTakeDamage_Alive, offset, 0, 0);
	}
	else
	{
		// fallback to OnTakeDamage
		if (!UtilHelpers::gamedata::GetOffset(gd_sdkhooks, gd_navbot, offset, "OnTakeDamage")) { return false; }
		SH_MANUALHOOK_RECONFIGURE(CBaseBot_OnTakeDamage_Alive, offset, 0, 0);
	}

	if (!gd_navbot->GetOffset("Event_Killed", &offset)) { smutils->LogError(myself, "Failed to setup hook CBasePlayer::Event_Killed"); return false; }
	SH_MANUALHOOK_RECONFIGURE(CBaseBot_Event_Killed, offset, 0, 0);

	if (CBaseBot::s_hookBCC)
	{
		if (!gd_navbot->GetOffset("Event_KilledOther", &offset)) { smutils->LogError(myself, "Failed to setup hook CBasePlayer::Event_KilledOther"); return false; }
		SH_MANUALHOOK_RECONFIGURE(CBaseBot_Event_KilledOther, offset, 0, 0);
	}

	if (!gd_navbot->GetOffset("PhysicsSimulate", &offset)) { smutils->LogError(myself, "Failed to setup hook CBasePlayer::PhysicsSimulate"); return false; }
	SH_MANUALHOOK_RECONFIGURE(CBaseBot_PhysicsSimulate, offset, 0, 0);

	if (!UtilHelpers::gamedata::GetOffset(gd_sdkhooks, gd_navbot, offset, "Weapon_Equip")) { return false; }
	SH_MANUALHOOK_RECONFIGURE(CBaseBot_Weapon_Equip, offset, 0, 0);

	// This mod needs to hook CBasePlayer::PlayerRunCommand
	if (extension->ShouldHookRunPlayerCommand())
	{
		if (!UtilHelpers::gamedata::GetOffset(gd_sdktools, gd_navbot, offset, "PlayerRunCmd")) { return false; }
		SH_MANUALHOOK_RECONFIGURE(CBaseBot_PlayerRunCommand, offset, 0, 0);
	}

	szValue = gd_navbot->GetKeyValue("HookCanBeAutobalanced");

	if (szValue && szValue[0] != '\0' && UtilHelpers::StringToBoolean(szValue))
	{
		if (UtilHelpers::gamedata::GetOffset(gd_sdkhooks, gd_navbot, offset, "CanBeAutobalanced"))
		{
			CBaseBot::s_hookCanBeAutobalanced = true;
			SH_MANUALHOOK_RECONFIGURE(CBaseBot_CanBeAutobalanced, offset, 0, 0);
		}
	}

#endif // METAMOD_PLAPI_VERSION >= 18

	return true;
}

void CBaseBot::AddHooks()
{
	CBaseEntity* ifaceptr = GetEntity();
	
#ifdef EXT_DEBUG
	if (ifaceptr == nullptr)
	{
		smutils->LogError(myself, "CBaseBot::AddHooks NULL CBaseEntity!");
	}
#endif // EXT_DEBUG


#if METAMOD_PLAPI_VERSION >= 18
	m_hookSpawn.Configure(CBaseBot::s_offsetSpawn);
	m_hookSpawn.AddContext(this, &CBaseBot::Hook_Spawn_Pre, &CBaseBot::Hook_Spawn_Post);
	m_hookSpawn.Add(ifaceptr);
	m_hookTouch.Configure(CBaseBot::s_offsetTouch);
	m_hookTouch.AddContext(this, &CBaseBot::Hook_Touch_Pre, nullptr);
	m_hookTouch.Add(ifaceptr);
	m_hookOnTakeDamageAlive.Configure(CBaseBot::s_offsetOnTakeDamageAlive);
	m_hookOnTakeDamageAlive.AddContext(this, &CBaseBot::Hook_OnTakeDamage_Alive_Pre, nullptr);
	m_hookOnTakeDamageAlive.Add(ifaceptr);
	m_hookEventKilled.Configure(CBaseBot::s_offsetEventKilled);
	m_hookEventKilled.AddContext(this, &CBaseBot::Hook_OnEventKilled_Pre, nullptr);
	m_hookEventKilled.Add(ifaceptr);
	m_hookPhysicsSimulate.Configure(CBaseBot::s_offsetPhysicsSimulate);
	m_hookPhysicsSimulate.AddContext(this, &CBaseBot::Hook_PhysicsSimulate_Pre, nullptr);
	m_hookPhysicsSimulate.Add(ifaceptr);
	m_hookWeaponEquip.Configure(CBaseBot::s_offsetWeaponEquip);
	m_hookWeaponEquip.AddContext(this, nullptr, &CBaseBot::Hook_WeaponEquip_Post);
	m_hookWeaponEquip.Add(ifaceptr);

	if (CBaseBot::s_hookBCC)
	{
		m_hookEventKilledOther.Configure(CBaseBot::s_offsetEventKilledOther);
		m_hookEventKilledOther.AddContext(this, &CBaseBot::Hook_OnEventKilledOther_Pre, nullptr);
		m_hookEventKilledOther.Add(ifaceptr);
	}

	if (extension->ShouldHookRunPlayerCommand())
	{
		m_hookPlayerRunCommand.Configure(CBaseBot::s_offsetRunPlayerCommand);
		m_hookPlayerRunCommand.AddContext(this, &CBaseBot::Hook_PlayerRunCommand_Pre, nullptr);
		m_hookPlayerRunCommand.Add(ifaceptr);
	}

	if (CBaseBot::s_hookCanBeAutobalanced)
	{
		m_hookCanBeAutoTeamBalanced.Configure(CBaseBot::s_offsetRunPlayerCommand);
		m_hookCanBeAutoTeamBalanced.AddContext(this, &CBaseBot::Hook_CanBeAutoTeamBalanced_Pre, nullptr);
		m_hookCanBeAutoTeamBalanced.Add(ifaceptr);
	}

#else
	// v17 or lower

	// Add hooks for this bot instance, hooks are removed on the destructor
	m_shhooks.push_back(SH_ADD_MANUALHOOK(CBaseBot_Spawn, ifaceptr, SH_MEMBER(this, &CBaseBot::Hook_Spawn), false));
	m_shhooks.push_back(SH_ADD_MANUALHOOK(CBaseBot_Spawn, ifaceptr, SH_MEMBER(this, &CBaseBot::Hook_Spawn_Post), true));
	m_shhooks.push_back(SH_ADD_MANUALHOOK(CBaseBot_Touch, ifaceptr, SH_MEMBER(this, &CBaseBot::Hook_Touch), false));
	m_shhooks.push_back(SH_ADD_MANUALHOOK(CBaseBot_OnTakeDamage_Alive, ifaceptr, SH_MEMBER(this, &CBaseBot::Hook_OnTakeDamage_Alive), false));
	m_shhooks.push_back(SH_ADD_MANUALHOOK(CBaseBot_Event_Killed, ifaceptr, SH_MEMBER(this, &CBaseBot::Hook_Event_Killed), false));
	m_shhooks.push_back(SH_ADD_MANUALHOOK(CBaseBot_PhysicsSimulate, ifaceptr, SH_MEMBER(this, &CBaseBot::Hook_PhysicsSimulate), false));
	m_shhooks.push_back(SH_ADD_MANUALHOOK(CBaseBot_Weapon_Equip, ifaceptr, SH_MEMBER(this, &CBaseBot::Hook_Weapon_Equip_Post), true));

	if (CBaseBot::s_hookBCC)
	{
		m_shhooks.push_back(SH_ADD_MANUALHOOK(CBaseBot_Event_KilledOther, ifaceptr, SH_MEMBER(this, &CBaseBot::Hook_Event_KilledOther), false));
	}

	if (extension->ShouldHookRunPlayerCommand())
	{
		m_shhooks.push_back(SH_ADD_MANUALHOOK(CBaseBot_PlayerRunCommand, ifaceptr, SH_MEMBER(this, &CBaseBot::Hook_PlayerRunCommand), false));
	}

	if (CBaseBot::s_hookCanBeAutobalanced)
	{
		m_shhooks.push_back(SH_ADD_MANUALHOOK(CBaseBot_CanBeAutobalanced, ifaceptr, SH_MEMBER(this, &CBaseBot::Hook_CanBeAutobalanced), false));
	}
#endif // METAMOD_PLAPI_VERSION >= 18
}
#if METAMOD_PLAPI_VERSION >= 18

KHook::Return<void> CBaseBot::Hook_Spawn_Pre(CBaseEntity* pEntity)
{
#ifdef EXT_DEBUG
	if (cvar_print_bot_hooks.GetBool())
	{
		META_CONPRINTF("CBaseBot::Hook_Spawn <%p>\n", this);
	}
#endif // EXT_DEBUG

	return { KHook::Action::Ignore };
}

KHook::Return<void> CBaseBot::Hook_Spawn_Post(CBaseEntity* pEntity)
{
#ifdef EXT_DEBUG
	if (cvar_print_bot_hooks.GetBool())
	{
		META_CONPRINTF("CBaseBot::Hook_Spawn_Post <%p> %3.2f\n", this, gpGlobals->curtime);
	}

#endif // EXT_DEBUG

	Spawn();

	return { KHook::Action::Ignore };
}

KHook::Return<void> CBaseBot::Hook_Touch_Pre(CBaseEntity* pEntity, CBaseEntity* pTouch)
{
#ifdef EXT_DEBUG
	if (cvar_print_bot_touch.GetBool() && pTouch != nullptr)
	{
		int index = gamehelpers->EntityToBCompatRef(pTouch);
		const char* classname = gamehelpers->GetEntityClassname(pTouch);

		// don't print worldspawn because it will spam the console
		if (index != 0)
		{
			META_CONPRINTF("%s Hook_Touch #%i <%s> \n", GetDebugIdentifier(), index, classname);
		}
	}
#endif // EXT_DEBUG

	Touch(pTouch);
	OnContact(pTouch);
	return { KHook::Action::Ignore };
}

KHook::Return<int> CBaseBot::Hook_OnTakeDamage_Alive_Pre(CBaseEntity* pEntity, const CTakeDamageInfo& info)
{
	if ((info.GetDamageType() & DMG_BURN) != 0)
	{
		if (!m_burningtimer.HasStarted() || m_burningtimer.IsGreaterThen(1.0f))
		{
			m_burningtimer.Start();
			OnIgnited(info);
		}
	}

	m_lasthurttimer.Start();

	OnTakeDamage_Alive(info);
	OnInjured(info);

	return { KHook::Action::Ignore, 0 };
}

KHook::Return<void> CBaseBot::Hook_OnEventKilled_Pre(CBaseEntity* pEntity, const CTakeDamageInfo& info)
{
	m_lastkilledtimer.Start();
	Killed();
	OnKilled(info);
	return { KHook::Action::Ignore };
}

KHook::Return<void> CBaseBot::Hook_OnEventKilledOther_Pre(CBaseEntity* pEntity, CBaseEntity* pVictim, const CTakeDamageInfo& info)
{
	OnOtherKilled(pVictim, info);
	return { KHook::Action::Ignore };
}

KHook::Return<void> CBaseBot::Hook_PhysicsSimulate_Pre(CBaseEntity* pEntity)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CBaseBot::Hook_PhysicsSimulate", "NavBot");
#endif // EXT_VPROF_ENABLED

#ifdef EXT_DEBUG
	if (pEntity != GetEntity())
	{
		Warning("CBaseBot::Hook_PhysicsSimulatethis != CBaseBot::m_pEntity %p != %p \n", pEntity, GetEntity());
	}
#endif // EXT_DEBUG

	// Don't simulate the bot more than once per frame.
	if (m_simulationtick == gpGlobals->tickcount)
	{
		return { KHook::Action::Ignore };
	}

	m_simulationtick = gpGlobals->tickcount;
	PlayerThink();
	return { KHook::Action::Ignore };
}

KHook::Return<void> CBaseBot::Hook_WeaponEquip_Post(CBaseEntity* pEntity, CBaseEntity* pWeapon)
{
#ifdef EXT_DEBUG
	if (cvar_print_bot_hooks.GetBool())
	{
		META_CONPRINTF("%s Hook_WeaponEquip_Post %p \"%s\"", GetDebugIdentifier(), pWeapon, UtilHelpers::textformat::FormatEntity(pWeapon));
	}
#endif

	this->OnWeaponEquip(pWeapon);
	return { KHook::Action::Ignore };
}

KHook::Return<void> CBaseBot::Hook_PlayerRunCommand_Pre(CBaseEntity* pEntity, CUserCmd* usercmd, IMoveHelper* movehelper)
{
	if (!RunPlayerCommands())
	{
		return { KHook::Action::Ignore };
	}

	CBotCmd* botcmd = GetUserCommand();
	BotHookHelpers::CopyBotCmdtoUserCmd(usercmd, botcmd);
	// MRES_HANDLED is gone, use Ignore instead
	return { KHook::Action::Ignore };
}

KHook::Return<bool> CBaseBot::Hook_CanBeAutoTeamBalanced_Pre(CBaseEntity* pEntity)
{
	bool useOriginal = false;
	bool result = this->CanBeAutoBalanced(useOriginal);

#ifdef EXT_DEBUG
	if (cvar_print_bot_hooks.GetBool())
	{
		META_CONPRINTF("%s Hook_CanBeAutobalanced Use Original: %s Result: %s\n", GetDebugIdentifier(),
			useOriginal ? "TRUE" : "FALSE", result ? "TRUE" : "FALSE");
	}
#endif

	if (!useOriginal)
	{
		return { KHook::Action::Supersede, result };
	}

	return { KHook::Action::Ignore, false };
}

#else
	// v17 or lower

void CBaseBot::Hook_Spawn()
{
#ifdef EXT_DEBUG
	if (cvar_print_bot_hooks.GetBool())
	{
		META_CONPRINTF("CBaseBot::Hook_Spawn <%p>\n", this);
	}
#endif // EXT_DEBUG

	RETURN_META(MRES_IGNORED);
}

void CBaseBot::Hook_Spawn_Post()
{
#ifdef EXT_DEBUG
	if (cvar_print_bot_hooks.GetBool())
	{
		META_CONPRINTF("CBaseBot::Hook_Spawn_Post <%p> %3.2f\n", this, gpGlobals->curtime);
	}

#endif // EXT_DEBUG

	Spawn();
	RETURN_META(MRES_IGNORED);
}

void CBaseBot::Hook_Touch(CBaseEntity* pOther)
{
#ifdef EXT_DEBUG
	if (cvar_print_bot_touch.GetBool() && pOther != nullptr)
	{
		int index = gamehelpers->EntityToBCompatRef(pOther);
		const char* classname = gamehelpers->GetEntityClassname(pOther);

		// don't print worldspawn because it will spam the console
		if (index != 0)
		{
			META_CONPRINTF("%s Hook_Touch #%i <%s> \n", GetDebugIdentifier(), index, classname);
		}
	}
#endif // EXT_DEBUG

	Touch(pOther);
	OnContact(pOther);
	RETURN_META(MRES_IGNORED);
}

int CBaseBot::Hook_OnTakeDamage_Alive(const CTakeDamageInfo& info)
{
	if (info.GetDamageType() & DMG_BURN)
	{
		if (!m_burningtimer.HasStarted() || m_burningtimer.IsGreaterThen(1.0f))
		{
			m_burningtimer.Start();
			OnIgnited(info);
		}
	}

	m_lasthurttimer.Start();

	OnTakeDamage_Alive(info);
	OnInjured(info);
	RETURN_META_VALUE(MRES_IGNORED, 0);
}

void CBaseBot::Hook_Event_Killed(const CTakeDamageInfo& info)
{
	m_lastkilledtimer.Start();
	Killed();
	OnKilled(info);
	RETURN_META(MRES_IGNORED);
}

void CBaseBot::Hook_Event_KilledOther(CBaseEntity* pVictim, const CTakeDamageInfo& info)
{
	OnOtherKilled(pVictim, info);
	RETURN_META(MRES_IGNORED);
}

void CBaseBot::Hook_PhysicsSimulate()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CBaseBot::Hook_PhysicsSimulate", "NavBot");
#endif // EXT_VPROF_ENABLED

#ifdef EXT_DEBUG
	if (m_controller == nullptr && !IsPluginBot() && botmanager != nullptr)
	{
		META_CONPRINTF("CBaseBot::Hook_PhysicsSimulate called with NULL m_controller <%p>\n", this);
	}

	// sanity check, see if the hook 'this' pointer isn't the bot itself
	CBaseEntity* pPlayer = META_IFACEPTR(CBaseEntity);

	if (pPlayer != GetEntity())
	{
		Warning("CBaseBot::Hook_PhysicsSimulatethis != CBaseBot::m_pEntity %p != %p \n", pPlayer, GetEntity());
	}

#endif // EXT_DEBUG

	// Don't simulate the bot more than once per frame.
	if (m_simulationtick == gpGlobals->tickcount)
	{
		RETURN_META(MRES_IGNORED);
	}

	m_simulationtick = gpGlobals->tickcount;
	PlayerThink();
	RETURN_META(MRES_IGNORED);
}

void CBaseBot::Hook_PlayerRunCommand(CUserCmd* usercmd, IMoveHelper* movehelper)
{
	if (!RunPlayerCommands())
	{
		RETURN_META(MRES_IGNORED);
	}

	CBotCmd* botcmd = GetUserCommand();
	BotHookHelpers::CopyBotCmdtoUserCmd(usercmd, botcmd);
	RETURN_META(MRES_HANDLED);
}

void CBaseBot::Hook_Weapon_Equip_Post(CBaseEntity* weapon)
{
#ifdef EXT_DEBUG
	if (cvar_print_bot_hooks.GetBool() && weapon)
	{
		int index = UtilHelpers::IndexOfEntity(weapon);
		const char* classname = gamehelpers->GetEntityClassname(weapon);
		META_CONPRINTF("%s Hook_Weapon_Equip_Post %p [%i] %s", GetDebugIdentifier(), weapon, index, classname);
	}
#endif

	this->OnWeaponEquip(weapon);

	RETURN_META(MRES_IGNORED);
}

bool CBaseBot::Hook_CanBeAutobalanced()
{
	bool useOriginal = false;
	bool result = this->CanBeAutoBalanced(useOriginal);

#ifdef EXT_DEBUG
	if (cvar_print_bot_hooks.GetBool())
	{
		META_CONPRINTF("%s Hook_CanBeAutobalanced Use Original: %s Result: %s\n", GetDebugIdentifier(),
			useOriginal ? "TRUE" : "FALSE", result ? "TRUE" : "FALSE");
	}
#endif

	if (!useOriginal)
	{
		RETURN_META_VALUE(MRES_SUPERCEDE, result);
	}

	RETURN_META_VALUE(MRES_IGNORED, false);
}

#endif // METAMOD_PLAPI_VERSION >= 18

