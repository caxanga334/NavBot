#include <extension.h>
#include <manager.h>
#include <extplayer.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <bot/basebot.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/nav/tfnavmesh.h>
#include <entities/tf2/tf_entities.h>
#include "tf2lib.h"
#include "teamfortress2mod.h"

static ConVar sm_navbot_tf_force_class("sm_navbot_tf_force_class", "none", FCVAR_GAMEDLL, "Forces all NavBots to use the specified class.");
static ConVar sm_navbot_tf_mod_debug("sm_navbot_tf_mod_debug", "0", FCVAR_GAMEDLL, "TF2 mod debugging.");

static const char* s_tf2gamemodenames[] = {
	"NONE/DEFAULT",
	"CAPTURE THE FLAG",
	"CONTROL POINT",
	"ATTACK/DEFENCE",
	"KING OF THE HILL",
	"MANN VS MACHINE",
	"PAYLOAD",
	"PAYLOAD RACE",
	"PLAYER DESTRUCTION",
	"SPECIAL DELIVERY",
	"TERRITORIAL CONTROL",
	"ARENA"
};

static_assert((sizeof(s_tf2gamemodenames) / sizeof(char*)) == static_cast<int>(TeamFortress2::GameModeType::GM_MAX_GAMEMODE_TYPES), 
	"You added or removed a TF2 game mode and forgot to update the game mode name array!");

CTeamFortress2Mod::CTeamFortress2Mod() : CBaseMod()
{
	m_gamemode = TeamFortress2::GameModeType::GM_NONE;

	m_weaponidmap.reserve(72);
	m_weaponidmap.emplace("tf_weapon_scattergun", TeamFortress2::TFWeaponID::TF_WEAPON_SCATTERGUN);
	m_weaponidmap.emplace("tf_weapon_handgun_scout_primary", TeamFortress2::TFWeaponID::TF_WEAPON_HANDGUN_SCOUT_PRIMARY);
	m_weaponidmap.emplace("tf_weapon_soda_popper", TeamFortress2::TFWeaponID::TF_WEAPON_SODA_POPPER);
	m_weaponidmap.emplace("tf_weapon_pep_brawler_blaster", TeamFortress2::TFWeaponID::TF_WEAPON_PEP_BRAWLER_BLASTER);
	m_weaponidmap.emplace("tf_weapon_pistol", TeamFortress2::TFWeaponID::TF_WEAPON_PISTOL);
	m_weaponidmap.emplace("tf_weapon_lunchbox_drink", TeamFortress2::TFWeaponID::TF_WEAPON_LUNCHBOX);
	m_weaponidmap.emplace("tf_weapon_jar_milk", TeamFortress2::TFWeaponID::TF_WEAPON_JAR_MILK);
	m_weaponidmap.emplace("tf_weapon_handgun_scout_secondary", TeamFortress2::TFWeaponID::TF_WEAPON_HANDGUN_SCOUT_SEC);
	m_weaponidmap.emplace("tf_weapon_cleaver", TeamFortress2::TFWeaponID::TF_WEAPON_CLEAVER);
	m_weaponidmap.emplace("tf_weapon_bat", TeamFortress2::TFWeaponID::TF_WEAPON_BAT);
	m_weaponidmap.emplace("tf_weapon_bat_wood", TeamFortress2::TFWeaponID::TF_WEAPON_BAT_WOOD);
	m_weaponidmap.emplace("tf_weapon_bat_fish", TeamFortress2::TFWeaponID::TF_WEAPON_BAT_FISH);
	m_weaponidmap.emplace("tf_weapon_bat_giftwrap", TeamFortress2::TFWeaponID::TF_WEAPON_BAT_GIFTWRAP);
	m_weaponidmap.emplace("tf_weapon_rocketlauncher", TeamFortress2::TFWeaponID::TF_WEAPON_ROCKETLAUNCHER);
	m_weaponidmap.emplace("tf_weapon_rocketlauncher_directhit", TeamFortress2::TFWeaponID::TF_WEAPON_DIRECTHIT);
	m_weaponidmap.emplace("tf_weapon_particle_cannon", TeamFortress2::TFWeaponID::TF_WEAPON_PARTICLE_CANNON);
	m_weaponidmap.emplace("tf_weapon_rocketlauncher_airstrike", TeamFortress2::TFWeaponID::TF_WEAPON_ROCKETLAUNCHER);
	m_weaponidmap.emplace("tf_weapon_shotgun_soldier", TeamFortress2::TFWeaponID::TF_WEAPON_SHOTGUN_SOLDIER);
	m_weaponidmap.emplace("tf_weapon_buff_item", TeamFortress2::TFWeaponID::TF_WEAPON_BUFF_ITEM);
	m_weaponidmap.emplace("tf_weapon_raygun", TeamFortress2::TFWeaponID::TF_WEAPON_RAYGUN);
	m_weaponidmap.emplace("tf_weapon_parachute", TeamFortress2::TFWeaponID::TF_WEAPON_PARACHUTE);
	m_weaponidmap.emplace("tf_weapon_shovel", TeamFortress2::TFWeaponID::TF_WEAPON_SHOVEL);
	m_weaponidmap.emplace("tf_weapon_katana", TeamFortress2::TFWeaponID::TF_WEAPON_SWORD);
	m_weaponidmap.emplace("tf_weapon_flamethrower", TeamFortress2::TFWeaponID::TF_WEAPON_FLAMETHROWER);
	m_weaponidmap.emplace("tf_weapon_rocketlauncher_fireball", TeamFortress2::TFWeaponID::TF_WEAPON_FLAMETHROWER_ROCKET);
	m_weaponidmap.emplace("tf_weapon_shotgun_pyro", TeamFortress2::TFWeaponID::TF_WEAPON_SHOTGUN_PYRO);
	m_weaponidmap.emplace("tf_weapon_flaregun", TeamFortress2::TFWeaponID::TF_WEAPON_FLAREGUN);
	m_weaponidmap.emplace("tf_weapon_flaregun_revenge", TeamFortress2::TFWeaponID::TF_WEAPON_FLAREGUN);
	m_weaponidmap.emplace("tf_weapon_rocketpack", TeamFortress2::TFWeaponID::TF_WEAPON_ROCKETPACK);
	m_weaponidmap.emplace("tf_weapon_jar_gas", TeamFortress2::TFWeaponID::TF_WEAPON_JAR_GAS);
	m_weaponidmap.emplace("tf_weapon_fireaxe", TeamFortress2::TFWeaponID::TF_WEAPON_FIREAXE);
	m_weaponidmap.emplace("tf_weapon_breakable_sign", TeamFortress2::TFWeaponID::TF_WEAPON_BREAKABLE_SIGN);
	m_weaponidmap.emplace("tf_weapon_slap", TeamFortress2::TFWeaponID::TF_WEAPON_SLAP);
	m_weaponidmap.emplace("tf_weapon_grenadelauncher", TeamFortress2::TFWeaponID::TF_WEAPON_GRENADELAUNCHER);
	m_weaponidmap.emplace("tf_weapon_cannon", TeamFortress2::TFWeaponID::TF_WEAPON_CANNON);
	m_weaponidmap.emplace("tf_weapon_pipebomblauncher", TeamFortress2::TFWeaponID::TF_WEAPON_PIPEBOMBLAUNCHER);
	m_weaponidmap.emplace("tf_weapon_bottle", TeamFortress2::TFWeaponID::TF_WEAPON_BOTTLE);
	m_weaponidmap.emplace("tf_weapon_sword", TeamFortress2::TFWeaponID::TF_WEAPON_SWORD);
	m_weaponidmap.emplace("tf_weapon_stickbomb", TeamFortress2::TFWeaponID::TF_WEAPON_STICKBOMB);
	m_weaponidmap.emplace("tf_weapon_minigun", TeamFortress2::TFWeaponID::TF_WEAPON_MINIGUN);
	m_weaponidmap.emplace("tf_weapon_shotgun_hwg", TeamFortress2::TFWeaponID::TF_WEAPON_SHOTGUN_HWG);
	m_weaponidmap.emplace("tf_weapon_lunchbox", TeamFortress2::TFWeaponID::TF_WEAPON_LUNCHBOX);
	m_weaponidmap.emplace("tf_weapon_fists", TeamFortress2::TFWeaponID::TF_WEAPON_FISTS);
	m_weaponidmap.emplace("tf_weapon_shotgun_primary", TeamFortress2::TFWeaponID::TF_WEAPON_SHOTGUN_PRIMARY);
	m_weaponidmap.emplace("tf_weapon_sentry_revenge", TeamFortress2::TFWeaponID::TF_WEAPON_SENTRY_REVENGE);
	m_weaponidmap.emplace("tf_weapon_drg_pomson", TeamFortress2::TFWeaponID::TF_WEAPON_DRG_POMSON);
	m_weaponidmap.emplace("tf_weapon_shotgun_building_rescue", TeamFortress2::TFWeaponID::TF_WEAPON_SHOTGUN_BUILDING_RESCUE);
	m_weaponidmap.emplace("tf_weapon_laser_pointer", TeamFortress2::TFWeaponID::TF_WEAPON_LASER_POINTER);
	m_weaponidmap.emplace("tf_weapon_mechanical_arm", TeamFortress2::TFWeaponID::TF_WEAPON_MECHANICAL_ARM);
	m_weaponidmap.emplace("tf_weapon_wrench", TeamFortress2::TFWeaponID::TF_WEAPON_WRENCH);
	m_weaponidmap.emplace("tf_weapon_robot_arm", TeamFortress2::TFWeaponID::TF_WEAPON_WRENCH);
	m_weaponidmap.emplace("tf_weapon_pda_engineer_build", TeamFortress2::TFWeaponID::TF_WEAPON_PDA_ENGINEER_BUILD);
	m_weaponidmap.emplace("tf_weapon_pda_engineer_destroy", TeamFortress2::TFWeaponID::TF_WEAPON_PDA_ENGINEER_DESTROY);
	m_weaponidmap.emplace("tf_weapon_builder", TeamFortress2::TFWeaponID::TF_WEAPON_BUILDER);
	m_weaponidmap.emplace("tf_weapon_syringegun_medic", TeamFortress2::TFWeaponID::TF_WEAPON_SYRINGEGUN_MEDIC);
	m_weaponidmap.emplace("tf_weapon_crossbow", TeamFortress2::TFWeaponID::TF_WEAPON_CROSSBOW);
	m_weaponidmap.emplace("tf_weapon_medigun", TeamFortress2::TFWeaponID::TF_WEAPON_MEDIGUN);
	m_weaponidmap.emplace("tf_weapon_bonesaw", TeamFortress2::TFWeaponID::TF_WEAPON_BONESAW);
	m_weaponidmap.emplace("tf_weapon_sniperrifle", TeamFortress2::TFWeaponID::TF_WEAPON_SNIPERRIFLE);
	m_weaponidmap.emplace("tf_weapon_compound_bow", TeamFortress2::TFWeaponID::TF_WEAPON_COMPOUND_BOW);
	m_weaponidmap.emplace("tf_weapon_sniperrifle_decap", TeamFortress2::TFWeaponID::TF_WEAPON_SNIPERRIFLE_DECAP);
	m_weaponidmap.emplace("tf_weapon_sniperrifle_classic", TeamFortress2::TFWeaponID::TF_WEAPON_SNIPERRIFLE_CLASSIC);
	m_weaponidmap.emplace("tf_weapon_smg", TeamFortress2::TFWeaponID::TF_WEAPON_SMG);
	m_weaponidmap.emplace("tf_weapon_jar", TeamFortress2::TFWeaponID::TF_WEAPON_JAR);
	m_weaponidmap.emplace("tf_weapon_charged_smg", TeamFortress2::TFWeaponID::TF_WEAPON_CHARGED_SMG);
	m_weaponidmap.emplace("tf_weapon_club", TeamFortress2::TFWeaponID::TF_WEAPON_CLUB);
	m_weaponidmap.emplace("tf_weapon_revolver", TeamFortress2::TFWeaponID::TF_WEAPON_REVOLVER);
	m_weaponidmap.emplace("tf_weapon_sapper", TeamFortress2::TFWeaponID::TF_WEAPON_BUILDER);
	m_weaponidmap.emplace("tf_weapon_knife", TeamFortress2::TFWeaponID::TF_WEAPON_KNIFE);
	m_weaponidmap.emplace("tf_weapon_pda_spy", TeamFortress2::TFWeaponID::TF_WEAPON_PDA_SPY);
	m_weaponidmap.emplace("tf_weapon_invis", TeamFortress2::TFWeaponID::TF_WEAPON_INVIS);

	m_classselector.LoadClassSelectionData();

	ListenForGameEvent("teamplay_round_start");
	ListenForGameEvent("controlpoint_initialized");
	ListenForGameEvent("mvm_begin_wave");
	ListenForGameEvent("mvm_wave_complete");
}

CTeamFortress2Mod::~CTeamFortress2Mod()
{
}

CTeamFortress2Mod* CTeamFortress2Mod::GetTF2Mod()
{
	return static_cast<CTeamFortress2Mod*>(extmanager->GetMod());
}

void CTeamFortress2Mod::FireGameEvent(IGameEvent* event)
{
	if (event != nullptr)
	{
		const char* name = event->GetName();

		if (strncasecmp(name, "teamplay_round_start", 20) == 0)
		{
			OnRoundStart();
			return;
		}

		if (strncasecmp(name, "mvm_begin_wave", 14) == 0)
		{
			OnRoundStart();
			return;
		}

		if (strncasecmp(name, "mvm_wave_complete", 17) == 0)
		{
			OnRoundStart();
			return;
		}

		if (strncasecmp(name, "controlpoint_initialized", 24) == 0)
		{
			FindControlPoints();
			return;
		}
	}
}

void CTeamFortress2Mod::OnMapStart()
{
	CBaseMod::OnMapStart();

	DetectCurrentGameMode();
	auto map = STRING(gpGlobals->mapname);
	auto mode = GetCurrentGameModeName();
	smutils->LogMessage(myself, "Detected game mode \"%s\" for map \"%s\".", mode, map);
	m_upgrademanager.ParseUpgradeFile();
	m_upgrademanager.ParseBotUpgradeInfoFile();
}

void CTeamFortress2Mod::OnMapEnd()
{
	CBaseMod::OnMapEnd();

	m_upgrademanager.Reset();
}

CBaseBot* CTeamFortress2Mod::AllocateBot(edict_t* edict)
{
	return new CTF2Bot(edict);
}

CNavMesh* CTeamFortress2Mod::NavMeshFactory()
{
	return new CTFNavMesh;
}

int CTeamFortress2Mod::GetWeaponEconIndex(edict_t* weapon) const
{
	return tf2lib::GetWeaponItemDefinitionIndex(weapon);
}

int CTeamFortress2Mod::GetWeaponID(edict_t* weapon) const
{
	auto classname = gamehelpers->GetEntityClassname(weapon);

	if (classname == nullptr || classname[0] == 0)
		return CBaseMod::NO_WEAPON_ID;

	std::string szname(classname);
	return static_cast<int>(GetTFWeaponID(szname));
}

bool CTeamFortress2Mod::BotQuotaIsClientIgnored(int client, edict_t* entity, SourceMod::IGamePlayer* player) const
{
	if (m_gamemode == TeamFortress2::GameModeType::GM_MVM)
	{
		if (player->IsFakeClient() && tf2lib::GetEntityTFTeam(client) != TeamFortress2::TFTeam::TFTeam_Red)
		{
			return true; // ignore spectator and BLU team bots on MvM
		}
	}

	return false;
}

const char* CTeamFortress2Mod::GetCurrentGameModeName() const
{
	return s_tf2gamemodenames[static_cast<int>(m_gamemode)];
}

void CTeamFortress2Mod::DetectCurrentGameMode()
{
	// When adding detection for community game mode, detect them first

	if (DetectMapViaGameRules())
	{
		return;
	}

	if (DetectMapViaName())
	{
		rootconsole->ConsolePrint("[NavBot] Game mode detected via map prefix.");
		return;
	}

	if (DetectKoth())
	{
		return;
	}

	if (DetectPlayerDestruction())
	{
		return;
	}

	m_gamemode = TeamFortress2::GameModeType::GM_NONE; // detection failed
}

bool CTeamFortress2Mod::DetectMapViaName()
{
	/*
	* Detects the map type via the map prefix
	*/

	std::string map(STRING(gpGlobals->mapname));

	if (map.find("ctf_") != std::string::npos)
	{
		m_gamemode = TeamFortress2::GameModeType::GM_CTF;
		return true;
	}

	if (map.find("cp_") != std::string::npos)
	{
		m_gamemode = TeamFortress2::GameModeType::GM_CP;
		return true;
	}

	if (map.find("tc_") != std::string::npos)
	{
		m_gamemode = TeamFortress2::GameModeType::GM_TC;
		return true;
	}

	if (map.find("mvm_") != std::string::npos)
	{
		m_gamemode = TeamFortress2::GameModeType::GM_MVM;
		return true;
	}

	if (map.find("pl_") != std::string::npos)
	{
		m_gamemode = TeamFortress2::GameModeType::GM_PL;
		return true;
	}

	if (map.find("plr_") != std::string::npos)
	{
		m_gamemode = TeamFortress2::GameModeType::GM_PL_RACE;
		return true;
	}

	if (map.find("arena_") != std::string::npos)
	{
		m_gamemode = TeamFortress2::GameModeType::GM_ARENA;
		return true;
	}

	if (map.find("koth_") != std::string::npos)
	{
		m_gamemode = TeamFortress2::GameModeType::GM_KOTH;
		return true;
	}

	if (map.find("sd_") != std::string::npos)
	{
		m_gamemode = TeamFortress2::GameModeType::GM_SD;
		return true;
	}

	if (map.find("trade_") != std::string::npos)
	{
		m_gamemode = TeamFortress2::GameModeType::GM_NONE;
		return true;
	}

	return false;
}

bool CTeamFortress2Mod::DetectMapViaGameRules()
{
	// Thanks to jugheadbomb for the snippet.

	int result = 0;

	// values for TF2 gamerules's m_nGameType netprop
	constexpr int TF2GR_GAMETYPE_UNKNOWN = 0;
	constexpr int TF2GR_GAMETYPE_CTF = 1;
	constexpr int TF2GR_GAMETYPE_CP = 2;
	constexpr int TF2GR_GAMETYPE_PL = 3;
	constexpr int TF2GR_GAMETYPE_ARENA = 4;

	entprops->GameRules_GetProp("m_bPlayingSpecialDeliveryMode", result);

	if (result != 0)
	{
		m_gamemode = TeamFortress2::GameModeType::GM_SD;
		rootconsole->ConsolePrint("[NavBot] Gamerules 'm_bPlayingSpecialDeliveryMode' == 1, game mode is Special Delivery.");
		return true;
	}

	entprops->GameRules_GetProp("m_bPlayingMannVsMachine", result);

	if (result != 0)
	{
		m_gamemode = TeamFortress2::GameModeType::GM_MVM;
		rootconsole->ConsolePrint("[NavBot] Gamerules 'm_bPlayingMannVsMachine' == 1, game mode is Mann vs Machine.");
		return true;
	}

	entprops->GameRules_GetProp("m_bPlayingKoth", result);

	if (result != 0)
	{
		m_gamemode = TeamFortress2::GameModeType::GM_KOTH;
		rootconsole->ConsolePrint("[NavBot] Gamerules 'm_bPlayingKoth' == 1, game mode is King Of The Hill.");
		return true;
	}

	entprops->GameRules_GetProp("m_nGameType", result);

	switch (result)
	{
	case TF2GR_GAMETYPE_CTF:
		m_gamemode = TeamFortress2::GameModeType::GM_CTF;
		rootconsole->ConsolePrint("[NavBot] Gamerules 'm_nGameType' == 1, game mode is Capture The Flag.");
		return true;
	case TF2GR_GAMETYPE_ARENA: // TO-DO: Some community game modes uses arena as a base
		m_gamemode = TeamFortress2::GameModeType::GM_ARENA;
		rootconsole->ConsolePrint("[NavBot] Gamerules 'm_nGameType' == 4, game mode is Arena.");
		return true;
	case TF2GR_GAMETYPE_PL:
		// Payload race requires this entity
		if (UtilHelpers::FindEntityByClassname(-1, "tf_logic_multiple_escort") != INVALID_EHANDLE_INDEX)
		{
			rootconsole->ConsolePrint("[NavBot] Gamerules 'm_nGameType' == 3 and entity 'tf_logic_multiple_escort' exists, game mode is Payload Race.");
			m_gamemode = TeamFortress2::GameModeType::GM_PL_RACE;
			return true;
		}

		rootconsole->ConsolePrint("[NavBot] Gamerules 'm_nGameType' == 3, game mode is Payload.");
		m_gamemode = TeamFortress2::GameModeType::GM_PL;
		return true;
	case TF2GR_GAMETYPE_CP:
	{
		// 5cp, a/d cp, and tc all show up as this game type
		// koth and mvm too, but we already filtered those
		// Check for multi-stage maps first.
		int roundCount = 0;
		int roundCP = -1;
		int priority = -1;

		int restrictWinner = -1;
		int restrictCount = 0;

		while ((roundCP = UtilHelpers::FindEntityByClassname(roundCP, "team_control_point_round")) != INVALID_EHANDLE_INDEX)
		{
			roundCount++;

			entprops->GetEntProp(roundCP, Prop_Data, "m_iInvalidCapWinner", restrictWinner);
			if (restrictWinner > 1)
			{
				restrictCount++;
			}

			int newPriority = -1;
			entprops->GetEntProp(roundCP, Prop_Data, "m_nPriority", newPriority);
			if (newPriority > priority)
			{
				priority = newPriority;
			}
			else if (newPriority == priority)
			{
				// Only TC maps have multiple rounds with the same priority, and it's the highest priority
				// Sadly, this will fail to detect push/pull TC maps
				rootconsole->ConsolePrint("[NavBot] Gamerules 'm_nGameType' == 2, control point setup indicates Territorial Control.");
				m_gamemode = TeamFortress2::GameModeType::GM_TC;
				return true;
			}
		}

		// All rounds have a winner restriction, so it must be a A/D cp map
		if (roundCount > 1 && roundCount == restrictCount)
		{
			rootconsole->ConsolePrint("[NavBot] Gamerules 'm_nGameType' == 2, control point setup indicates Attack/Defence.");
			m_gamemode = TeamFortress2::GameModeType::GM_ADCP;
			return true;
		}

		if (roundCount > 1)
		{
			// We had multiple rounds, but not all of them were restricted...
			// must be a push/pull TC map
			rootconsole->ConsolePrint("[NavBot] Gamerules 'm_nGameType' == 2, control point setup indicates Territorial Control.");
			m_gamemode = TeamFortress2::GameModeType::GM_TC;
			return true;
		}

		// Now for single round maps... same check on control point master
		int masterCP = UtilHelpers::FindEntityByClassname(-1, "team_control_point_master");

		if (masterCP != INVALID_EHANDLE_INDEX)
		{
			// Single round restricted are always A/D (gorge, gravelpit)
			entprops->GetEntProp(masterCP, Prop_Data, "m_iInvalidCapWinner", restrictWinner);
			if (restrictWinner > 1)
			{
				rootconsole->ConsolePrint("[NavBot] Gamerules 'm_nGameType' == 2, control point setup indicates Attack/Defence.");
				m_gamemode = TeamFortress2::GameModeType::GM_ADCP;
				return true;
			}
		}

		rootconsole->ConsolePrint("[NavBot] Gamerules 'm_nGameType' == 2, game mode is Control Point.");
		m_gamemode = TeamFortress2::GameModeType::GM_CP;
		return true;
	}
	default:
		break;
	}

	return false;
}

bool CTeamFortress2Mod::DetectKoth()
{
	int logic_koth = UtilHelpers::FindEntityByClassname(gpGlobals->maxClients + 1, "tf_logic_koth");

	if (logic_koth != INVALID_EHANDLE_INDEX)
	{
		rootconsole->ConsolePrint("[NavBot] Found 'tf_logic_koth', game mode is King Of The Hill.");
		m_gamemode = TeamFortress2::GameModeType::GM_KOTH;
		return true;
	}

	return false;
}

bool CTeamFortress2Mod::DetectPlayerDestruction()
{
	int logic_pd = UtilHelpers::FindEntityByClassname(gpGlobals->maxClients + 1, "tf_logic_player_destruction");

	if (logic_pd != INVALID_EHANDLE_INDEX)
	{
		rootconsole->ConsolePrint("[NavBot] Found 'tf_logic_player_destruction', game mode is Player Destruction.");
		m_gamemode = TeamFortress2::GameModeType::GM_PD;
		return true;
	}

	return false;
}

void CTeamFortress2Mod::FindPayloadCarts()
{
	m_red_payload.Term();
	m_blu_payload.Term();

	UtilHelpers::ForEachEntityOfClassname("team_train_watcher", [this](int index, edict_t* edict, CBaseEntity* entity) {
		if (edict == nullptr)
		{
			return true; // return early, keep loop
		}

		tfentities::HTFBaseEntity be(entity);

		if (be.IsDisabled())
		{
			return true; // return early, keep loop
		}

		if (!m_red_payload.IsValid() && be.GetTFTeam() == TeamFortress2::TFTeam::TFTeam_Red)
		{
			std::unique_ptr<char[]> targetname = std::make_unique<char[]>(256);
			std::size_t len = 0;

			if (entprops->GetEntPropString(index, Prop_Data, "m_iszTrain", targetname.get(), 256, len))
			{
				int train = UtilHelpers::FindEntityByTargetname(INVALID_EHANDLE_INDEX, targetname.get());
				CBaseEntity* pTrain = gamehelpers->ReferenceToEntity(train);

				if (pTrain != nullptr)
				{
					if (sm_navbot_tf_mod_debug.GetBool())
					{
						smutils->LogMessage(myself, "[DEBUG] Found RED payload cart %s<#%i>", targetname.get(), train);
					}

					m_red_payload.Set(pTrain);
				}
			}
		}

		if (!m_blu_payload.IsValid() && be.GetTFTeam() == TeamFortress2::TFTeam::TFTeam_Blue)
		{
			std::unique_ptr<char[]> targetname = std::make_unique<char[]>(256);
			std::size_t len = 0;

			if (entprops->GetEntPropString(index, Prop_Data, "m_iszTrain", targetname.get(), 256, len))
			{
				int train = UtilHelpers::FindEntityByTargetname(INVALID_EHANDLE_INDEX, targetname.get());
				CBaseEntity* pTrain = gamehelpers->ReferenceToEntity(train);

				if (pTrain != nullptr)
				{
					if (sm_navbot_tf_mod_debug.GetBool())
					{
						smutils->LogMessage(myself, "[DEBUG] Found BLU payload cart %s<#%i>", targetname.get(), train);
					}

					m_blu_payload.Set(pTrain);
				}
			}
		}

		return true; // keep loop
	});
}

void CTeamFortress2Mod::FindControlPoints()
{
	for (auto& handle : m_controlpoints)
	{
		handle.Term();
	}

	size_t i = 0;

	UtilHelpers::ForEachEntityOfClassname("team_control_point", [this, &i](int index, edict_t* edict, CBaseEntity* entity) {
		m_controlpoints[i].Set(entity);
		
		if (++i >= TeamFortress2::TF_MAX_CONTROL_POINTS)
		{
			return false; // end search
		}

		return true;
	});

	if (sm_navbot_tf_mod_debug.GetBool())
	{
		smutils->LogMessage(myself, "[DEBUG] Found %i control points.", i);

		for (auto& handle : m_controlpoints)
		{
			if (handle.IsValid())
			{
				smutils->LogMessage(myself, "[DEBUG] Control Point: %i", handle.GetEntryIndex());
			}
		}
	}
}

bool CTeamFortress2Mod::ShouldSwitchClass(CTF2Bot* bot) const
{
	std::string classname(sm_navbot_tf_force_class.GetString());
	auto forcedclass = tf2lib::GetClassTypeFromName(classname);

	if (forcedclass != TeamFortress2::TFClassType::TFClass_Unknown)
	{
		return forcedclass != bot->GetMyClassType();
	}

	return m_classselector.IsClassAboveLimit(bot->GetMyClassType(), bot->GetMyTFTeam(), GetRosterForTeam(bot->GetMyTFTeam()));
}

TeamFortress2::TFClassType CTeamFortress2Mod::SelectAClassForBot(CTF2Bot* bot) const
{
	std::string classname(sm_navbot_tf_force_class.GetString());
	auto forcedclass = tf2lib::GetClassTypeFromName(classname);

	if (forcedclass != TeamFortress2::TFClassType::TFClass_Unknown)
	{
		return forcedclass;
	}

	return m_classselector.SelectAClass(bot->GetMyTFTeam(), GetRosterForTeam(bot->GetMyTFTeam()));
}

TeamFortress2::TeamRoles CTeamFortress2Mod::GetTeamRole(TeamFortress2::TFTeam team) const
{
	auto teamentity = UtilHelpers::GetTeamManagerEntity(static_cast<int>(team), "tf_team");

	if (!teamentity.has_value()) // failed to find team manager entity
		return TeamFortress2::TeamRoles::TEAM_ROLE_NONE;

	int role = 0;

	entprops->GetEntProp(teamentity.value(), Prop_Send, "m_iRole", role);

	return static_cast<TeamFortress2::TeamRoles>(role);
}

CTF2ClassSelection::ClassRosterType CTeamFortress2Mod::GetRosterForTeam(TeamFortress2::TFTeam team) const
{
	switch (m_gamemode)
	{
	case TeamFortress2::GameModeType::GM_NONE:
		return CTF2ClassSelection::ClassRosterType::ROSTER_DEFAULT;
	case TeamFortress2::GameModeType::GM_CTF:
		return CTF2ClassSelection::ClassRosterType::ROSTER_DEFAULT;
	case TeamFortress2::GameModeType::GM_CP:
		return CTF2ClassSelection::ClassRosterType::ROSTER_DEFAULT;
	case TeamFortress2::GameModeType::GM_ADCP:
		break;
	case TeamFortress2::GameModeType::GM_KOTH:
		return CTF2ClassSelection::ClassRosterType::ROSTER_DEFAULT;
	case TeamFortress2::GameModeType::GM_MVM:
		return CTF2ClassSelection::ClassRosterType::ROSTER_MANNVSMACHINE;
	case TeamFortress2::GameModeType::GM_PL:
		break;
	case TeamFortress2::GameModeType::GM_PL_RACE:
		return CTF2ClassSelection::ClassRosterType::ROSTER_DEFAULT;
	case TeamFortress2::GameModeType::GM_PD:
		return CTF2ClassSelection::ClassRosterType::ROSTER_DEFAULT;
	case TeamFortress2::GameModeType::GM_SD:
		return CTF2ClassSelection::ClassRosterType::ROSTER_DEFAULT;
	case TeamFortress2::GameModeType::GM_TC:
		break;
	case TeamFortress2::GameModeType::GM_ARENA:
		return CTF2ClassSelection::ClassRosterType::ROSTER_DEFAULT;
	case TeamFortress2::GameModeType::GM_MAX_GAMEMODE_TYPES:
	default:
		break;
	}

	auto role = GetTeamRole(team);

	switch (role)
	{
	case TeamFortress2::TEAM_ROLE_DEFENDERS:
		return CTF2ClassSelection::ClassRosterType::ROSTER_DEFENDERS;
	case TeamFortress2::TEAM_ROLE_ATTACKERS:
		return CTF2ClassSelection::ClassRosterType::ROSTER_ATTACKERS;
	default:
		return CTF2ClassSelection::ClassRosterType::ROSTER_DEFAULT;
	}
}

edict_t* CTeamFortress2Mod::GetFlagToFetch(TeamFortress2::TFTeam team)
{
	int flag = INVALID_EHANDLE_INDEX;

	while ((flag = UtilHelpers::FindEntityByClassname(flag, "item_teamflag")) != INVALID_EHANDLE_INDEX)
	{
		bool disabled = false;
		entprops->GetEntPropBool(flag, Prop_Send, "m_bDisabled", disabled);

		if (disabled)
			continue;

		auto entteam = tf2lib::GetEntityTFTeam(flag);

		if (entteam != tf2lib::GetEnemyTFTeam(team))
			continue;

		auto owner = UtilHelpers::GetOwnerEntity(flag);

		if (owner.value_or(INVALID_EHANDLE_INDEX) != INVALID_EHANDLE_INDEX)
			continue; // only fetch dropped flags

		// return the first flag
		edict_t* pFlag = nullptr;

		if (UtilHelpers::IndexToAThings(flag, nullptr, &pFlag))
		{
			return pFlag;
		}
	}

	return nullptr;
}

void CTeamFortress2Mod::OnRoundStart()
{
	FindPayloadCarts();
	FindControlPoints();

	extmanager->ForEachBot([](CBaseBot* bot) {
		bot->OnRoundStateChanged();
	});
}

#if SOURCE_ENGINE == SE_TF2

CON_COMMAND(sm_navbot_tf_show_upgrades, "[TF2] List all MvM Upgrades known by the bots.")
{
	CTeamFortress2Mod::GetTF2Mod()->GetMvMUpgradeManager().ConCommand_ListUpgrades();
}

CON_COMMAND(sm_navbot_tf_reload_upgrades, "[TF2] Reload MvM Upgrades")
{
	CTeamFortress2Mod::GetTF2Mod()->ReloadUpgradeManager();
	rootconsole->ConsolePrint("Reloaded MvM Upgrades.");
}

#endif // SOURCE_ENGINE == SE_TF2
