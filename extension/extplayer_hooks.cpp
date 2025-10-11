#include NAVBOT_PCH_FILE
#include "extension.h"
#include "extplayer.h"

SH_DECL_MANUALHOOK0_void(CBaseExtPlayer_PreThink, 0, 0, 0);

bool CBaseExtPlayer::InitHooks(SourceMod::IGameConfig* gd_navbot, SourceMod::IGameConfig* gd_sdkhooks, SourceMod::IGameConfig* gd_sdktools)
{
	int offset = 0;

	if (!gd_sdkhooks->GetOffset("PreThink", &offset))
	{
		smutils->LogError(myself, "Failed to get virutal offset to CBasePlayer::PreThink from SDKHooks gamedata!");
		return false;
	}

	SH_MANUALHOOK_RECONFIGURE(CBaseExtPlayer_PreThink, offset, 0, 0);

	return true;
}

void CBaseExtPlayer::SetupPlayerHooks()
{
	CBaseEntity* pThis = GetEntity();

	m_prethinkHook = SH_ADD_MANUALHOOK(CBaseExtPlayer_PreThink, pThis, SH_MEMBER(this, &CBaseExtPlayer::Hook_PreThink), false);

#ifdef EXT_DEBUG
	META_CONPRINTF("SetupPlayerHooks %p\n", pThis);
#endif // EXT_DEBUG
}

void CBaseExtPlayer::Hook_PreThink()
{
	PlayerThink();
}
