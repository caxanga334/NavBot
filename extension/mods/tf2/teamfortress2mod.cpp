#include NAVBOT_PCH_FILE
#include <cstring>
#include <algorithm>

#include <extension.h>
#include <manager.h>
#include <extplayer.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <bot/basebot.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/nav/tfnavmesh.h>
#include <mods/tf2/nav/tfnav_waypoint.h>
#include <entities/tf2/tf_entities.h>
#include "tf2lib.h"
#include "teamfortress2mod.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

#undef min
#undef max
#undef clamp

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
	"ROBOT DESTRUCTION",
	"SPECIAL DELIVERY",
	"TERRITORIAL CONTROL",
	"ARENA",
	"PASS TIME",
	"VERSUS SAXTON HALE (VSCRIPT)",
	"ZOMBIE INFECTION",
	"GUN GAME",
	"DEATHMATCH",
	"SLENDER FORTRESS",
	"FREE FOR ALL DEATHMATCH (VSCRIPT)",
	"TUG OF WAR"
};

static_assert((sizeof(s_tf2gamemodenames) / sizeof(char*)) == static_cast<int>(TeamFortress2::GameModeType::GM_MAX_GAMEMODE_TYPES), 
	"You added or removed a TF2 game mode and forgot to update the game mode name array!");

SourceMod::SMCResult CTF2ModSettings::ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value)
{
	if (cfg_parser_depth == 1)
	{
		if (strncasecmp(key, "engineer_nest_dispenser_range", 29) == 0)
		{
			float range = atof(value);
			range = std::clamp(range, 600.0f, 4096.0f);
			SetEngineerNestDispenserRange(range);
			return SourceMod::SMCResult_Continue;
		}
		if (strncasecmp(key, "engineer_nest_exit_range", 24) == 0)
		{
			float range = atof(value);
			range = std::clamp(range, 600.0f, 4096.0f);
			SetEngineerNestExitRange(range);
			return SourceMod::SMCResult_Continue;
		}
		if (strncasecmp(key, "entrance_spawn_range", 20) == 0)
		{
			float range = atof(value);
			range = std::clamp(range, 1500.0f, 6000.0f);
			SetEntranceSpawnRange(range);
			return SourceMod::SMCResult_Continue;
		}
		if (strncasecmp(key, "mvm_sentry_to_bomb_range", 24) == 0)
		{
			float range = atof(value);
			range = std::clamp(range, 1000.0f, 3000.0f);
			SetMvMSentryToBombRange(range);
			return SourceMod::SMCResult_Continue;
		}
		if (strncasecmp(key, "medic_patient_scan_range", 24) == 0)
		{
			float range = atof(value);
			range = std::clamp(range, 750.0f, 5000.0f);
			SetMedicPatientScanRange(range);
			return SourceMod::SMCResult_Continue;
		}
		if (strncasecmp(key, "engineer_destroy_travel_range", 29) == 0)
		{
			float range = atof(value);
			range = std::clamp(range, 1000.0f, 10000.0f);
			SetEngineerMoveDestroyBuildingRange(range);
			return SourceMod::SMCResult_Continue;
		}
		if (strncasecmp(key, "engineer_move_check_interval", 28) == 0)
		{
			float v = atof(value);
			v = std::clamp(v, 30.0f, 180.0f);
			SetEngineerMoveCheckInterval(v);
			return SourceMod::SMCResult_Continue;
		}
		if (strncasecmp(key, "engineer_sentry_killassist_threshold", 36) == 0)
		{
			int v = atoi(value);
			v = std::clamp(v, 1, 30);
			SetEngineerSentryKillAssistsThreshold(v);
			return SourceMod::SMCResult_Continue;
		}
		if (strncasecmp(key, "engineer_teleporter_uses_threshold", 34) == 0)
		{
			int v = atoi(value);
			v = std::clamp(v, 1, 10);
			SetEngineerTeleporterUsesThreshold(v);
			return SourceMod::SMCResult_Continue;
		}
		if (strncasecmp(key, "engineer_help_ally_max_range", 28) == 0)
		{
			float range = atof(value);
			range = std::clamp(range, 500.0f, 4000.0f);
			SetEngineerHelpAllyMaxRange(range);
			return SourceMod::SMCResult_Continue;
		}
		if (strncasecmp(key, "engineer_nav_build_range", 24) == 0)
		{
			float range = atof(value);
			range = std::clamp(range, 1024.0f, 10000.0f);
			SetEngineerRandomNavAreaBuildRange(range);
			return SourceMod::SMCResult_Continue;
		}
		if (strncasecmp(key, "engineer_nav_build_check_vis", 28) == 0)
		{
			SetRandomNavAreaBuildCheckForVisiblity(UtilHelpers::StringToBoolean(value));
			return SourceMod::SMCResult_Continue;
		}
		if (strncasecmp(key, "setup_time_allow_mess_around", 28) == 0)
		{
			SetIsAllowedToMessAroundDuringSetupTime(UtilHelpers::StringToBoolean(value));
			return SourceMod::SMCResult_Continue;
		}
		if (strncasecmp(key, "engineer_trust_waypoints", 24) == 0)
		{
			SetEngineerTrustWaypoints(UtilHelpers::StringToBoolean(value));
			return SourceMod::SMCResult_Continue;
		}
		if (strncasecmp(key, "vsh_saxton_hale_team", 20) == 0)
		{
			SetVSHSaxtonHaleTeam(tf2lib::TFTeamFromString(value));
			return SourceMod::SMCResult_Continue;
		}
	}

	return CModSettings::ReadSMC_KeyValue(states, key, value);
}

CTeamFortress2Mod::CTeamFortress2Mod() : CBaseMod()
{
	m_gamemode = TeamFortress2::GameModeType::GM_NONE;
	m_bInSetup = false;
	m_isTruceActive = false;
	m_MvMHatchPos = vec3_origin;

	ListenForGameEvent("teamplay_round_start");
	ListenForGameEvent("teamplay_setup_finished");
	ListenForGameEvent("controlpoint_initialized");
	ListenForGameEvent("mvm_begin_wave");
	ListenForGameEvent("mvm_wave_complete");
	ListenForGameEvent("mvm_wave_failed");
	ListenForGameEvent("teamplay_flag_event");
	ListenForGameEvent("teamplay_point_startcapture");
	ListenForGameEvent("teamplay_point_captured");
	ListenForGameEvent("recalculate_truce");
	ListenForGameEvent("player_upgradedobject");
	ListenForGameEvent("player_sapped_object");

	CServerCommandManager& manager = extmanager->GetServerCommandManager();

	m_cvar_forceclass = manager.RegisterConVar("sm_navbot_tf_force_class", "Forces all NavBots to use the specified class.", "none", FCVAR_GAMEDLL);
	m_cvar_debug = manager.RegisterConVar("sm_navbot_tf_mod_debug", "TF2 mod debugging.", "0", FCVAR_GAMEDLL);
	m_cvar_forcegamemode = manager.RegisterConVar("sm_navbot_tf_force_gamemode", "Skips game mode detection and forces a specific game mode. -1 to disable.", "-1", FCVAR_GAMEDLL, CTeamFortress2Mod::OnForceGamemodeConVarChanged);
	m_cvar_gamemode_fs = manager.RegisterConVar("sm_navbot_tf_file_based_gamemode_detection", "If enabled, allow detecting game modes via filesystem.", "1", FCVAR_GAMEDLL);
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

#if defined(EXT_DEBUG) && defined(TF_DLL)
		ConColorMsg(Color(255, 200, 150, 255), "CTeamFortress2Mod::FireGameEvent %s \n", name);
#endif // defined(EXT_DEBUG) && defined(TF_DLL)

		if (strncasecmp(name, "teamplay_round_start", 20) == 0)
		{
			CheckForSetup();
			OnRoundStart();
		}
		else if (strncasecmp(name, "mvm_begin_wave", 14) == 0)
		{
			OnRoundStart();
		}
		else if (strncasecmp(name, "mvm_wave_complete", 17) == 0)
		{
			auto func = [&event](CBaseBot* baseBot) {
				static_cast<CTF2Bot*>(baseBot)->GetUpgradeManager().OnWaveEnd();
				baseBot->OnGameEvent(event, nullptr);
			};

			extmanager->ForEachBot(func);

			OnRoundStart();
		}
		else if (strncasecmp(name, "mvm_wave_failed", 15) == 0)
		{
			auto func = [](CBaseBot* baseBot) {
				static_cast<CTF2Bot*>(baseBot)->GetUpgradeManager().OnWaveFailed();
			};
			extmanager->ForEachBot(func);

			OnRoundStart();
		}
		else if (strncasecmp(name, "controlpoint_initialized", 24) == 0)
		{
			FindControlPoints();
		}
		else if (strncasecmp(name, "teamplay_setup_finished", 23) == 0)
		{
			m_bInSetup = false;
		}
		else if (strncasecmp(name, "teamplay_flag_event", 19) == 0)
		{
			edict_t* edict = gamehelpers->EdictOfIndex(event->GetInt("player", -1));

			if (UtilHelpers::IsValidEdict(edict))
			{
				CBaseEntity* entity = UtilHelpers::EdictToBaseEntity(edict);

				TeamFortress2::TFFlagEvent flagevent = static_cast<TeamFortress2::TFFlagEvent>(event->GetInt("eventtype", 0));

				if (flagevent == TeamFortress2::TF_FLAGEVENT_PICKEDUP)
				{
					auto func = [&entity](CBaseBot* bot) {
						bot->OnFlagTaken(entity);
					};

					extmanager->ForEachBot(func);
				}
				else if (flagevent == TeamFortress2::TF_FLAGEVENT_DROPPED)
				{
					auto func = [&entity](CBaseBot* bot) {
						bot->OnFlagDropped(entity);
					};

					extmanager->ForEachBot(func);
				}
			}
		}
		else if (strncasecmp(name, "teamplay_point_captured", 23) == 0)
		{
			int pointID = event->GetInt("cp");
			int teamWhoCapped = event->GetInt("team");
			CBaseEntity* entity = GetControlPointByIndex(pointID);

			if (entity)
			{
				auto func = [&entity, &teamWhoCapped](CBaseBot* bot) {

					if (bot->GetCurrentTeamIndex() == teamWhoCapped)
					{
						bot->OnControlPointCaptured(entity);
					}
					else
					{
						bot->OnControlPointLost(entity);
					}
				};
				extmanager->ForEachBot(func);
			}
		}
		else if (strncasecmp(name, "teamplay_point_startcapture", 27) == 0)
		{
			int pointID = event->GetInt("cp");
			CBaseEntity* entity = GetControlPointByIndex(pointID);

			if (entity)
			{
				auto func = [&entity](CBaseBot* bot) {
					bot->OnControlPointContested(entity);
				};
				extmanager->ForEachBot(func);
			}
		}
		else if (strncasecmp(name, "recalculate_truce", 17) == 0)
		{
			m_updateTruceStatus.Start(0.5f);
		}
		else if (std::strcmp(name, "player_upgradedobject") == 0)
		{
			bool isBuilder = event->GetBool("isbuilder", true);
			
			if (!isBuilder)
			{
				int entindex = event->GetInt("index", INVALID_EHANDLE_INDEX);
				
				if (entindex != INVALID_EHANDLE_INDEX)
				{
					int builder = 0;
					entprops->GetEntPropEnt(entindex, Prop_Send, "m_hBuilder", &builder);

					if (builder != 0)
					{
						CBaseBot* bot = extmanager->GetBotByIndex(builder);

						if (bot && randomgen->GetRandomChance(bot->GetDifficultyProfile()->GetTeamwork()))
						{
							static_cast<CTF2Bot*>(bot)->SendVoiceCommand(TeamFortress2::VoiceCommandsID::VC_THANKS);
						}
					}
				}
			}
		}
		else if (std::strcmp(name, "player_sapped_object") == 0)
		{
			int owner = playerhelpers->GetClientOfUserId(event->GetInt("ownerid", -1));
			int saboteur = playerhelpers->GetClientOfUserId(event->GetInt("userid", -1));

			if (owner != 0 && saboteur != 0)
			{
				CBaseEntity* pOwner = gamehelpers->ReferenceToEntity(owner);
				CBaseEntity* pSaboteur = gamehelpers->ReferenceToEntity(saboteur);

				auto func = [&pOwner, &pSaboteur](CBaseBot* bot) {
					bot->OnObjectSapped(pOwner, pSaboteur);
				};

				extmanager->ForEachBot(func);
			}
		}
	}
}

std::string CTeamFortress2Mod::GetCurrentMapName() const
{
	// TF2 workshop names follows this format: workshop/ctf_harbine.ugc3067683041
	// this method should be fine for most maps
	std::string mapname(gamehelpers->GetCurrentMap());

	auto n1 = mapname.find("workshop/");
	auto n2 = mapname.find(".ugc");

	if (n1 == std::string::npos)
	{
		return mapname; // not a workshop map
	}

	constexpr size_t WORKSHOP_STR_SIZE = 9U;
	constexpr size_t UGC_STR_SIZE = 4U;
	auto count = n2 - (n1 + WORKSHOP_STR_SIZE);
	auto sub1 = mapname.substr(n1 + WORKSHOP_STR_SIZE, count);
	auto sub2 = mapname.substr(n2 + UGC_STR_SIZE);
	std::string finalname = sub1 + "_ugc" + sub2; // becomes something like this: ctf_harbine_ugc3067683041
	return finalname;
}

void CTeamFortress2Mod::Update()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CTeamFortress2Mod::Update", "NavBot");
#endif // EXT_VPROF_ENABLED

	CBaseMod::Update();

	using namespace TeamFortress2;

	if (m_gamemode == GameModeType::GM_PL || m_gamemode == GameModeType::GM_PL_RACE)
	{
		if (m_updatePayloadTimer.IsElapsed())
		{
			m_updatePayloadTimer.Start(5.0f);
			// some community maps have multiple payloads, we need to update this from time to time.
			FindPayloadCarts();
		}
	}

	if (m_bInSetup)
	{
		if (m_setupExpireTimer.IsElapsed())
		{
			Warning("[NAVBOT-TF2] \"teamplay_setup_finished\" game event didn't fire!\n");
			m_bInSetup = false;
		}
	}

	if (m_updateTruceStatus.HasStarted() && m_updateTruceStatus.IsElapsed())
	{
		m_updateTruceStatus.Invalidate();

		bool inTruce = false;
		entprops->GameRules_GetPropBool("m_bTruceActive", inTruce);

		if (m_isTruceActive != inTruce)
		{
			m_isTruceActive = inTruce;

			auto func = [&inTruce](CBaseBot* bot) {
				bot->OnTruceChanged(inTruce);
			};

			extmanager->ForEachBot(func);
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
	m_bInSetup = false;
	m_isTruceActive = false;

	if (m_gamemode == TeamFortress2::GameModeType::GM_MVM)
	{
		FindMvMBombHatchPosition();
	}
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

void CTeamFortress2Mod::OnNavMeshLoaded()
{
	auto functor = [this](CTFWaypoint* waypoint) {
		if (waypoint->GetTFHint() == CTFWaypoint::TFHint::TFHINT_SNIPER && waypoint->GetNumOfAvailableAngles() > 0)
		{
			m_sniperWaypoints.push_back(waypoint);
		}

		if (waypoint->HasFlags(CWaypoint::BaseFlags::BASEFLAGS_SNIPER) && waypoint->GetNumOfAvailableAngles() > 0)
		{
			m_sniperWaypoints.push_back(waypoint);
		}

		if (waypoint->GetTFHint() == CTFWaypoint::TFHint::TFHINT_SENTRYGUN && waypoint->GetNumOfAvailableAngles() > 0)
		{
			m_sentryWaypoints.push_back(waypoint);
		}

		if (waypoint->GetTFHint() == CTFWaypoint::TFHint::TFHINT_DISPENSER && waypoint->GetNumOfAvailableAngles() > 0)
		{
			m_dispenserWaypoints.push_back(waypoint);
		}

		if (waypoint->GetTFHint() == CTFWaypoint::TFHint::TFHINT_TELE_ENTRANCE && waypoint->GetNumOfAvailableAngles() > 0)
		{
			m_teleentranceWaypoints.push_back(waypoint);
		}

		if (waypoint->GetTFHint() == CTFWaypoint::TFHint::TFHINT_TELE_EXIT && waypoint->GetNumOfAvailableAngles() > 0)
		{
			m_teleexitWaypoints.push_back(waypoint);
		}

		return true;
	};

	TheNavMesh->ForEveryWaypoint<CTFWaypoint>(functor);
}

void CTeamFortress2Mod::OnNavMeshDestroyed()
{
	m_sniperWaypoints.clear();
	m_sentryWaypoints.clear();
	m_dispenserWaypoints.clear();
	m_teleentranceWaypoints.clear();
	m_teleexitWaypoints.clear();
}

const char* CTeamFortress2Mod::GetCurrentGameModeName() const
{
	return s_tf2gamemodenames[static_cast<int>(m_gamemode)];
}

void CTeamFortress2Mod::DetectCurrentGameMode()
{
	int forcedmode = m_cvar_forcegamemode->GetInt();

	if (forcedmode >= 0 && forcedmode < static_cast<int>(TeamFortress2::GameModeType::GM_MAX_GAMEMODE_TYPES))
	{
		m_gamemode = static_cast<TeamFortress2::GameModeType>(forcedmode);
		smutils->LogMessage(myself, "Using forced game mode via sm_navbot_tf_force_gamemode ConVar: %s", GetCurrentGameModeName());
		return;
	}

	// Community modes that needs to be detected before game rules method
	if (DetectCommunityGameModes())
	{
		return;
	}

	if (DetectMapViaGameRules())
	{
		return;
	}

	if (DetectMapViaEntities())
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

	std::string map = tf2lib::maps::GetMapName();

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

bool CTeamFortress2Mod::DetectCommunityGameModes()
{
	/*
	* Detects the map type via the map prefix
	*/

	std::string map = tf2lib::maps::GetMapName();

	if (m_cvar_gamemode_fs->GetBool())
	{
		if (filesystem->FileExists("scripts/vscripts/vssaxtonhale/vsh.nut", "BSP"))
		{
			m_gamemode = TeamFortress2::GameModeType::GM_VSH;
			META_CONPRINT("[NavBot] Found scripts/vscripts/vssaxtonhale/vsh.nut packed inside BSP, assuming Versus Saxton Hale (vscript)! \n");
			return true;
		}

		if (filesystem->FileExists("scripts/vscripts/ffa/ffa.nut", "BSP"))
		{
			m_gamemode = TeamFortress2::GameModeType::GM_VSFFA;
			META_CONPRINT("[NavBot] Found scripts/vscripts/ffa/ffa.nut packed inside BSP, assuming OF Free For All Deathmatch (vscript)! \n");
			return true;
		}

		if (filesystem->FileExists("scripts/vscripts/infection/infection.nut", "BSP"))
		{
			m_gamemode = TeamFortress2::GameModeType::GM_ZI;
			META_CONPRINT("[NavBot] Found scripts/vscripts/infection/infection.nut packed inside BSP, assuming Zombie Infection (vscript)! \n");
			return true;
		}
	}

	if (map.find("gg_") != std::string::npos)
	{
		m_gamemode = TeamFortress2::GameModeType::GM_GG;
		return true;
	}

	if (map.find("slender_") != std::string::npos)
	{
		m_gamemode = TeamFortress2::GameModeType::GM_SF;
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
		if (UtilHelpers::FindEntityByClassname(INVALID_EHANDLE_INDEX, "team_train_watcher") != INVALID_EHANDLE_INDEX &&
			UtilHelpers::GetEntityOfClassnameCount("team_control_point") == 3)
		{
			m_gamemode = TeamFortress2::GameModeType::GM_TOW;
			rootconsole->ConsolePrint("[NavBot] Gamerules 'm_bPlayingKoth' == 1, Map setup suggests it's a tug of war map.");
			return true;
		}

		m_gamemode = TeamFortress2::GameModeType::GM_KOTH;
		rootconsole->ConsolePrint("[NavBot] Gamerules 'm_bPlayingKoth' == 1, game mode is King Of The Hill.");
		return true;
	}

	entprops->GameRules_GetProp("m_nGameType", result);

	switch (result)
	{
	case TF2GR_GAMETYPE_CTF:
	{
		// Some SD maps have m_bPlayingSpecialDeliveryMode set to false

		int num_flags = 0;
		bool foundrcflag = false; // Resource Control flag

		auto functor = [&num_flags, &foundrcflag](int index, edict_t* edict, CBaseEntity* entity) {
			if (entity)
			{
				num_flags++;
				int type = -1;
				entprops->GetEntProp(index, Prop_Data, "m_nType", type);

				if (type == static_cast<int>(TeamFortress2::TFFlagType::TF_FLAGTYPE_RESOURCE_CONTROL))
				{
					foundrcflag = true;
				}
			}

			return true;
		};

		UtilHelpers::ForEachEntityOfClassname("item_teamflag", functor);

		if (num_flags == 1 && foundrcflag)
		{
			m_gamemode = TeamFortress2::GameModeType::GM_SD;
			rootconsole->ConsolePrint("[NavBot] Gamerules 'm_nGameType' == 1, found only one flag with GameType set to TF_FLAGTYPE_RESOURCE_CONTROL.");
			return true;
		}

		m_gamemode = TeamFortress2::GameModeType::GM_CTF;
		rootconsole->ConsolePrint("[NavBot] Gamerules 'm_nGameType' == 1, game mode is Capture The Flag.");
		return true;
	}
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

bool CTeamFortress2Mod::DetectMapViaEntities()
{
	if (UtilHelpers::FindEntityByClassname(INVALID_EHANDLE_INDEX, "passtime_logic") != INVALID_EHANDLE_INDEX)
	{
		rootconsole->ConsolePrint("[NavBot] Found 'passtime_logic', game mode is Pass Time.");
		m_gamemode = TeamFortress2::GameModeType::GM_PASSTIME;
		return true;
	}

	if (UtilHelpers::FindEntityByClassname(INVALID_EHANDLE_INDEX, "tf_logic_robot_destruction") != INVALID_EHANDLE_INDEX)
	{
		rootconsole->ConsolePrint("[NavBot] Found 'tf_logic_robot_destruction', game mode is Robot Destruction.");
		m_gamemode = TeamFortress2::GameModeType::GM_RD;
		return true;
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

static CBaseEntity* FindCaptureTriggerForTrain(CBaseEntity* pTrain, TeamFortress2::TFTeam team, CTeamFortress2Mod* tf2mod)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("FindCaptureTriggerForTrain", "NavBot");
#endif // EXT_VPROF_ENABLED

	CBaseEntity* pTrigger = nullptr;

	// Method 1: On most maps, the trigger is parented to the payload's func_tracktrain entity.
	auto m1func = [&pTrigger, &pTrain](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity != nullptr)
		{
			tfentities::HTFBaseEntity capture(entity);
			if (capture.GetMoveParent() == pTrain)
			{
				pTrigger = entity;
				return false;
			}
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("trigger_capture_area", m1func);

	if (pTrigger != nullptr)
	{
		return pTrigger;
	}

	std::vector<CBaseEntity*> attackpoints;
	tf2mod->CollectControlPointsToAttack(team, attackpoints);

	// Method 2: On some maps (example: pl_embargo) the capture trigger is parented to a secondary train.
	// Use the first avaialble trigger_capture_area
	auto m2func = [&pTrigger, &pTrain, &attackpoints](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity != nullptr)
		{
			tfentities::HTFBaseEntity capture(entity);

			if (capture.IsDisabled())
			{
				return true; // skip disabled capture zones
			}

			char cappointname[64]{};
			std::size_t len = 0;

			entprops->GetEntPropString(index, Prop_Data, "m_iszCapPointName", cappointname, sizeof(cappointname), len);

			if (len == 0)
			{
				return true; // no name?
			}

			int controlpoint = UtilHelpers::FindEntityByTargetname(INVALID_EHANDLE_INDEX, cappointname);

			if (controlpoint == INVALID_EHANDLE_INDEX)
			{
				return true; // skip this one
			}

			CBaseEntity* pControlPoint = gamehelpers->ReferenceToEntity(controlpoint);

			for (auto& pAttackControlPoint : attackpoints)
			{
				if (pAttackControlPoint == pControlPoint)
				{

					pTrigger = entity;
					return false;
				}
			}
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("trigger_capture_area", m2func);

	if (pTrigger != nullptr)
	{
		return pTrigger;
	}

	// As a last resort, use the first enabled trigger_capture_area
	// This works fine for pl_embargo. Most maps will stop at method 1.
	auto m3func = [&pTrigger](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity != nullptr)
		{
			tfentities::HTFBaseEntity capture(entity);

			if (capture.IsDisabled())
			{
				return true; // skip disabled capture zones
			}

			pTrigger = entity;
			return false;
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("trigger_capture_area", m3func);

	return pTrigger;
}

void CTeamFortress2Mod::FindPayloadCarts()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CTeamFortress2Mod::FindPayloadCarts", "NavBot");
#endif // EXT_VPROF_ENABLED

	m_red_payload.Term();
	m_blu_payload.Term();

	auto functor = [this](int index, edict_t* edict, CBaseEntity* entity) {
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

					CBaseEntity* pTrigger = FindCaptureTriggerForTrain(pTrain, TeamFortress2::TFTeam::TFTeam_Red, this);

					if (m_cvar_debug->GetBool())
					{
						smutils->LogMessage(myself, "[DEBUG] Found RED payload cart %s<#%i> Trigger: %p", targetname.get(), train, pTrigger);
					}

					// if a trigger is found, store it
					m_red_payload.Set(pTrigger != nullptr ? pTrigger : pTrain);
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
					CBaseEntity* pTrigger = FindCaptureTriggerForTrain(pTrain, TeamFortress2::TFTeam::TFTeam_Blue, this);

					if (m_cvar_debug->GetBool())
					{
						smutils->LogMessage(myself, "[DEBUG] Found RED payload cart %s<#%i> Trigger: %p", targetname.get(), train, pTrigger);
					}

					// if a trigger is found, store it
					m_blu_payload.Set(pTrigger != nullptr ? pTrigger : pTrain);
				}
			}
		}

		return true; // keep loop
	};

	UtilHelpers::ForEachEntityOfClassname("team_train_watcher", functor);
}

void CTeamFortress2Mod::FindControlPoints()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CTeamFortress2Mod::FindControlPoints", "NavBot");
#endif // EXT_VPROF_ENABLED

	for (auto& handle : m_controlpoints)
	{
		handle.Term();
	}

	size_t i = 0;
	auto functor = [this, &i](int index, edict_t* edict, CBaseEntity* entity) {
		m_controlpoints[i].Set(entity);

		if (++i >= TeamFortress2::TF_MAX_CONTROL_POINTS)
		{
			return false; // end search
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("team_control_point", functor);

	if (m_cvar_debug->GetBool())
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

void CTeamFortress2Mod::CheckForSetup()
{
	bool setup = false;
	int setuplength = 0;
	auto functor = [&setup, &setuplength](int index, edict_t* edict, CBaseEntity* entity) {

		int disabled = 0;
		entprops->GetEntProp(index, Prop_Send, "m_bIsDisabled", disabled);

		if (disabled != 0)
		{
			return true; // exit early, keep searching
		}

		int paused = 0;
		entprops->GetEntProp(index, Prop_Send, "m_bTimerPaused", paused);

		if (paused != 0)
		{
			return true; // exit early, keep searching
		}


		entprops->GetEntProp(index, Prop_Send, "m_nSetupTimeLength", setuplength);

		if (setuplength > 0)
		{
			setup = true;
			return false; // end search
		}


		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("team_round_timer", functor);

#if defined(EXT_DEBUG) && defined(TF_DLL)
	ConColorMsg(Color(64, 200, 0, 255), "CTeamFortress2Mod::CheckForSetup m_bInSetup = %s\n", setup ? "TRUE" : "FALSE");
#endif // defined(EXT_DEBUG) && defined(TF_DLL)

	m_bInSetup = setup;

	if (setup)
	{
		m_setupExpireTimer.Start(static_cast<float>(setuplength) + 5.0f);
	}
	else
	{
		m_setupExpireTimer.Invalidate();
	}
}

void CTeamFortress2Mod::UpdateObjectiveResource()
{
	int ref = UtilHelpers::FindEntityByNetClass(-1, "CTFObjectiveResource");

	if (ref == INVALID_EHANDLE_INDEX)
	{
		if (m_cvar_debug->GetBool())
		{
			Warning("Failed to locate CTFObjectiveResource! \n");
		}
		return;
	}

	CBaseEntity* entity = gamehelpers->ReferenceToEntity(ref);

	if (entity == nullptr)
	{
		if (m_cvar_debug->GetBool())
		{
			Warning("Found reference %i but failed to retreive CBaseEntity pointer!\n", ref);
		}

		return;
	}

	m_objecteResourceEntity.Set(entity);

	m_objectiveResourcesData.m_iTimerToShowInHUD = entprops->GetPointerToEntData<int>(entity, Prop_Send, "m_iTimerToShowInHUD");
	m_objectiveResourcesData.m_iStopWatchTimer = entprops->GetPointerToEntData<int>(entity, Prop_Send, "m_iStopWatchTimer");
	m_objectiveResourcesData.m_bPlayingMiniRounds = entprops->GetPointerToEntData<bool>(entity, Prop_Send, "m_bPlayingMiniRounds");
	m_objectiveResourcesData.m_bCPIsVisible = entprops->GetPointerToEntData<int>(entity, Prop_Send, "m_bCPIsVisible");
	m_objectiveResourcesData.m_iPreviousPoints = entprops->GetPointerToEntData<int>(entity, Prop_Send, "m_iPreviousPoints");
	m_objectiveResourcesData.m_bTeamCanCap = entprops->GetPointerToEntData<bool>(entity, Prop_Send, "m_bTeamCanCap");
	m_objectiveResourcesData.m_bInMiniRound = entprops->GetPointerToEntData<bool>(entity, Prop_Send, "m_bInMiniRound");
	m_objectiveResourcesData.m_bCPLocked = entprops->GetPointerToEntData<bool>(entity, Prop_Send, "m_bCPLocked");
	m_objectiveResourcesData.m_iOwner = entprops->GetPointerToEntData<int>(entity, Prop_Send, "m_iOwner");

	{
		unsigned int offset = 0;
		entprops->HasEntProp(ref, Prop_Send, "m_iCPGroup", &offset);

		int offset_from_gd = 0;
		
		if (!extension->GetExtensionGameData()->GetOffset("CBaseTeamObjectiveResource::m_flCapPercentages", &offset_from_gd))
		{
			smutils->LogError(myself, "CBaseTeamObjectiveResource::m_flCapPercentages missing from gamedata file! Using hardcoded offset value!");

			offset_from_gd = static_cast<int>(sizeof(int[MAX_CONTROL_POINTS]));
		}

		m_objectiveResourcesData.m_flCapPercentages = entprops->GetPointerToEntData<float>(entity, offset + static_cast<unsigned int>(offset_from_gd));
	}

	if (m_cvar_debug->GetBool())
	{
		META_CONPRINTF("Found CTFObjectiveResource %p", UtilHelpers::textformat::FormatEntity(entity));
	}
}

bool CTeamFortress2Mod::TeamMayCapturePoint(int team, int pointindex) const
{
	if (m_objecteResourceEntity.Get() == nullptr)
	{
		return false;
	}

	int pointneeded = m_objectiveResourcesData.GetPreviousPointForPoint(pointindex, team, 0);

	if (pointneeded == pointindex || pointneeded == -1)
	{
		return true;
	}

	if (m_objectiveResourcesData.IsLocked(pointindex))
	{
		return false;
	}

	// Check if the team owns the previous points
	for (int prev = 0; prev < MAX_PREVIOUS_POINTS; prev++)
	{
		pointneeded = m_objectiveResourcesData.GetPreviousPointForPoint(pointindex, team, prev);

		if (pointneeded != -1)
		{
			if (m_objectiveResourcesData.GetOwner(pointneeded) != team)
			{
				return false;
			}
		}
	}

	return true;
}

void CTeamFortress2Mod::FindMvMBombHatchPosition()
{
	int capzone = UtilHelpers::FindEntityByClassname(INVALID_EHANDLE_INDEX, "func_capturezone");

	if (capzone == INVALID_EHANDLE_INDEX)
	{
		rootconsole->ConsolePrint("[NavBot] Mann vs Machine: Failed to find the bomb hatch position! (func_capturezone)");
	}

	CBaseEntity* entity = gamehelpers->ReferenceToEntity(capzone);
	m_MvMHatchPos = UtilHelpers::getWorldSpaceCenter(entity);

	if (m_cvar_debug->GetBool())
	{
		rootconsole->ConsolePrint("[NavBot] Debug: MvM Bomb Hatch Pos: %3.2f %3.2f %3.2f", m_MvMHatchPos.x, m_MvMHatchPos.y, m_MvMHatchPos.z);
	}
}

void CTeamFortress2Mod::FindGameModeSpecifics()
{
	switch (m_gamemode)
	{
	case TeamFortress2::GameModeType::GM_TOW:
	{
		int iEnt = UtilHelpers::FindEntityByClassname(INVALID_EHANDLE_INDEX, "trigger_capture_area");

		if (iEnt != INVALID_EHANDLE_INDEX)
		{
			m_towGoalEntity = gamehelpers->ReferenceToEntity(iEnt);
		}

		break;
	}
	default:
		break;
	}
}

bool CTeamFortress2Mod::IsAllowedToChangeClasses() const
{
	if (!GetModSettings()->IsAllowedToChangeClasses())
	{
		return false;
	}

	switch (m_gamemode)
	{
	case TeamFortress2::GameModeType::GM_MVM:
	{
		if (entprops->GameRules_GetRoundState() == RoundState::RoundState_RoundRunning)
		{
			return false;
		}

		break;
	}
	case TeamFortress2::GameModeType::GM_VSFFA:
	{
		return false;
	}
	default:
		break;
	}

	return true;
}

bool CTeamFortress2Mod::ShouldSwitchClass(CTF2Bot* bot) const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CTeamFortress2Mod::ShouldSwitchClass", "NavBot");
#endif // EXT_VPROF_ENABLED


	std::string classname(m_cvar_forceclass->GetString());
	auto forcedclass = tf2lib::GetClassTypeFromName(classname);

	if (forcedclass != TeamFortress2::TFClassType::TFClass_Unknown)
	{
		return forcedclass != bot->GetMyClassType();
	}

	if (m_classselector.IsClassAboveLimit(bot->GetMyClassType(), bot->GetMyTFTeam(), GetRosterForTeam(bot->GetMyTFTeam())) ||
		m_classselector.AnyPriorityClass(bot->GetMyTFTeam(), GetRosterForTeam(bot->GetMyTFTeam())))
	{
		return true;
	}

	return false;
}

TeamFortress2::TFClassType CTeamFortress2Mod::SelectAClassForBot(CTF2Bot* bot) const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CTeamFortress2Mod::SelectAClassForBot", "NavBot");
#endif // EXT_VPROF_ENABLED

	std::string classname(m_cvar_forceclass->GetString());
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
	if (IsPlayingMedievalMode())
	{
		return CTF2ClassSelection::ClassRosterType::ROSTER_MEDIEVAL;
	}

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
	case TeamFortress2::GameModeType::GM_PASSTIME:
		return CTF2ClassSelection::ClassRosterType::ROSTER_PASSTIME;
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
	CBaseMod::OnRoundStart();

	m_isTruceActive = false;
	librandom::ReSeedGlobalGenerators();
	UpdateObjectiveResource(); // call this first
	FindControlPoints(); // this must be before findpayloadcarts
	FindPayloadCarts();
	FindGameModeSpecifics();

	auto func = [](CBaseBot* bot) {
		bot->OnRoundStateChanged();
	};

	extmanager->ForEachBot(func);
}

const TeamFortress2::TFObjectiveResource* CTeamFortress2Mod::GetTFObjectiveResource() const
{
	if (m_objecteResourceEntity.Get() != nullptr)
	{
		return &m_objectiveResourcesData;
	}

	return nullptr;
}

void CTeamFortress2Mod::CollectControlPointsToAttack(TeamFortress2::TFTeam tfteam, std::vector<CBaseEntity*>& out)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CTeamFortress2Mod::CollectControlPointsToAttack", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (m_objecteResourceEntity.Get() == nullptr)
	{
		return;
	}

	int team = static_cast<int>(tfteam);
	bool minirounds = m_objectiveResourcesData.IsPlayingMiniRounds();

	for (auto& handle : m_controlpoints)
	{
		CBaseEntity* entity = handle.Get();

		if (entity == nullptr)
		{
			continue;
		}

		tfentities::HTeamControlPoint point(entity);
		int index = point.GetPointIndex();

		if (m_objectiveResourcesData.IsLocked(index))
		{
			continue;
		}

		if (!m_objectiveResourcesData.TeamCanCapPoint(index, team))
		{
			continue;
		}

		if (m_objectiveResourcesData.GetOwner(index) == team)
		{
			continue;
		}

		if (minirounds && !m_objectiveResourcesData.InMiniRound(index))
		{
			continue;
		}

		if (TeamMayCapturePoint(team, index))
		{
			out.push_back(entity);
		}
	}
}

void CTeamFortress2Mod::CollectControlPointsToDefend(TeamFortress2::TFTeam tfteam, std::vector<CBaseEntity*>& out)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CTeamFortress2Mod::CollectControlPointsToDefend", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (m_objecteResourceEntity.Get() == nullptr)
	{
		return;
	}

	// collect points the enemy team can capture
	int team = static_cast<int>(tf2lib::GetEnemyTFTeam(tfteam));
	int myteam = static_cast<int>(tfteam);
	bool minirounds = m_objectiveResourcesData.IsPlayingMiniRounds();

	for (auto& handle : m_controlpoints)
	{
		CBaseEntity* entity = handle.Get();

		if (entity == nullptr)
		{
			continue;
		}

		tfentities::HTeamControlPoint point(entity);
		int index = point.GetPointIndex();

		if (m_objectiveResourcesData.IsLocked(index))
		{
			continue;
		}

		if (!m_objectiveResourcesData.TeamCanCapPoint(index, team))
		{
			continue;
		}

		// Only defend control points owned by my team
		if (m_objectiveResourcesData.GetOwner(index) != myteam)
		{
			continue;
		}

		if (minirounds && !m_objectiveResourcesData.InMiniRound(index))
		{
			continue;
		}

		if (TeamMayCapturePoint(team, index))
		{
			out.push_back(entity);
		}
	}
}

CBaseEntity* CTeamFortress2Mod::FindNeutralControlPoint(const bool allowLocked) const
{
	if (m_objecteResourceEntity.Get() == nullptr)
	{
		return nullptr;
	}

	bool minirounds = m_objectiveResourcesData.IsPlayingMiniRounds();

	for (auto& handle : m_controlpoints)
	{
		CBaseEntity* entity = handle.Get();

		if (entity == nullptr)
		{
			continue;
		}

		tfentities::HTeamControlPoint point(entity);
		int index = point.GetPointIndex();

		if (!allowLocked && m_objectiveResourcesData.IsLocked(index))
		{
			continue;
		}

		if (minirounds && !m_objectiveResourcesData.InMiniRound(index))
		{
			continue;
		}

		if (m_objectiveResourcesData.GetOwner(index) <= static_cast<int>(TeamFortress2::TFTeam::TFTeam_Spectator))
		{
			return entity;
		}
	}

	return nullptr;
}

CBaseEntity* CTeamFortress2Mod::GetControlPointByIndex(const int index) const
{
	for (auto& handle : m_controlpoints)
	{
		if (handle.Get() == nullptr)
		{
			continue;
		}

		tfentities::HTeamControlPoint controlpoint(handle.Get());

		if (index == controlpoint.GetPointIndex())
		{
			return handle.Get();
		}
	}

	return nullptr;
}

CBaseEntity* CTeamFortress2Mod::GetRoundTimer(TeamFortress2::TFTeam team) const
{
	if (m_gamemode == TeamFortress2::GameModeType::GM_KOTH)
	{
		int entindex = INVALID_EHANDLE_INDEX;

		switch (team)
		{
		case TeamFortress2::TFTeam_Red:
			entprops->GameRules_GetPropEnt("m_hRedKothTimer", entindex);
			break;
		case TeamFortress2::TFTeam_Blue:
			entprops->GameRules_GetPropEnt("m_hBlueKothTimer", entindex);
			break;
		default:
			break;
		}

		if (entindex == INVALID_EHANDLE_INDEX)
		{
			return nullptr;
		}

		return gamehelpers->ReferenceToEntity(entindex);
	}

	if (m_objecteResourceEntity.Get() == nullptr)
	{
		return nullptr;
	}

	int entindex = m_objectiveResourcesData.GetTimerToShowInHUD();

	if (entindex > 0)
	{
		CBaseEntity* entity = gamehelpers->ReferenceToEntity(entindex);

		// the objective resource 'm_iTimerToShowInHUD' property is a raw entity index which may point to a deleted timer entity.
		// then another entity can take the timer index and we return an entity that isn't a timer.
		// this checks if the entity is actually a timer entity.
		if (entity && entprops->HasEntProp(entity, Prop_Send, "m_flTimerEndTime", nullptr))
		{
			return entity;
		}

		return nullptr;
	}

	return nullptr;
}

void CTeamFortress2Mod::DebugInfo_ControlPoints()
{
	if (m_objecteResourceEntity.Get() == nullptr)
	{
		Warning("Objective Resource is invalid! \n");
		return;
	}

	bool minirounds = m_objectiveResourcesData.IsPlayingMiniRounds();

	Msg("Is playing mini rounds: %s \n", minirounds ? "YES" : "NO");

	for (auto& handle : m_controlpoints)
	{
		CBaseEntity* pPoint = handle.Get();

		if (pPoint == nullptr)
		{
			continue;
		}

		tfentities::HTeamControlPoint controlpoint(pPoint);
		int index = controlpoint.GetPointIndex();

		std::unique_ptr<char[]> targetname = std::make_unique<char[]>(128);
		controlpoint.GetTargetName(targetname.get(), 128);

		bool locked = m_objectiveResourcesData.IsLocked(index);
		bool canredcap = m_objectiveResourcesData.TeamCanCapPoint(index, 2);
		bool canblucap = m_objectiveResourcesData.TeamCanCapPoint(index, 3);
		bool redmaycap = TeamMayCapturePoint(2, index);
		bool blumaycap = TeamMayCapturePoint(3, index);
		int owner = m_objectiveResourcesData.GetOwner(index);

		Msg("Control Point Data for point #%i (%i) <%s> \n", handle.GetEntryIndex(), index, targetname.get());
		Msg("    Locked: %s \n", locked ? "YES" : "NO");
		Msg("    Can RED capture: %s \n", canredcap ? "YES" : "NO");
		Msg("    Can BLU capture: %s \n", canblucap ? "YES" : "NO");
		Msg("    RED may capture: %s \n", redmaycap ? "YES" : "NO");
		Msg("    BLU may capture: %s \n", blumaycap ? "YES" : "NO");
		Msg("    Owner: %i \n", owner);

		if (minirounds)
		{
			bool inminiround = m_objectiveResourcesData.InMiniRound(index);
			Msg("    In Mini Round: %s \n", inminiround ? "YES" : "NO");
		}
	}

	std::vector<CBaseEntity*> points;
	CollectControlPointsToAttack(TeamFortress2::TFTeam::TFTeam_Red, points);

	for (CBaseEntity* ent : points)
	{
		tfentities::HTeamControlPoint controlpoint(ent);

		Msg("RED can attack control point of index %i \n", controlpoint.GetPointIndex());
	}

	points.clear();
	CollectControlPointsToDefend(TeamFortress2::TFTeam::TFTeam_Red, points);

	for (CBaseEntity* ent : points)
	{
		tfentities::HTeamControlPoint controlpoint(ent);

		Msg("RED can defend control point of index %i \n", controlpoint.GetPointIndex());
	}

	points.clear();
	CollectControlPointsToAttack(TeamFortress2::TFTeam::TFTeam_Blue, points);

	for (CBaseEntity* ent : points)
	{
		tfentities::HTeamControlPoint controlpoint(ent);

		Msg("BLU can attack control point of index %i \n", controlpoint.GetPointIndex());
	}

	points.clear();
	CollectControlPointsToDefend(TeamFortress2::TFTeam::TFTeam_Blue, points);

	for (CBaseEntity* ent : points)
	{
		tfentities::HTeamControlPoint controlpoint(ent);

		Msg("BLU can defend control point of index %i \n", controlpoint.GetPointIndex());
	}
}

void CTeamFortress2Mod::Command_ShowControlPoints() const
{
	for (auto& ehandle : m_controlpoints)
	{
		CBaseEntity* pEntity = ehandle.Get();

		if (pEntity == nullptr)
			continue;

		tfentities::HTeamControlPoint cp(pEntity);

		Msg("Control Point #%i\n", cp.GetPointIndex());

		std::string printname = cp.GetPrintName();

		if (!printname.empty())
		{
			Msg("    Display Name: %s\n", printname.c_str());
		}

		char targetname[512]{};
		cp.GetTargetName(targetname, sizeof(targetname));

		Msg("    Target Name: %s\n", targetname);
	}
}

void CTeamFortress2Mod::OnClientCommand(edict_t* pEdict, SourceMod::IGamePlayer* player, const CConCommandArgs& args)
{
	if (args.Count() > 2)
	{
		if (args.IsCommand("voicemenu"))
		{
			int command = TeamFortress2::GetVoiceCommandID(atoi(args[1]), atoi(args[2]));
			CBaseEntity* pEntity = pEdict->GetIServerEntity()->GetBaseEntity();

			auto func = [&pEntity, &command](CBaseBot* bot) {
				// send voice commands for all bots
				if (bot->GetEntity() != pEntity)
				{
					bot->OnVoiceCommand(pEntity, command);
				}
			};

			extmanager->ForEachBot(func);
		}
	}
}

const bool CTeamFortress2Mod::IsPlayingMedievalMode() const
{
	bool value = false;
	entprops->GameRules_GetPropBool("m_bPlayingMedieval", value);
	return value;
}

bool CTeamFortress2Mod::IsLineOfFireClear(const Vector& from, const Vector& to, CBaseEntity* passEnt) const
{
	CTraceFilterWorldAndPropsOnly filter;
	trace_t result;
	trace::line(from, to, MASK_SOLID, &filter, result);
	return !result.DidHit();
}

void CTeamFortress2Mod::OnPostInit()
{
	m_classselector.LoadClassSelectionData();
}

void CTeamFortress2Mod::RegisterModCommands()
{
	CServerCommandManager& manager = extmanager->GetServerCommandManager();

	auto showupgrades = [](const CConCommandArgs& args) {
		CTeamFortress2Mod::GetTF2Mod()->GetMvMUpgradeManager().ConCommand_ListUpgrades();
	};

	auto reloadupgrades = [](const CConCommandArgs& args) {
		CTeamFortress2Mod::GetTF2Mod()->ReloadUpgradeManager();
		rootconsole->ConsolePrint("Reloaded MvM Upgrades.");
	};

	auto debugcps = [](const CConCommandArgs& args) {
		CTeamFortress2Mod::GetTF2Mod()->DebugInfo_ControlPoints();
	};

	auto listcps = [](const CConCommandArgs& args) {
		CTeamFortress2Mod::GetTF2Mod()->Command_ShowControlPoints();
	};

	manager.RegisterConCommand("sm_navbot_tf_show_upgrades", "[TF2] List all MvM upgrades registered.", FCVAR_GAMEDLL, showupgrades);
	manager.RegisterConCommand("sm_navbot_tf_reload_upgrades", "[TF2] Reload MvM Upgrades.", FCVAR_GAMEDLL, reloadupgrades);
	manager.RegisterConCommand("sm_navbot_tf_debug_control_points", "[TF2] Show control point data.", FCVAR_GAMEDLL | FCVAR_CHEAT, debugcps);
	manager.RegisterConCommand("sm_navbot_tf_list_control_points", "[TF2] Shows a list of control points on this map.", FCVAR_GAMEDLL | FCVAR_CHEAT, listcps);

	CTF2BotSensor::RegisterTF2ConVars();
}
