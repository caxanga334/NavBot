#include <stdexcept>
#include <cstring>
#include <algorithm>

#include <extension.h>
#include <manager.h>
#include <util/sdkcalls.h>
#include <bot/blackmesa/bmbot.h>
#include "nav/bm_nav_mesh.h"
#include "blackmesadm_mod.h"

CBlackMesaDeathmatchMod::CBlackMesaDeathmatchMod()
{
	m_isTeamPlay = false;
	std::fill(m_maxCarry.begin(), m_maxCarry.end(), 0);

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

	BuildMaxCarryArray();

	if (!sdkcalls->IsGetBoneTransformAvailable())
	{
		smutils->LogError(myself, "SDK Call to virtual CBaseAnimating::GetBoneTransform function unavailable! Check gamedata.");
		// this is required for Black Mesa since the bot aim relies on it.
		throw std::runtime_error("Critical gamedata failure for Black Mesa!");
	}
}

void CBlackMesaDeathmatchMod::OnRoundStart()
{
	randomgen->RandomReSeed();
	CBaseBot::s_usercmdrng.RandomReSeed();
	CBaseBot::s_botrng.RandomReSeed();
}

void CBlackMesaDeathmatchMod::BuildMaxCarryArray()
{
	ConVarRef sk_ammo_9mm_max("sk_ammo_9mm_max");

	if (sk_ammo_9mm_max.IsValid())
	{
		m_maxCarry[static_cast<int>(blackmesa::Ammo_9mm)] = sk_ammo_9mm_max.GetInt();
	}

	ConVarRef sk_ammo_357_max("sk_ammo_357_max");

	if (sk_ammo_357_max.IsValid())
	{
		m_maxCarry[static_cast<int>(blackmesa::Ammo_357)] = sk_ammo_357_max.GetInt();
	}

	ConVarRef sk_ammo_bolt_max("sk_ammo_bolt_max");

	if (sk_ammo_bolt_max.IsValid())
	{
		m_maxCarry[static_cast<int>(blackmesa::Ammo_Bolts)] = sk_ammo_bolt_max.GetInt();
	}

	ConVarRef sk_ammo_buckshot_max("sk_ammo_buckshot_max");

	if (sk_ammo_buckshot_max.IsValid())
	{
		m_maxCarry[static_cast<int>(blackmesa::Ammo_12ga)] = sk_ammo_buckshot_max.GetInt();
	}

	ConVarRef sk_ammo_energy_max("sk_ammo_energy_max");

	if (sk_ammo_energy_max.IsValid())
	{
		m_maxCarry[static_cast<int>(blackmesa::Ammo_DepletedUranium)] = sk_ammo_energy_max.GetInt();
	}

	ConVarRef sk_ammo_grenade_mp5_max("sk_ammo_grenade_mp5_max");

	if (sk_ammo_grenade_mp5_max.IsValid())
	{
		m_maxCarry[static_cast<int>(blackmesa::Ammo_40mmGL)] = sk_ammo_grenade_mp5_max.GetInt();
	}

	ConVarRef sk_ammo_grenade_rpg_max("sk_ammo_grenade_rpg_max");

	if (sk_ammo_grenade_rpg_max.IsValid())
	{
		m_maxCarry[static_cast<int>(blackmesa::Ammo_Rockets)] = sk_ammo_grenade_rpg_max.GetInt();
	}

	ConVarRef sk_ammo_grenade_frag_max("sk_ammo_grenade_frag_max");

	if (sk_ammo_grenade_frag_max.IsValid())
	{
		m_maxCarry[static_cast<int>(blackmesa::Ammo_FragGrenades)] = sk_ammo_grenade_frag_max.GetInt();
	}

	ConVarRef sk_ammo_grenade_satchel_max("sk_ammo_grenade_satchel_max");

	if (sk_ammo_grenade_satchel_max.IsValid())
	{
		m_maxCarry[static_cast<int>(blackmesa::Ammo_Satchel)] = sk_ammo_grenade_satchel_max.GetInt();
	}

	ConVarRef sk_ammo_grenade_tripmine_max("sk_ammo_grenade_tripmine_max");

	if (sk_ammo_grenade_tripmine_max.IsValid())
	{
		m_maxCarry[static_cast<int>(blackmesa::Ammo_Tripmines)] = sk_ammo_grenade_tripmine_max.GetInt();
	}

	ConVarRef sk_ammo_hornet_max("sk_ammo_hornet_max");

	if (sk_ammo_hornet_max.IsValid())
	{
		m_maxCarry[static_cast<int>(blackmesa::Ammo_Hornets)] = sk_ammo_hornet_max.GetInt();
	}

	ConVarRef sk_ammo_snark_max("sk_ammo_snark_max");

	if (sk_ammo_snark_max.IsValid())
	{
		m_maxCarry[static_cast<int>(blackmesa::Ammo_Snarks)] = sk_ammo_snark_max.GetInt();
	}
}
