#include <extension.h>
#include <manager.h>
#include <bot/blackmesa/bmbot.h>
#include "blackmesadm_mod.h"

CBlackMesaDeathmatchMod::CBlackMesaDeathmatchMod()
{
	m_isTeamPlay = false;
}

CBlackMesaDeathmatchMod::~CBlackMesaDeathmatchMod()
{
}

CBlackMesaDeathmatchMod* CBlackMesaDeathmatchMod::GetBMMod()
{
	return static_cast<CBlackMesaDeathmatchMod*>(extmanager->GetMod());
}

CBaseBot* CBlackMesaDeathmatchMod::AllocateBot(edict_t* edict)
{
	return new CBlackMesaBot(edict);
}

void CBlackMesaDeathmatchMod::OnMapStart()
{
	CBaseMod::OnMapStart();

	ConVarRef teamplay("mp_teamplay");

	if (!teamplay.IsValid())
	{
		smutils->LogError(myself, "Failed to find ConVar \"mp_teamplay\"!");
	}
	else
	{
		m_isTeamPlay = teamplay.GetBool();
	}
}
