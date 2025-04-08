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

CBaseMod::CBaseMod()
{
	m_parser_depth = 0;
	m_playerresourceentity.Term();
}

CBaseMod::~CBaseMod()
{
}

SourceMod::SMCResult CBaseMod::ReadSMC_NewSection(const SourceMod::SMCStates* states, const char* name)
{
	if (m_parser_depth == 0)
	{
		if (strncasecmp(name, "ModSettings", 11) == 0)
		{
			m_parser_depth++;
		}
		else
		{
			smutils->LogError(myself, "Invalid mod settings file (%s) at line %i col %i", name, states->line, states->col);
			return SourceMod::SMCResult_HaltFail;
		}
	}

	return SourceMod::SMCResult_Continue;
}

SourceMod::SMCResult CBaseMod::ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value)
{
	if (m_parser_depth == 1)
	{
		if (strncasecmp(key, "defend_rate", 11) == 0)
		{
			int v = atoi(value);
			v = std::clamp(v, 0, 100);
			m_modsettings->SetDefendRate(v);
		}
		else if (strncasecmp(key, "stuck_suicide_threshold", 23) == 0)
		{
			int v = atoi(value);
			v = std::clamp(v, 5, 60);
			m_modsettings->SetStuckSuicideThreshold(v);
		}
		else if (strncasecmp(key, "update_rate", 11) == 0)
		{
			float v = atof(value);
			v = std::clamp(v, 0.0f, 0.5f);
			m_modsettings->SetUpdateRate(v);
		}
		else if (strncasecmp(key, "vision_npc_update_rate", 22) == 0)
		{
			float v = atof(value);
			v = std::clamp(v, 0.0f, 2.0f);
			m_modsettings->SetVisionNPCUpdateRate(v);
		}
		else if (strncasecmp(key, "inventory_update_rate", 21) == 0)
		{
			float v = atof(value);
			v = std::clamp(v, 0.05f, 60.0f);
			m_modsettings->SetInventoryUpdateRate(v);
		}
		else if (strncasecmp(key, "vision_statistics_update", 24) == 0)
		{
			float v = atof(value);
			v = std::clamp(v, 0.05f, 2.0f);
			m_modsettings->SetVisionStatisticsUpdateRate(v);
		}
		else
		{
			smutils->LogError(myself, "[MOD SETTINGS] Unknown Key Value pair (\"%s\"    \"%s\") at line %i col %i", key, value, states->line, states->col);
		}
	}

	return SourceMod::SMCResult_Continue;
}

SourceMod::SMCResult CBaseMod::ReadSMC_LeavingSection(const SourceMod::SMCStates* states)
{
	if (--m_parser_depth < 0)
	{
		smutils->LogError(myself, "Invalid mod settings file at line %i col %i", states->line, states->col);
		return SourceMod::SMCResult_HaltFail;
	}

	return SourceMod::SMCResult_Continue;
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

void CBaseMod::OnMapStart()
{
	InternalFindPlayerResourceEntity();

	m_modsettings.reset(CreateModSettings());
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

	extmanager->ForEachBot([&manager](CBaseBot* bot) {
		bot->RefreshDifficulty(manager);
	});
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

void CBaseMod::ParseModSettings()
{
	std::unique_ptr<char[]> path = std::make_unique<char[]>(PLATFORM_MAX_PATH);
	const char* modfolder = smutils->GetGameFolderName();

	smutils->BuildPath(SourceMod::Path_SM, path.get(), PLATFORM_MAX_PATH, "configs/navbot/%s/settings.cfg", modfolder);

	if (!std::filesystem::exists(path.get()))
	{
		rootconsole->ConsolePrint("[NAVBOT] Mod settings file at \"%s\" doesn't exists. Using default values.", path.get());
		return;
	}

	rootconsole->ConsolePrint("[NAVBOT] Parsing mod settings file (\"%s\").", path.get());

	SourceMod::SMCStates states;
	textparsers->ParseFile_SMC(path.get(), this, &states);

}

CON_COMMAND(sm_navbot_reload_difficulty_profiles, "Reloads the bot difficulty profile config file.")
{
	extmanager->GetMod()->ReloadBotDifficultyProfile();
	rootconsole->ConsolePrint("Reloaded bot difficulty profile config file!");
}