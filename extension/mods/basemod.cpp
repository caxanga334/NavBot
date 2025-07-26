#include NAVBOT_PCH_FILE
#include <algorithm>
#include <filesystem>

#include <extension.h>
#include <manager.h>
#include <util/helpers.h>
#include <extplayer.h>
#include <bot/basebot.h>
#include <navmesh/nav_mesh.h>
#include <server_class.h>
#include "basemod.h"

#undef min
#undef max
#undef clamp

void CBaseMod::PropagateGameEventToBots::operator()(CBaseBot* bot)
{
	bot->OnGameEvent(this->event, this->moddata);
}

CBaseMod::CBaseMod()
{
	m_playerresourceentity.Term();

	for (std::size_t i = 0; i < m_teamsharedmemory.max_size(); i++)
	{
		m_teamsharedmemory[i] = nullptr;
	}
}

CBaseMod::~CBaseMod()
{
}

std::string CBaseMod::GetCurrentMapName() const
{
	return std::string(STRING(gpGlobals->mapname));
}

void CBaseMod::PostCreation()
{
	m_weaponinfomanager.reset(CreateWeaponInfoManager());
	
	if (!m_weaponinfomanager->LoadConfigFile())
	{
		Warning("Weapon info manager failed to load config file!\n");
	}

	m_profilemanager.reset(CreateBotDifficultyProfileManager());

	m_profilemanager->LoadProfiles();
}

void CBaseMod::Frame()
{
	for (auto& sbm : m_teamsharedmemory)
	{
		if (sbm)
		{
			sbm->Frame();
		}
	}
}

void CBaseMod::Update()
{
	for (auto& sbm : m_teamsharedmemory)
	{
		if (sbm)
		{
			sbm->Update();
		}
	}
}

void CBaseMod::OnMapStart()
{
	InternalFindPlayerResourceEntity();

	m_modsettings.reset(CreateModSettings());
	m_modsettings->ParseConfigFile();
}

CBaseBot* CBaseMod::AllocateBot(edict_t* edict)
{
	return new CBaseBot(edict);
}

CNavMesh* CBaseMod::NavMeshFactory()
{
	return new CNavMesh;
}

std::optional<int> CBaseMod::GetPlayerResourceEntity()
{
	if (gamehelpers->GetHandleEntity(m_playerresourceentity) != nullptr)
	{
		return m_playerresourceentity.GetEntryIndex();
	}

	return std::nullopt;
}

void CBaseMod::OnRoundStart()
{
	for (auto& sbm : m_teamsharedmemory)
	{
		if (sbm)
		{
			sbm->OnRoundRestart();
		}
	}
}

void CBaseMod::ReloadModSettingsFile()
{
	m_modsettings.reset(CreateModSettings());
	m_modsettings->ParseConfigFile();
}

void CBaseMod::ReloadWeaponInfoConfigFile()
{
	if (!m_weaponinfomanager->LoadConfigFile())
	{
		Warning("Weapon info config reloaded with errors!\n");
	}
	else
	{
		Msg("Weapon info config reloaded without errors.\n");
	}
}

void CBaseMod::ReloadBotDifficultyProfile()
{
	m_profilemanager.reset(CreateBotDifficultyProfileManager());
	m_profilemanager->LoadProfiles();

	const CDifficultyManager* manager = m_profilemanager.get();

	auto func = [&manager](CBaseBot* bot) {
		bot->RefreshDifficulty(manager);
	};

	extmanager->ForEachBot(func);
}

ISharedBotMemory* CBaseMod::GetSharedBotMemory(int teamindex)
{
	if (teamindex >= 0 && teamindex < static_cast<int>(m_teamsharedmemory.max_size()))
	{
		if (!m_teamsharedmemory[teamindex])
		{
			m_teamsharedmemory[teamindex].reset(CreateSharedMemory());
		}

		return m_teamsharedmemory[teamindex].get();
	}


	// Assert to make sure TEAM_UNASSIGNED is a sane value
	static_assert(TEAM_UNASSIGNED >= 0 && TEAM_UNASSIGNED < MAX_TEAMS, "Problematic TEAM_UNASSIGNED value!");

	if (!m_teamsharedmemory[TEAM_UNASSIGNED])
	{
		m_teamsharedmemory[TEAM_UNASSIGNED].reset(CreateSharedMemory());
	}

	// this function should never NULL, return the shared interface for the unassigned team if the given team index is invalid.
	return m_teamsharedmemory[TEAM_UNASSIGNED].get();
}

void CBaseMod::InternalFindPlayerResourceEntity()
{
	SourceMod::IGameConfig* gamedata = nullptr;
	m_playerresourceentity.Term();
	
	constexpr auto maxlength = 256U;
	char error[maxlength];

	if (!gameconfs->LoadGameConfigFile("sdktools.games", &gamedata, error, maxlength))
	{
		smutils->LogError(myself, "Failed to load SDK Tools gamedata file! error: \"%s\".", error);
		gameconfs->CloseGameConfigFile(gamedata);
		return;
	}

#if SOURCE_ENGINE >= SE_ORANGEBOX
	auto classname = gamedata->GetKeyValue("ResourceEntityClassname");
	if (classname != nullptr)
	{
		for (CBaseEntity* ent = static_cast<CBaseEntity*>(servertools->FirstEntity()); ent != nullptr; ent = static_cast<CBaseEntity*>(servertools->NextEntity(ent)))
		{
			if (strcmp(gamehelpers->GetEntityClassname(ent), classname) == 0)
			{
				m_playerresourceentity = reinterpret_cast<IHandleEntity*>(ent)->GetRefEHandle();
				break;
			}
		}
	}
	else
#endif // SOURCE_ENGINE >= SE_ORANGEBOX
	{
		int edictCount = gpGlobals->maxEntities;

		for (int i = 0; i < edictCount; i++)
		{
			edict_t* pEdict = gamehelpers->EdictOfIndex(i);
			if (!pEdict || pEdict->IsFree())
			{
				continue;
			}

			if (!pEdict->GetNetworkable())
			{
				continue;
			}

			IHandleEntity* pHandleEnt = pEdict->GetNetworkable()->GetEntityHandle();
			if (!pHandleEnt)
			{
				continue;
			}

			ServerClass* pClass = pEdict->GetNetworkable()->GetServerClass();

			if (UtilHelpers::FindNestedDataTable(pClass->m_pTable, "DT_PlayerResource"))
			{
				m_playerresourceentity = pHandleEnt->GetRefEHandle();
				break;
			}
		}
	}

#ifdef EXT_DEBUG
	if (m_playerresourceentity.IsValid())
	{
		smutils->LogMessage(myself, "Found Player Resource Entity #%i", m_playerresourceentity.GetEntryIndex());
	}
#endif

	gameconfs->CloseGameConfigFile(gamedata);
}

CON_COMMAND(sm_navbot_reload_difficulty_profiles, "Reloads the bot difficulty profile config file.")
{
	extmanager->GetMod()->ReloadBotDifficultyProfile();
	rootconsole->ConsolePrint("Reloaded bot difficulty profile config file!");
}

CON_COMMAND(sm_navbot_reload_mod_settings, "Reloads the mod settings config file.")
{
	extmanager->GetMod()->ReloadModSettingsFile();
}
