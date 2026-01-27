#include NAVBOT_PCH_FILE
#include <tier1/KeyValues.h>
#include <navmesh/nav_mesh.h>
#include <bot/insmic/insmicbot.h>
#include "insmic_modhelpers.h"
#include "insmicmod.h"

CInsMICMod* CInsMICMod::GetInsurgencyMod()
{
	return static_cast<CInsMICMod*>(extmanager->GetMod());
}

CInsMICMod::CInsMICMod()
{
	m_gametype = insmic::InsMICGameTypes_t::GAMETYPE_INVALID;

	ListenForGameEvent("round_reset");
}

void CInsMICMod::FireGameEvent(IGameEvent* event)
{
	if (event)
	{
		const char* name = event->GetName();

#ifdef EXT_DEBUG
		META_CONPRINTF("CInsMICMod::FireGameEvent \"%s\" \n", name);
#endif // EXT_DEBUG

		if (std::strcmp(name, "round_reset") == 0)
		{
			OnRoundEnd();
			OnRoundStart();
		}
	}
}

CBaseBot* CInsMICMod::AllocateBot(edict_t* edict)
{
	return new CInsMICBot(edict);
}

IModHelpers* CInsMICMod::AllocModHelpers() const
{
	return new CInsMICModHelpers;
}

void CInsMICMod::SelectTeamAndSquadForBot(int& teamid, int& encodedSquadData) const
{
	int t1c, t2c;
	insmiclib::GetTeamClientCount(t1c, t2c);

	if (t1c > t2c)
	{
		teamid = static_cast<int>(insmic::InsMICTeam::INSMICTEAM_INSURGENTS);
	}
	else
	{
		teamid = static_cast<int>(insmic::InsMICTeam::INSMICTEAM_USMC);
	}

	int slot = insmic::INVALID_SLOT;
	int squadid = static_cast<int>(insmic::InsMICSquads_t::INSMIC_INVALID_SQUAD);

	auto func = [&teamid, &slot, &squadid](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			bool enabled = false;
			entprops->GetEntPropBool(entity, Prop_Send, "m_bEnabled", enabled);
			int teamnum = static_cast<int>(insmic::InsMICTeam::INSMICTEAM_UNASSINGED);
			entprops->GetEntProp(entity, Prop_Send, "m_iParentTeam", teamnum);

			if (enabled && teamnum == teamid)
			{
				if (insmiclib::GetRandomFreeSquadSlot(entity, slot))
				{
					squadid = insmiclib::GetSquadID(entity);
					return false; // stop the loop
				}
			}
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("ins_squad", func);

	if (slot == insmic::INVALID_SLOT)
	{
		encodedSquadData = insmic::INVALID_SLOT;
		return;
	}

	teamid--;
	encodedSquadData = insmiclib::EncodeSquadData(squadid, slot);
}

void CInsMICMod::OnMapStart()
{
	CBaseMod::OnMapStart();

	DetectGameType();
	FindTeamManagerEntities();
}

void CInsMICMod::OnRoundStart()
{
	FindTeamManagerEntities();

	CNavMesh::NotifyRoundRestart();
}

void CInsMICMod::DetectGameType()
{
	using namespace std::literals::string_view_literals;
	constexpr auto CONFIG_FILE_EXT = "imc2"sv;
	char path[256];

	ke::SafeSprintf(path, sizeof(path), "maps/%s.%s", STRING(gpGlobals->mapname), CONFIG_FILE_EXT.data());
	m_gametype = insmic::InsMICGameTypes_t::GAMETYPE_TDM; // default to TDM

	if (filesystem->FileExists(path, "MOD"))
	{
		KeyValues* root = new KeyValues("IMCData");

		if (!root->LoadFromFile(filesystem, path, "MOD"))
		{
			root->deleteThis();
			smutils->LogError(myself, "Failed to load IMCConfig file \"%s\"!", path);
			return;
		}

		KeyValues* profiledata = root->FindKey("ProfileData");

		if (profiledata)
		{
			for (KeyValues* sub = profiledata->GetFirstSubKey(); sub != nullptr; sub = sub->GetNextKey())
			{
				const char* gamemode = sub->GetString("gametype", nullptr);

				if (gamemode)
				{
					auto gametype = insmiclib::StringToGameType(gamemode);
					m_gametype = gametype != insmic::InsMICGameTypes_t::GAMETYPE_INVALID ? gametype : insmic::InsMICGameTypes_t::GAMETYPE_TDM;
					root->deleteThis();
					return;
				}
			}
		}

		root->deleteThis();
	}
}

void CInsMICMod::FindTeamManagerEntities()
{
	auto func = [this](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			int teamid = 0;
			entprops->GetEntProp(entity, Prop_Send, "m_iTeamID", teamid);

			if (teamid == static_cast<int>(insmic::InsMICTeam::INSMICTEAM_USMC))
			{
				this->m_hUSMCTeamManager = entity;
			}
			else if (teamid == static_cast<int>(insmic::InsMICTeam::INSMICTEAM_INSURGENTS))
			{
				this->m_hInsurgentsTeamManager = entity;
			}
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("pteam_manager", func);
}

int CInsMICMod::GetPlayerSquadData(CBaseEntity* pPlayer) const
{
	SourceMod::IGamePlayer* gp = playerhelpers->GetGamePlayer(UtilHelpers::BaseEntityToEdict(pPlayer));

	if (!gp || !gp->GetPlayerInfo())
	{
		return 0;
	}

	int teamid = gp->GetPlayerInfo()->GetTeamIndex();
	CBaseEntity* teammanager = GetTeamManager(static_cast<insmic::InsMICTeam>(teamid));

	if (!teammanager)
	{
		return 0;
	}

	int data = 0;
	entprops->GetEntProp(teammanager, Prop_Send, "m_iSquadData", data, 4, gp->GetIndex());
	return data;
}

CBaseEntity* CInsMICMod::GetObjectiveToCapture(insmic::InsMICTeam teamid) const
{
	std::array<CBaseEntity*, 32U> objectives;
	std::size_t n = 0;

	auto func = [teamid, &objectives, &n](int index, edict_t* edict, CBaseEntity* entity) {
		
		bool hidden = false;
		entprops->GetEntPropBool(entity, Prop_Send, "m_bHidden", hidden);
		
		// skip hidden objectives
		if (hidden)
		{
			return true;
		}

		// skip objectives that are already owned by the given team
		if (teamid == insmiclib::GetObjectiveOwnerTeam(entity))
		{
			return true;
		}

		objectives[n] = entity;
		n++;

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("ins_objective", func);

	if (n == 0)
	{
		return nullptr;
	}

	return objectives[randomgen->GetRandomInt<std::size_t>(0U, n - 1U)];
}

CBaseEntity* CInsMICMod::GetObjectiveToDefend(insmic::InsMICTeam teamid) const
{
	std::array<CBaseEntity*, 32U> objectives;
	std::size_t n = 0;

	auto func = [teamid, &objectives, &n](int index, edict_t* edict, CBaseEntity* entity) {

		bool hidden = false;
		entprops->GetEntPropBool(entity, Prop_Send, "m_bHidden", hidden);

		// skip hidden objectives
		if (hidden)
		{
			return true;
		}

		// skip objectives that are not owned by the given team
		if (teamid != insmiclib::GetObjectiveOwnerTeam(entity))
		{
			return true;
		}

		objectives[n] = entity;
		n++;

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("ins_objective", func);

	if (n == 0)
	{
		return nullptr;
	}

	return objectives[randomgen->GetRandomInt<std::size_t>(0U, n - 1U)];
}

static void InsMICCommand_DebugSquadIDs(const CConCommandArgs& args)
{
	auto func = [](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			bool enabled = false;
			entprops->GetEntPropBool(entity, Prop_Send, "m_bEnabled", enabled);
			int teamid = static_cast<int>(insmic::InsMICTeam::INSMICTEAM_UNASSINGED);
			entprops->GetEntProp(entity, Prop_Send, "m_iParentTeam", teamid);
			char name[256];
			size_t len = 0;
			std::memset(name, 0, sizeof(name));
			entprops->GetEntPropString(entity, Prop_Send, "m_szName", name, sizeof(name), len);
			int squadid = insmiclib::GetSquadID(entity);

			META_CONPRINTF("Squad %s\n  Enabled: %s\n  ID: %i\n  Team: %i\n  Name: %s\n", 
				UtilHelpers::textformat::FormatEntity(entity), UtilHelpers::textformat::FormatBool(enabled), squadid, teamid, name);
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("ins_squad", func);
}

static void InsMICCommand_DebugWeaponAmmo(const CConCommandArgs& args)
{
	CBaseExtPlayer* player = extmanager->GetListenServerHost();
	CBaseEntity* weapon = nullptr;

	entprops->GetEntPropEnt(player->GetEntity(), Prop_Send, "m_hActiveWeapon", nullptr, &weapon);

	if (!weapon)
	{
		META_CONPRINT("NULL active weapon! \n");
		return;
	}

	if (!entprops->HasEntProp(weapon, Prop_Send, "m_iClip"))
	{
		META_CONPRINTF("Error: Entity %s does not derives from CWeaponBallisticBase! \n", UtilHelpers::textformat::FormatEntity(weapon));
		return;
	}

	int ammo = insmiclib::GetBallisticWeaponReserveAmmoLeft(weapon);
	META_CONPRINTF("Reserve ammo for %s is %i. \n", UtilHelpers::textformat::FormatEntity(weapon), ammo);
}

void CInsMICMod::RegisterModCommands()
{
	CServerCommandManager& manager = extmanager->GetServerCommandManager();

	manager.RegisterConCommand("sm_navbot_insmic_debug_squad_ids",
		"Prints the squad manager entity's Squad IDs to the console.", FCVAR_GAMEDLL | FCVAR_CHEAT, InsMICCommand_DebugSquadIDs);
	manager.RegisterConCommand("sm_navbot_insmic_debug_weapon_ammo",
		"Prints your current weapon reserve ammo.", FCVAR_GAMEDLL | FCVAR_CHEAT, InsMICCommand_DebugWeaponAmmo, CServerCommandManager::COMMAND_ONLY_ON_LISTEN_SERVERS);
}
