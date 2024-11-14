#include <memory>
#include <stdexcept>

#include <extension.h>
#include <sm_argbuffer.h>
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

	if (!cfg_navbot->GetOffset("Weapon_Switch", &m_offsetof_cbc_weaponswitch))
	{
		smutils->LogError(myself, "Failed to get offset for CBaseCombatCharacter::Weapon_Switch!");
		fail = true;
	}

	if (!cfg_sdktools->GetOffset("Weapon_GetSlot", &m_offsetof_cbc_weaponslot))
	{
		smutils->LogError(myself, "Failed to get offset for CBaseCombatCharacter::Weapon_GetSlot from SDKTool's gamedata!");
		fail = true;
	}

	if (!cfg_navbot->GetOffset("CGameRules_ShouldCollide", &m_offsetof_cgr_shouldcollide))
	{
		smutils->LogError(myself, "Failed to get offset for CGameRules::ShouldCollide!");
		fail = true;
	}

	gameconfs->CloseGameConfigFile(cfg_navbot);
	gameconfs->CloseGameConfigFile(cfg_sdktools);
	cfg_navbot = nullptr;
	cfg_sdktools = nullptr;

	if (fail == false)
	{
		fail = !SetupCalls();
	}

	if (fail)
	{
		throw std::runtime_error("Failed to initialize SDK Caller gamedata!");
	}

	return true;
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

bool CSDKCaller::SetupCalls()
{
	SetupCBCWeaponSwitch();
	SetupCBCWeaponSlot();
	SetupCGRShouldCollide();

	if (m_call_cbc_weaponswitch == nullptr ||
		m_call_cbc_weaponslot == nullptr ||
		m_call_cgr_shouldcollide == nullptr)
	{
		return false;
	}

	return true;
}

void CSDKCaller::SetupCBCWeaponSwitch()
{
	using namespace SourceMod;

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
