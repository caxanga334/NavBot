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

	// ConVarRef is partially broken on Black Mesa
	ConVar* teamplay = g_pCVar->FindVar("mp_teamplay");

	if (teamplay == nullptr)
	{
		smutils->LogError(myself, "Failed to get ConVar pointer for \"mp_teamplay\"!");
	}
	else
	{
		if (teamplay->GetInt() != 0)
		{
			m_isTeamPlay = true;
		}
		else
		{
			m_isTeamPlay = false;
		}
	}
}

void CBlackMesaDeathmatchMod::OnRoundStart()
{
}
