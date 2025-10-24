#include NAVBOT_PCH_FILE
#include <bot/zps/zpsbot.h>
#include <navmesh/nav_mesh.h>
#include "zps_mod.h"

CZombiePanicSourceMod::CZombiePanicSourceMod()
{
	m_gamemode = zps::ZPSGamemodes::GAMEMODE_SURVIVAL;
	m_roundactive = false;

	ListenForGameEvent("endslate");
	ListenForGameEvent("clientsound");
}

CZombiePanicSourceMod* CZombiePanicSourceMod::GetZPSMod()
{
	return static_cast<CZombiePanicSourceMod*>(extmanager->GetMod());
}

void CZombiePanicSourceMod::FireGameEvent(IGameEvent* event)
{
	if (event)
	{
		const char* name = event->GetName();

#ifdef EXT_DEBUG
		META_CONPRINTF("CZombiePanicSourceMod::FireGameEvent: %s \n", name);
#endif // EXT_DEBUG

		if (V_strcmp(name, "endslate") == 0)
		{
			OnRoundEnd();
			return;
		}
		else if (V_strcmp(name, "clientsound") == 0)
		{
			const char* sound = event->GetString("sound");

			if (V_strcmp(sound, "Round_Starting") == 0)
			{
				m_roundstarttimer.Start(9.0f);
			}
			return;
		}

		CBaseMod::FireGameEvent(event);
	}
}

void CZombiePanicSourceMod::Update()
{
	CBaseMod::Update();

	if (m_roundstarttimer.HasStarted() && m_roundstarttimer.IsElapsed())
	{
		m_roundstarttimer.Invalidate();
		OnRoundStart();
	}

	entprops->GameRules_GetPropBool("m_bRoundInProgressClient", m_roundactive);
}

CBaseBot* CZombiePanicSourceMod::AllocateBot(edict_t* edict)
{
	return new CZPSBot(edict);
}

void CZombiePanicSourceMod::OnMapStart()
{
	CBaseMod::OnMapStart();
}

void CZombiePanicSourceMod::OnRoundStart()
{
#ifdef EXT_DEBUG
	META_CONPRINT("CZombiePanicSourceMod::OnRoundStart() \n");
#endif // EXT_DEBUG

	CBaseMod::OnRoundStart();
	CNavMesh::NotifyRoundRestart();

	// ZPS gives weapons when the bots joins a team, force a refresh
	IInventory::RefreshInventory func{ 1.0f };
	extmanager->ForEachBot(func);
}

void CZombiePanicSourceMod::OnRoundEnd()
{
#ifdef EXT_DEBUG
	META_CONPRINT("CZombiePanicSourceMod::OnRoundEnd() \n");
#endif // EXT_DEBUG

	CBaseMod::OnRoundEnd();
}

bool CZombiePanicSourceMod::IsEntityDamageable(CBaseEntity* entity, const int maxhealth) const
{
	bool base = CBaseMod::IsEntityDamageable(entity, maxhealth);

	if (base)
	{
		// prop_* entities have these
		bool* unbreakable = entprops->GetPointerToEntData<bool>(entity, Prop_Data, "m_bUnbreakable");

		if (unbreakable != nullptr && *unbreakable == true)
		{
			return false;
		}
	}

	return base;
}

bool CZombiePanicSourceMod::IsEntityBreakable(CBaseEntity* entity) const
{
	const char* classname = gamehelpers->GetEntityClassname(entity);

	if (std::strcmp(classname, "prop_door_rotating") == 0)
	{
		return true;
	}
	if (std::strcmp(classname, "prop_barricade") == 0)
	{
		return true;
	}

	return CBaseMod::IsEntityBreakable(entity);
}

void CZombiePanicSourceMod::DetectGameMode()
{
	std::string mapname = GetCurrentMapName();

	if (mapname.find("zps_") != std::string::npos)
	{
		m_gamemode = zps::ZPSGamemodes::GAMEMODE_SURVIVAL;
	}
	else if (mapname.find("zpo_") != std::string::npos)
	{
		m_gamemode = zps::ZPSGamemodes::GAMEMODE_OBJECTIVE;
	}
	else if (mapname.find("zph_") != std::string::npos)
	{
		m_gamemode = zps::ZPSGamemodes::GAMEMODE_HARDCORE;
	}
	else
	{
		m_gamemode = zps::ZPSGamemodes::GAMEMODE_SURVIVAL;
	}
}
