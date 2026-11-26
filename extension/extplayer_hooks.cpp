#include NAVBOT_PCH_FILE
#include "extension.h"
#include "extplayer.h"

#if METAMOD_PLAPI_VERSION <= 17
SH_DECL_MANUALHOOK0_void(CBaseExtPlayer_PreThink, 0, 0, 0);
#endif // METAMOD_PLAPI_VERSION <= 17

bool CBaseExtPlayer::InitHooks(SourceMod::IGameConfig* gd_navbot, SourceMod::IGameConfig* gd_sdkhooks, SourceMod::IGameConfig* gd_sdktools)
{
	int offset = 0;

#if METAMOD_PLAPI_VERSION >= 18
	if (!UtilHelpers::gamedata::GetOffset(gd_sdkhooks, gd_navbot, offset, "PreThink")) { return false; }
	CBaseExtPlayer::s_offsetPreThink = offset;
#else
	// v17 or lower
	if (!UtilHelpers::gamedata::GetOffset(gd_sdkhooks, gd_navbot, offset, "PreThink")) { return false; }
	SH_MANUALHOOK_RECONFIGURE(CBaseExtPlayer_PreThink, offset, 0, 0);
#endif // METAMOD_PLAPI_VERSION >= 18

	return true;
}

void CBaseExtPlayer::SetupPlayerHooks()
{
	CBaseEntity* pThis = GetEntity();

#if METAMOD_PLAPI_VERSION >= 18
	m_hookPreThink.Configure(CBaseExtPlayer::s_offsetPreThink);
	m_hookPreThink.AddContext(this, &CBaseExtPlayer::Hook_PreThink_Pre, nullptr);
	m_hookPreThink.Add(pThis);
#else
	m_prethinkHook = SH_ADD_MANUALHOOK(CBaseExtPlayer_PreThink, pThis, SH_MEMBER(this, &CBaseExtPlayer::Hook_PreThink), false);
#endif // METAMOD_PLAPI_VERSION >= 18

#ifdef EXT_DEBUG
	META_CONPRINTF("SetupPlayerHooks %p\n", pThis);
#endif // EXT_DEBUG
}


#if METAMOD_PLAPI_VERSION <= 17

void CBaseExtPlayer::Hook_PreThink()
{
	PlayerThink();

	RETURN_META(MRES_IGNORED);
}

#endif // METAMOD_PLAPI_VERSION <= 17

#if METAMOD_PLAPI_VERSION >= 18

KHook::Return<void> CBaseExtPlayer::Hook_PreThink_Pre(CBaseEntity* pThis)
{
	PlayerThink();
	return { KHook::Action::Ignore };
}

#endif // METAMOD_PLAPI_VERSION >= 18
