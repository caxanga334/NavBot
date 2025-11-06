#include NAVBOT_PCH_FILE
#include <memory>
#include <stdexcept>
#include <cstring>

#include <extension.h>

#if SOURCE_ENGINE == SE_TF2 || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM
class CBasePlayer;
#endif // SOURCE_ENGINE == SE_TF2 || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM

#include <usercmd.h>
#include <sm_argbuffer.h>
#include <sdkports/sdk_entityoutput.h>
#include "sdkcalls.h"

static CSDKCaller s_sdkcalls;
CSDKCaller* sdkcalls = &s_sdkcalls;

CSDKCaller::CSDKCaller()
{
	m_init = false;
	m_offsetof_cbc_weaponswitch = invalid_offset();
	m_call_cbc_weaponswitch = nullptr;
	m_offsetof_cbc_weaponslot = invalid_offset();
	m_call_cbc_weaponslot = nullptr;
	m_offsetof_cgr_shouldcollide = invalid_offset();
	m_call_cgr_shouldcollide = nullptr;
	m_offsetof_cbp_processusercmds = invalid_offset();
	m_call_cbp_processusercmds = nullptr;
	m_offsetof_cba_getbonetransform = invalid_offset();
	m_call_cba_getbonetransform = nullptr;
	InitVCallSetup(m_call_cbe_acceptinput);
	InitVCallSetup(m_call_cbe_teleport);
}

CSDKCaller::~CSDKCaller()
{
}

bool CSDKCaller::Init()
{
	using namespace SourceMod;
	if (m_init) { return true; }

	constexpr size_t size = 512;
	std::unique_ptr<char[]> error = std::make_unique<char[]>(size);
	bool fail = false;
	IGameConfig* cfg_navbot = nullptr;
	IGameConfig* cfg_sdktools = nullptr;
	IGameConfig* cfg_sdkhooks = nullptr;
	
	if (!gameconfs->LoadGameConfigFile("navbot.games", &cfg_navbot, error.get(), size))
	{
		smutils->LogError(myself, "Failed to initialize SDK Caller! Error: %s", error.get());
		return false;
	}

	if (!gameconfs->LoadGameConfigFile("sdktools.games", &cfg_sdktools, error.get(), size))
	{
		smutils->LogError(myself, "Failed to initialize SDK Caller! Error: %s", error.get());
		return false;
	}

	if (!gameconfs->LoadGameConfigFile("sdkhooks.games", &cfg_sdkhooks, error.get(), size))
	{
		smutils->LogError(myself, "Failed to initialize SDK Caller! Error: %s", error.get());
		return false;
	}

	if (!cfg_sdkhooks->GetOffset("Weapon_Switch", &m_offsetof_cbc_weaponswitch))
	{
		smutils->LogError(myself, "Failed to get offset for CBaseCombatCharacter::Weapon_Switch!");
		fail = true;
	}

	if (!cfg_sdktools->GetOffset("Weapon_GetSlot", &m_offsetof_cbc_weaponslot))
	{
		smutils->LogError(myself, "Failed to get offset for CBaseCombatCharacter::Weapon_GetSlot from SDKTool's gamedata!");
		fail = true;
	}

	if (!cfg_navbot->GetOffset("CGameRules::ShouldCollide", &m_offsetof_cgr_shouldcollide))
	{
		smutils->LogError(myself, "Failed to get offset for CGameRules::ShouldCollide!");
		fail = true;
	}

	if (!cfg_navbot->GetOffset("CBasePlayer::ProcessUsercmds", &m_offsetof_cbp_processusercmds))
	{
		// don't fail, this is optional
		m_offsetof_cbp_processusercmds = invalid_offset();
	}

	if (!cfg_navbot->GetOffset("CBaseAnimating::GetBoneTransform", &m_offsetof_cba_getbonetransform))
	{
		// don't fail, this is optional
		m_offsetof_cba_getbonetransform = invalid_offset();
	}

	if (!cfg_sdktools->GetOffset("AcceptInput", &m_call_cbe_acceptinput.first))
	{
		// don't fail, this is optional
		m_call_cbe_acceptinput.first = invalid_offset();
	}

	if (!cfg_sdktools->GetOffset("Teleport", &m_call_cbe_teleport.first))
	{
		// don't fail, this is optional
		m_call_cbe_teleport.first = invalid_offset();
	}

	gameconfs->CloseGameConfigFile(cfg_navbot);
	gameconfs->CloseGameConfigFile(cfg_sdktools);
	gameconfs->CloseGameConfigFile(cfg_sdkhooks);
	cfg_navbot = nullptr;
	cfg_sdktools = nullptr;
	cfg_sdkhooks = nullptr;

	return !fail;
}

void CSDKCaller::PostInit()
{
	if (!SetupCalls())
	{
		smutils->LogError(myself, "One or more SDK calls setup failed! Extension will probably crash!");
	}
}

bool CSDKCaller::CBaseCombatCharacter_Weapon_Switch(CBaseEntity* pBCC, CBaseEntity* pWeapon)
{
	ArgBuffer<void*, void*, int> vstk(pBCC, pWeapon, 0);
	bool returnvalue = false;
	m_call_cbc_weaponswitch->Execute(vstk, &returnvalue);
	return returnvalue;
}

CBaseEntity* CSDKCaller::CBaseCombatCharacter_Weapon_GetSlot(CBaseEntity* pBCC, int slot)
{
	ArgBuffer<void*, int> vstk(pBCC, slot);
	CBaseEntity* result = nullptr;
	m_call_cbc_weaponslot->Execute(vstk, &result);
	return result;
}

bool CSDKCaller::CGameRules_ShouldCollide(CGameRules* pGR, int collisionGroup0, int collisionGroup1)
{
	ArgBuffer<void*, int, int> vstk(pGR, collisionGroup0, collisionGroup1);
	bool result = false;
	m_call_cgr_shouldcollide->Execute(vstk, &result);
	return result;
}

void CSDKCaller::CBasePlayer_ProcessUsercmds(CBaseEntity* pBP, CBotCmd* botcmd)
{
	CUserCmd ucmd;

	ucmd.command_number = botcmd->command_number;
	ucmd.tick_count = botcmd->tick_count;
	ucmd.viewangles = botcmd->viewangles;
	ucmd.forwardmove = botcmd->forwardmove;
	ucmd.sidemove = botcmd->sidemove;
	ucmd.upmove = botcmd->upmove;
	ucmd.impulse = botcmd->impulse;
	ucmd.buttons = botcmd->buttons;
	ucmd.weaponselect = botcmd->weaponselect;
	ucmd.weaponsubtype = botcmd->weaponsubtype;
	ucmd.random_seed = botcmd->random_seed;

	/* void CBasePlayer::ProcessUsercmds( CUserCmd *cmds, int numcmds, int totalcmds, int dropped_packets, bool paused ) */

	ArgBuffer<void*, void*, int, int, int, bool> vstk(pBP, static_cast<void*>(&ucmd), 1, 1, 0, false);

	m_call_cbp_processusercmds->Execute(vstk, nullptr);
}

void CSDKCaller::CBaseAnimating_GetBoneTransform(CBaseEntity* pBA, int bone, matrix3x4_t* result)
{
	/* void CBaseAnimating::GetBoneTransform( int iBone, matrix3x4_t &pBoneToWorld ) */

	ArgBuffer<void*, int, void*> vstk(pBA, bone, result);
	m_call_cba_getbonetransform->Execute(vstk, nullptr);
}

bool CSDKCaller::CBaseEntity_AcceptInput(CBaseEntity* pThis, const char* szInputName, CBaseEntity* pActivator, CBaseEntity* pCaller, variant_t variant, int outputID)
{
	ArgBuffer<void*, const char*, CBaseEntity*, CBaseEntity*, decltype(variant), int> vstk(static_cast<void*>(pThis), szInputName, pActivator, pCaller, variant, outputID);
	bool ret = false;

	m_call_cbe_acceptinput.second->Execute(vstk, &ret);

	return ret;
}

void CSDKCaller::CBaseEntity_Teleport(CBaseEntity* pThis, const Vector* origin, const QAngle* angles, const Vector* velocity)
{
	ArgBuffer<void*, const void*, const void*, const void*> vstk{ pThis, origin, angles, velocity };
	m_call_cbe_teleport.second->Execute(vstk, nullptr);
}

bool CSDKCaller::SetupCalls()
{
	SetupCBCWeaponSwitch();
	SetupCBCWeaponSlot();
	SetupCGRShouldCollide();
	SetupCBPProcessUserCmds();
	SetupCBAGetBoneTransform();
	SetupCBEAcceptInput();
	SetupCBETeleport();

	if (m_call_cbc_weaponswitch == nullptr ||
		m_call_cbc_weaponslot == nullptr ||
		m_call_cgr_shouldcollide == nullptr)
	{
		return false;
	}

	// this is optional
	if (m_offsetof_cbp_processusercmds > 0 && m_call_cbp_processusercmds == nullptr)
	{
		return false;
	}

	if (m_offsetof_cba_getbonetransform > 0 && m_call_cba_getbonetransform == nullptr)
	{
		return false;
	}

	return true;
}

void CSDKCaller::SetupCBCWeaponSwitch()
{
	using namespace SourceMod;

	if (m_offsetof_cbc_weaponswitch <= 0) { return; }

	/* bool CBaseCombatCharacter::Weapon_Switch(CBaseCombatWeapon *pWeapon, int viewmodelindex) */

	PassInfo ret;
	ret.flags = PASSFLAG_BYVAL;
	ret.size = sizeof(bool);
	ret.type = PassType_Basic;

	PassInfo params[2];

	params[0].flags = PASSFLAG_BYVAL;
	params[0].size = sizeof(void*);
	params[0].type = PassType_Basic;

	params[1].flags = PASSFLAG_BYVAL;
	params[1].size = sizeof(int);
	params[1].type = PassType_Basic;

	m_call_cbc_weaponswitch = g_pBinTools->CreateVCall(m_offsetof_cbc_weaponswitch, 0, 0, &ret, params, 2);
}

void CSDKCaller::SetupCBCWeaponSlot()
{
	using namespace SourceMod;

	if (m_offsetof_cbc_weaponslot <= 0) { return; }

	/* CBaseCombatWeapon *CBaseCombatCharacter::Weapon_GetSlot( int slot ) */

	PassInfo ret;
	ret.flags = PASSFLAG_BYVAL;
	ret.size = sizeof(void*);
	ret.type = PassType_Basic;

	PassInfo params[1];
	params[0].flags = PASSFLAG_BYVAL;
	params[0].size = sizeof(int);
	params[0].type = PassType_Basic;

	m_call_cbc_weaponslot = g_pBinTools->CreateVCall(m_offsetof_cbc_weaponslot, 0, 0, &ret, params, 1);
}

void CSDKCaller::SetupCGRShouldCollide()
{
	using namespace SourceMod;

	if (m_offsetof_cgr_shouldcollide <= 0) { return; }

	/* bool CGameRules::ShouldCollide(int, int) */

	PassInfo ret;
	ret.flags = PASSFLAG_BYVAL;
	ret.size = sizeof(bool);
	ret.type = PassType_Basic;

	PassInfo params[2];
	params[0].flags = PASSFLAG_BYVAL;
	params[0].size = sizeof(int);
	params[0].type = PassType_Basic;
	params[1].flags = PASSFLAG_BYVAL;
	params[1].size = sizeof(int);
	params[1].type = PassType_Basic;

	m_call_cgr_shouldcollide = g_pBinTools->CreateVCall(m_offsetof_cgr_shouldcollide, 0, 0, &ret, params, 2);
}

void CSDKCaller::SetupCBPProcessUserCmds()
{
	using namespace SourceMod;

	// this is optional
	if (m_offsetof_cbp_processusercmds <= 0) { return; }

	/* void CBasePlayer::ProcessUsercmds( CUserCmd *cmds, int numcmds, int totalcmds, int dropped_packets, bool paused ) */

	PassInfo params[5];
	params[0].flags = PASSFLAG_BYVAL;
	params[0].size = sizeof(CUserCmd*);
	params[0].type = PassType_Basic;
	params[1].flags = PASSFLAG_BYVAL;
	params[1].size = sizeof(int);
	params[1].type = PassType_Basic;
	params[2].flags = PASSFLAG_BYVAL;
	params[2].size = sizeof(int);
	params[2].type = PassType_Basic;
	params[3].flags = PASSFLAG_BYVAL;
	params[3].size = sizeof(int);
	params[3].type = PassType_Basic;
	params[4].flags = PASSFLAG_BYVAL;
	params[4].size = sizeof(bool);
	params[4].type = PassType_Basic;

	m_call_cbp_processusercmds = g_pBinTools->CreateVCall(m_offsetof_cbp_processusercmds, 0, 0, nullptr, params, 5);
}

void CSDKCaller::SetupCBAGetBoneTransform()
{
	using namespace SourceMod;

	if (m_offsetof_cba_getbonetransform <= 0) { return; }

	/* void CBaseAnimating::GetBoneTransform( int iBone, matrix3x4_t &pBoneToWorld ) */

	PassInfo params[2];
	params[0].flags = PASSFLAG_BYVAL;
	params[0].size = sizeof(int);
	params[0].type = PassType_Basic;
	params[1].flags = PASSFLAG_BYVAL;
	params[1].size = sizeof(void*);
	params[1].type = PassType_Basic;


	m_call_cba_getbonetransform = g_pBinTools->CreateVCall(m_offsetof_cba_getbonetransform, 0, 0, nullptr, params, 2);
}

void CSDKCaller::SetupCBEAcceptInput()
{
	using namespace SourceMod;

	if (m_call_cbe_acceptinput.first <= 0) { return; }

	// yoink from SDKTools
	PassInfo pass[6];
	pass[0].type = PassType_Basic;
	pass[0].flags = PASSFLAG_BYVAL;
	pass[0].size = sizeof(const char*);
	pass[1].type = pass[2].type = PassType_Basic;
	pass[1].flags = pass[2].flags = PASSFLAG_BYVAL;
	pass[1].size = pass[2].size = sizeof(CBaseEntity*);
	pass[3].type = PassType_Object;
	pass[3].flags = PASSFLAG_BYVAL | PASSFLAG_OCTOR | PASSFLAG_ODTOR | PASSFLAG_OASSIGNOP;
	pass[3].size = sizeof(variant_t);
	pass[4].type = PassType_Basic;
	pass[4].flags = PASSFLAG_BYVAL;
	pass[4].size = sizeof(int);
	pass[5].type = PassType_Basic;
	pass[5].flags = PASSFLAG_BYVAL;
	pass[5].size = sizeof(bool);

	m_call_cbe_acceptinput.second = g_pBinTools->CreateVCall(m_call_cbe_acceptinput.first, 0, 0, &pass[5], pass, 5);
}

void CSDKCaller::SetupCBETeleport()
{
	using namespace SourceMod;

	if (m_call_cbe_teleport.first <= 0) { return; }

	PassInfo info[3];
	info[0].flags = info[1].flags = info[2].flags = PASSFLAG_BYVAL;
	info[0].size = info[1].size = info[2].size = sizeof(void*);
	info[0].type = info[1].type = info[2].type = PassType_Basic;

	m_call_cbe_teleport.second = g_pBinTools->CreateVCall(m_call_cbe_teleport.first, 0, 0, NULL, info, 3);
}
