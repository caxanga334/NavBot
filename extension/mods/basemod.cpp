#include NAVBOT_PCH_FILE
#include <algorithm>
#include <filesystem>

#include <extension.h>
#include <manager.h>
#include <util/helpers.h>
#include <util/prediction.h>
#include <util/sdkcalls.h>
#include <extplayer.h>
#include <bot/basebot.h>
#include <navmesh/nav_mesh.h>
#include <server_class.h>
#include "basemod.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

#undef min
#undef max
#undef clamp

void CBaseMod::PropagateGameEventToBots::operator()(CBaseBot* bot)
{
	bot->OnGameEvent(this->event, this->moddata);
}

CBaseMod::CBaseMod() :
	m_modName("CBaseMod")
{
	m_playerresourceentity.Term();
	m_modID = Mods::ModType::MOD_BASE;
	m_modFolder.assign(smutils->GetGameFolderName());
	ISensor::s_npcentities.reserve(MAX_EDICTS);

	for (std::size_t i = 0; i < m_teamsharedmemory.max_size(); i++)
	{
		m_teamsharedmemory[i] = nullptr;
	}
}

CBaseMod::~CBaseMod()
{
}

void CBaseMod::InvokePostInit()
{
	ReadNPCListFromGamedata();
	OnPostInit(); // propagate to derived classes
}

void CBaseMod::RegisterCommands()
{
	// register mod commands (virtual)
	RegisterModCommands();
}

std::string CBaseMod::GetCurrentMapName(Mods::MapNameType type) const
{
	return std::string(STRING(gpGlobals->mapname));
}

void CBaseMod::PostCreation()
{
	m_weaponinfomanager.reset(CreateWeaponInfoManager());
	m_profilemanager.reset(CreateBotDifficultyProfileManager());
	m_profilemanager->LoadProfiles();
}

void CBaseMod::Frame()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CBaseMod::Frame", "NavBot");
#endif // EXT_VPROF_ENABLED

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
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CBaseMod::Update", "NavBot");
#endif // EXT_VPROF_ENABLED

	for (auto& sbm : m_teamsharedmemory)
	{
		if (sbm)
		{
			sbm->Update();
		}
	}

	UpdateNPCs();
}

void CBaseMod::OnMapStart()
{
	// Weapon info is now parsed OnMapStart because it needs to be parsed after SM plugins are loaded.
	// As a bonus, the file is now reloaded on map change.
	if (!m_weaponinfomanager->LoadConfigFile())
	{
		META_CONPRINT("Weapon info manager failed to load config file!\n");
	}

	InternalFindPlayerResourceEntity();

	m_modsettings.reset(CreateModSettings());
	m_modsettings->ParseConfigFile();

	for (auto& sbm : m_teamsharedmemory)
	{
		if (sbm)
		{
			sbm->Reset();
		}
	}
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
	ISensor::s_npcentities.clear();

	for (auto& sbm : m_teamsharedmemory)
	{
		if (sbm)
		{
			sbm->OnRoundRestart();
		}
	}

#ifndef NO_SOURCEPAWN_API
	extmanager->SMAPI_OnRoundRestart();
#endif // !NO_SOURCEPAWN_API
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
		META_CONPRINT("[Warning]: Weapon info config reloaded with errors!\n");
	}
	else
	{
		META_CONPRINT("Weapon info config reloaded without errors.\n");
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

SourceMod::SMCError CBaseMod::ParseCustomBotDifficultyProfileFile(const char* file, const bool replace, const bool refresh)
{
	SourceMod::SMCError result = m_profilemanager->ParseCustomProfileConfig(file, replace);

	if (result == SourceMod::SMCError::SMCError_Okay && refresh)
	{
		const CDifficultyManager* manager = m_profilemanager.get();

		auto func = [&manager](CBaseBot* bot) {
			bot->RefreshDifficulty(manager);
		};

		extmanager->ForEachBot(func);
	}

	return result;
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

bool CBaseMod::IsLineOfFireClear(const Vector& from, const Vector& to, CBaseEntity* passEnt) const
{
	CTraceFilterWorldAndPropsOnly filter;
	trace_t result;
	trace::line(from, to, MASK_SHOT, &filter, result);
	return !result.DidHit();
}

bool CBaseMod::IsEntityDamageable(CBaseEntity* entity, const int maxhealth) const
{
	int takedamage = 0;

	if (entprops->GetEntProp(entity, Prop_Data, "m_takedamage", takedamage))
	{
		switch (takedamage)
		{
		case DAMAGE_NO:
			[[fallthrough]];
		case DAMAGE_EVENTS_ONLY:
		{
			return false; // entity doesn't take damage
		}
		default:
			break;
		}
	}

	int health = 0;

	if (entprops->GetEntProp(entity, Prop_Data, "m_iHealth", health))
	{
		if (health > maxhealth)
		{
			return false; // too much health
		}
	}

	return true;
}

bool CBaseMod::IsEntityDamageableBy(CBaseEntity* entity, CBaseEntity* attacker) const
{
	CBaseEntity* damageFilter = nullptr;

	if (entprops->GetEntPropEnt(entity, Prop_Data, "m_hDamageFilter", nullptr, &damageFilter))
	{
		if (damageFilter)
		{
			if (sdkcalls->IsPassesFilterImplAvailable())
			{
				if (!modhelpers->PassesFilterImpl(damageFilter, nullptr, attacker))
				{
					// The attacker does not passes the damage filter.
					return false;
				}
			}
			else
			{
				// The entity has a damage filter and the CBaseFilter::PassesFilterImpl SDKCall isn't available.
				// Consider as not damageable.
				return false;
			}
		}
	}

	return true;
}

bool CBaseMod::IsEntityBreakable(CBaseEntity* entity) const
{
	auto classname = gamehelpers->GetEntityClassname(entity);

	if (strncmp(classname, "func_breakable", 14) == 0)
	{
		return true;
	}

	if (strncmp(classname, "func_breakable_surf", 19) == 0)
	{
		return true;
	}

	if (strncmp(classname, "func_physbox", 12) == 0)
	{
		return true;
	}

	if (UtilHelpers::StringMatchesPattern(classname, "prop_phys*", 0))
	{
		return true;
	}

	return false;
}

bool CBaseMod::IsBreakableBroken(CBaseEntity* entity) const
{
	// breakable surfaces entities aren't deleted when broken.
	if (UtilHelpers::FClassnameIs(entity, "func_breakable_surf"))
	{
		bool* bBroken = entprops->GetPointerToEntData<bool>(entity, Prop_Send, "m_bIsBroken");

		if (bBroken != nullptr)
		{
			return *bBroken;
		}
	}

	return false;
}

IModHelpers* CBaseMod::AllocModHelpers() const
{
	return new IModHelpers;
}

bool CBaseMod::BuildPathToModFile(const std::string_view& basepath, const std::string_view& filepath, const std::string_view& userfilepath, std::string& path) const
{
	char tmp[PLATFORM_MAX_PATH];

	// check main folder

	if (!userfilepath.empty())
	{
		// check user custom file
		smutils->BuildPath(SourceMod::PathType::Path_SM, tmp, sizeof(tmp), "%s/%s/%s", basepath.data(), m_modFolder.c_str(), userfilepath.data());

		if (std::filesystem::exists(tmp))
		{
			path.assign(tmp);
			return true;
		}
	}

	smutils->BuildPath(SourceMod::PathType::Path_SM, tmp, sizeof(tmp), "%s/%s/%s", basepath.data(), m_modFolder.c_str(), filepath.data());

	if (std::filesystem::exists(tmp))
	{
		path.assign(tmp);
		return true;
	}

	// check fallback folder

	if (!userfilepath.empty())
	{
		// check user custom file
		smutils->BuildPath(SourceMod::PathType::Path_SM, tmp, sizeof(tmp), "%s/%s/%s", basepath.data(), m_modFallbackFolder.c_str(), userfilepath.data());

		if (std::filesystem::exists(tmp))
		{
			path.assign(tmp);
			return true;
		}
	}

	smutils->BuildPath(SourceMod::PathType::Path_SM, tmp, sizeof(tmp), "%s/%s/%s", basepath.data(), m_modFallbackFolder.c_str(), filepath.data());

	if (std::filesystem::exists(tmp))
	{
		path.assign(tmp);
		return true;
	}

	return false;
}

bool CBaseMod::IsInProjectilesPath(CBaseBot* bot, CBaseEntity* projectile, Vector& hitPos) const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CBaseMod::IsInProjectilesPath", "NavBot");
#endif // EXT_VPROF_ENABLED

	const Vector& projOrigin = UtilHelpers::getEntityOrigin(projectile);
	const float dist = (bot->WorldSpaceCenter() - projOrigin).Length();

	Vector velocity{ 0.0f, 0.0f, 0.0f };
	entityprops::GetEntityAbsVelocity(projectile, velocity);
	const float blastradius = GetModSettings()->GetDefaultBlastRadius();

	// projectile is either not moving or the velocity isn't stored in CBaseEntity::m_vecAbsVelocity
	if (velocity.IsZero(1.0f))
	{
		if (bot->GetRangeTo(projOrigin) <= blastradius)
		{
			return true;
		}

		return false;
	}

	const float gravity = UtilHelpers::GetEntityGravity(UtilHelpers::IndexOfEntity(projectile));

	// no gravity, assume it moves straigh in a constant speed
	if (gravity <= 0.01f)
	{
		hitPos = pred::PredictStraightProjectileHitPosition(projectile, MASK_SOLID);
	}
	else
	{
		// Stop prediction when the projectile hits ground.
		// The physics sim isn't that good when things are on ground and this should be good enough for explode on contact projectiles.
		hitPos = pred::PredictBallisticProjectileHitPosition(projectile, MASK_SOLID, 1.0f, true);
	}

	if (bot->GetRangeTo(hitPos) <= blastradius)
	{
		return true;
	}

	return false;
}

void CBaseMod::AddNPCClassnameToList(const char* classname)
{
	for (auto& name : m_NPCClassnameList)
	{
		if (ke::StrCaseCmp(name.c_str(), classname) == 0)
		{
			return;
		}
	}

	m_NPCClassnameList.emplace_back(classname);
}

void CBaseMod::RemoveNPCClassnameFromList(const char* classname)
{
	for (auto it = m_NPCClassnameList.begin(); it != m_NPCClassnameList.end(); it++)
	{
		if (ke::StrCaseCmp(it->c_str(), classname) == 0)
		{
			m_NPCClassnameList.erase(it);
			return;
		}
	}
}

#ifdef EXT_DEBUG

void CBaseMod::Debug_PrintNPCClassnameList()
{
	META_CONPRINT("NPC Scan Classname List: \n");

	for (auto& name : m_NPCClassnameList)
	{
		META_CONPRINTF("- %s \n", name.c_str());
	}
}

#endif // EXT_DEBUG

void CBaseMod::InternalFindPlayerResourceEntity()
{
	SourceMod::IGameConfig* gamedata = nullptr;
	m_playerresourceentity.Term();
	
	constexpr std::size_t maxlength = 256U;
	char error[maxlength];

	if (!gameconfs->LoadGameConfigFile("sdktools.games", &gamedata, error, maxlength))
	{
		smutils->LogError(myself, "Failed to load SDK Tools gamedata file! error: \"%s\".", error);
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

void CBaseMod::UpdateNPCs()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CBaseMod::UpdateNPCs", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (m_sensorNPCUpdateTimer.IsElapsed())
	{
		m_sensorNPCUpdateTimer.Start(GetModSettings()->GetVisionNPCUpdateRate());
		ISensor::s_npcentities.clear();
		NPCCollector functor;

		for (const std::string& classname : m_NPCClassnameList)
		{
			UtilHelpers::ForEachEntityOfClassname(classname.c_str(), functor);
		}
	}
}

void CBaseMod::ReadNPCListFromGamedata()
{
	SourceMod::IGameConfig* cfg = extension->GetExtensionGameData();

	const char* value = cfg->GetKeyValue("ISensor_NPCList");

	if (value && std::strlen(value) > 3)
	{
		std::string strvalue(value);
		std::stringstream stream(strvalue);
		std::string token;

		while (std::getline(stream, token, ','))
		{
			m_NPCClassnameList.emplace_back(token);
		}
	}

#ifdef EXT_DEBUG
	META_CONPRINTF("[CBaseMod]: NPC list contains %zu NPCs classnames. \n", m_NPCClassnameList.size());

	for (const std::string& classname : m_NPCClassnameList)
	{
		META_CONPRINTF("- %s\n", classname.c_str());
	}
#endif // EXT_DEBUG
}

bool CBaseMod::NPCCollector::operator()(int index, edict_t* edict, CBaseEntity* entity) const
{
	if (entity)
	{
		ISensor::s_npcentities.emplace_back(entity);
	}

	return true;
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

#ifdef EXT_DEBUG
CON_COMMAND(sm_navbot_mod_debug_breakable, "Reports an entity breakable state.")
{
	DECLARE_COMMAND_ARGS;

	CBaseExtPlayer* host = extmanager->GetListenServerHost();

	if (!host) { return; }

	if (args.ArgC() < 2)
	{
		META_CONPRINT("[SM] Usage: sm_navbot_mod_debug_breakable <entindex>\n");
		return;
	}

	int index = atoi(args[1]);
	CBaseEntity* pEnt = gamehelpers->ReferenceToEntity(index);

	if (!pEnt)
	{
		META_CONPRINT("NULL ent!\n");
		return;
	}

	CBaseMod* mod = extmanager->GetMod();

	bool damageable = mod->IsEntityDamageable(pEnt, mod->GetModSettings()->GetBreakableMaxHealth());
	bool breakable = mod->IsEntityBreakable(pEnt);
	bool damageablebyyou = mod->IsEntityDamageableBy(pEnt, host->GetEntity());
	int health = UtilHelpers::GetEntityHealth(index);

	META_CONPRINTF("Damageable %i Damageable (by you) %i Breakable %i Health %i\n", 
		static_cast<int>(damageable), static_cast<int>(damageablebyyou), static_cast<int>(breakable), health);
}

CON_COMMAND(sm_navbot_mod_debug_npc_list, "Dumps the NPC classname list to console.")
{
	extmanager->GetMod()->Debug_PrintNPCClassnameList();
}

#endif // EXT_DEBUG
