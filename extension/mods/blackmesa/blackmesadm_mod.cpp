#include <cstring>

#include <extension.h>
#include <manager.h>
#include <bot/blackmesa/bmbot.h>
#include "nav/bm_nav_mesh.h"
#include "blackmesadm_mod.h"

CBlackMesaDeathmatchMod::CBlackMesaDeathmatchMod()
{
	m_isTeamPlay = false;

	ListenForGameEvent("round_start");
}

CBlackMesaDeathmatchMod::~CBlackMesaDeathmatchMod()
{
}

CBlackMesaDeathmatchMod* CBlackMesaDeathmatchMod::GetBMMod()
{
	return static_cast<CBlackMesaDeathmatchMod*>(extmanager->GetMod());
}

void CBlackMesaDeathmatchMod::FireGameEvent(IGameEvent* event)
{
	CBaseMod::FireGameEvent(event);

	if (event)
	{
		const char* name = event->GetName();

		if (std::strcmp(name, "round_start") == 0)
		{
			OnNewRound();
			OnRoundStart();
		}
	}
}

CBaseBot* CBlackMesaDeathmatchMod::AllocateBot(edict_t* edict)
{
	return new CBlackMesaBot(edict);
}

CNavMesh* CBlackMesaDeathmatchMod::NavMeshFactory()
{
	return new CBlackMesaNavMesh;
}

void CBlackMesaDeathmatchMod::OnMapStart()
{
	CBaseMod::OnMapStart();

	ConVarRef mp_teamplay("mp_teamplay");

	if (mp_teamplay.IsValid())
	{
		m_isTeamPlay = mp_teamplay.GetBool();
	}
	else
	{
		m_isTeamPlay = false;
		smutils->LogError(myself, "Failed to find ConVar \"mp_teamplay\"!");
	}

#ifdef EXT_DEBUG
	rootconsole->ConsolePrint("Team Play is %s", m_isTeamPlay ? "ON" : "OFF");
#endif // EXT_DEBUG
}

void CBlackMesaDeathmatchMod::OnRoundStart()
{
}
