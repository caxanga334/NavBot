#include <extension.h>
#include <sdkports/sdk_takedamageinfo.h>
#include "basebot.h"

#if SOURCE_ENGINE == SE_TF2 || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM
class CBasePlayer;
#endif // SOURCE_ENGINE == SE_TF2 || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM

// Need this for CUserCmd class definition
#include <usercmd.h>

#ifdef EXT_DEBUG
static ConVar cvar_print_bot_touch("sm_navbot_debug_bot_touch", "Prints entities touched by the bot", FCVAR_GAMEDLL);
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

SH_DECL_MANUALHOOK0_void(CBaseBot_Spawn, 0, 0, 0)
SH_DECL_MANUALHOOK1_void(CBaseBot_Touch, 0, 0, 0, CBaseEntity*);
SH_DECL_MANUALHOOK1(CBaseBot_OnTakeDamage_Alive, 0, 0, 0, int, const CTakeDamageInfo&)
SH_DECL_MANUALHOOK1_void(CBaseBot_Event_Killed, 0, 0, 0, const CTakeDamageInfo&)
SH_DECL_MANUALHOOK2_void(CBaseBot_Event_KilledOther, 0, 0, 0, CBaseEntity*, const CTakeDamageInfo&)
SH_DECL_MANUALHOOK0_void(CBaseBot_PhysicsSimulate, 0, 0, 0)
SH_DECL_MANUALHOOK2_void(CBaseBot_PlayerRunCommand, 0, 0, 0, CUserCmd*, IMoveHelper*)
SH_DECL_MANUALHOOK1_void(CBaseBot_Weapon_Equip, 0, 0, 0, CBaseEntity*)

bool CBaseBot::InitHooks(SourceMod::IGameConfig* gd_navbot, SourceMod::IGameConfig* gd_sdkhooks, SourceMod::IGameConfig* gd_sdktools)
{
	int offset = 0;

	if (!gd_sdkhooks->GetOffset("Spawn", &offset)) { return false; }
	SH_MANUALHOOK_RECONFIGURE(CBaseBot_Spawn, offset, 0, 0);

	if (!gd_sdkhooks->GetOffset("Touch", &offset)) { return false; }
	SH_MANUALHOOK_RECONFIGURE(CBaseBot_Touch, offset, 0, 0);

	if (!gd_sdkhooks->GetOffset("OnTakeDamage_Alive", &offset)) { return false; }
	SH_MANUALHOOK_RECONFIGURE(CBaseBot_OnTakeDamage_Alive, offset, 0, 0);

	if (!gd_navbot->GetOffset("Event_Killed", &offset)) { return false; }
	SH_MANUALHOOK_RECONFIGURE(CBaseBot_Event_Killed, offset, 0, 0);

	if (!gd_navbot->GetOffset("Event_KilledOther", &offset)) { return false; }
	SH_MANUALHOOK_RECONFIGURE(CBaseBot_Event_KilledOther, offset, 0, 0);

	if (!gd_navbot->GetOffset("PhysicsSimulate", &offset)) { return false; }
	SH_MANUALHOOK_RECONFIGURE(CBaseBot_PhysicsSimulate, offset, 0, 0);

	if (!gd_sdkhooks->GetOffset("Weapon_Equip", &offset)) { return false; }
	SH_MANUALHOOK_RECONFIGURE(CBaseBot_Weapon_Equip, offset, 0, 0);

	// This mod needs to hook CBasePlayer::PlayerRunCommand
	if (extension->ShouldHookRunPlayerCommand())
	{
		if (!gd_sdktools->GetOffset("PlayerRunCmd", &offset)) { return false; }
		SH_MANUALHOOK_RECONFIGURE(CBaseBot_PlayerRunCommand, offset, 0, 0);
	}

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

	// Add hooks for this bot instance, hooks are removed on the destructor
	m_shhooks.push_back(SH_ADD_MANUALHOOK(CBaseBot_Spawn, ifaceptr, SH_MEMBER(this, &CBaseBot::Hook_Spawn), false));
	m_shhooks.push_back(SH_ADD_MANUALHOOK(CBaseBot_Spawn, ifaceptr, SH_MEMBER(this, &CBaseBot::Hook_Spawn_Post), true));
	m_shhooks.push_back(SH_ADD_MANUALHOOK(CBaseBot_Touch, ifaceptr, SH_MEMBER(this, &CBaseBot::Hook_Touch), false));
	m_shhooks.push_back(SH_ADD_MANUALHOOK(CBaseBot_OnTakeDamage_Alive, ifaceptr, SH_MEMBER(this, &CBaseBot::Hook_OnTakeDamage_Alive), false));
	m_shhooks.push_back(SH_ADD_MANUALHOOK(CBaseBot_Event_Killed, ifaceptr, SH_MEMBER(this, &CBaseBot::Hook_Event_Killed), false));
	m_shhooks.push_back(SH_ADD_MANUALHOOK(CBaseBot_Event_KilledOther, ifaceptr, SH_MEMBER(this, &CBaseBot::Hook_Event_KilledOther), false));
	m_shhooks.push_back(SH_ADD_MANUALHOOK(CBaseBot_PhysicsSimulate, ifaceptr, SH_MEMBER(this, &CBaseBot::Hook_PhysicsSimulate), false));
	m_shhooks.push_back(SH_ADD_MANUALHOOK(CBaseBot_Weapon_Equip, ifaceptr, SH_MEMBER(this, &CBaseBot::Hook_Weapon_Equip_Post), true));

	if (extension->ShouldHookRunPlayerCommand())
	{
		m_shhooks.push_back(SH_ADD_MANUALHOOK(CBaseBot_PlayerRunCommand, ifaceptr, SH_MEMBER(this, &CBaseBot::Hook_PlayerRunCommand), false));
	}
}

void CBaseBot::Hook_Spawn()
{
#ifdef EXT_DEBUG
	ConColorMsg(Color(0, 150, 0, 255), "CBaseBot::Hook_Spawn <%p>\n", this);
#endif // EXT_DEBUG
	
	RETURN_META(MRES_IGNORED);
}

void CBaseBot::Hook_Spawn_Post()
{
#ifdef EXT_DEBUG
	ConColorMsg(Color(0, 150, 0, 255), "CBaseBot::Hook_Spawn_Post <%p>\n", this);
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
#ifdef EXT_DEBUG
	if (m_controller == nullptr && !IsPluginBot())
	{
		ConColorMsg(Color(255, 0, 0, 255), "CBaseBot::Hook_PhysicsSimulate called with NULL m_controller <%p>\n", this);
	}

	// sanity check, see if the hook 'this' pointer isn't the bot itself
	CBaseEntity* pPlayer = META_IFACEPTR(CBaseEntity);

	if (pPlayer != GetEntity())
	{
		Warning(__FUNCTION__ " this != CBaseBot::m_pEntity %p != %p \n", pPlayer, GetEntity());
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
	this->OnWeaponEquip(weapon);

	RETURN_META(MRES_IGNORED);
}
