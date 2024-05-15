#include <extension.h>
#include <sdkports/sdk_takedamageinfo.h>
#include "basebot.h"

SH_DECL_MANUALHOOK0_void(CBaseBot_Spawn, 0, 0, 0)
SH_DECL_MANUALHOOK1_void(CBaseBot_Touch, 0, 0, 0, CBaseEntity*);
SH_DECL_MANUALHOOK1(CBaseBot_OnTakeDamage_Alive, 0, 0, 0, int, const CTakeDamageInfo&)
SH_DECL_MANUALHOOK1_void(CBaseBot_Event_Killed, 0, 0, 0, const CTakeDamageInfo&)
SH_DECL_MANUALHOOK2_void(CBaseBot_Event_KilledOther, 0, 0, 0, CBaseEntity*, const CTakeDamageInfo&)

bool CBaseBot::InitHooks(SourceMod::IGameConfig* gd_navbot, SourceMod::IGameConfig* gd_sdkhooks)
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

	return true;
}

// TO-DO: Move the PlayerRunCMD hook to here too!

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
	m_shhooks.push_back(SH_ADD_MANUALHOOK(CBaseBot_Touch, ifaceptr, SH_MEMBER(this, &CBaseBot::Hook_Touch), false));
	m_shhooks.push_back(SH_ADD_MANUALHOOK(CBaseBot_OnTakeDamage_Alive, ifaceptr, SH_MEMBER(this, &CBaseBot::Hook_OnTakeDamage_Alive), false));
	m_shhooks.push_back(SH_ADD_MANUALHOOK(CBaseBot_Event_Killed, ifaceptr, SH_MEMBER(this, &CBaseBot::Hook_Event_Killed), false));
	m_shhooks.push_back(SH_ADD_MANUALHOOK(CBaseBot_Event_KilledOther, ifaceptr, SH_MEMBER(this, &CBaseBot::Hook_Event_KilledOther), false));
}

void CBaseBot::Hook_Spawn()
{
	Spawn();
	RETURN_META(MRES_IGNORED);
}

void CBaseBot::Hook_Touch(CBaseEntity* pOther)
{
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
