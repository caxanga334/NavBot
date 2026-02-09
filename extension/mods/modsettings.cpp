#include NAVBOT_PCH_FILE
#include <extension.h>
#include <manager.h>
#include <util/librandom.h>
#include <coordsize.h>
#include "basemod.h"
#include "modsettings.h"

void CModSettings::ParseConfigFile()
{
	char path[PLATFORM_MAX_PATH]{};
	
	const CBaseMod* mod = extmanager->GetMod();
	const char* modfolder = mod->GetModFolder().c_str();

	// Parse the global file first if it exists
	smutils->BuildPath(SourceMod::Path_SM, path, PLATFORM_MAX_PATH, "configs/navbot/settings.custom.cfg");

	if (std::filesystem::exists(path))
	{
		ParseFile(path);
	}
	else
	{
		smutils->BuildPath(SourceMod::Path_SM, path, PLATFORM_MAX_PATH, "configs/navbot/settings.cfg");

		if (std::filesystem::exists(path))
		{
			ParseFile(path);
		}
	}

	// Then parse the per mod global if it exists
	smutils->BuildPath(SourceMod::Path_SM, path, PLATFORM_MAX_PATH, "configs/navbot/%s/settings.custom.cfg", modfolder);

	if (std::filesystem::exists(path))
	{
		ParseFile(path);
	}
	else
	{
		smutils->BuildPath(SourceMod::Path_SM, path, PLATFORM_MAX_PATH, "configs/navbot/%s/settings.cfg", modfolder);

		if (std::filesystem::exists(path))
		{
			ParseFile(path);
		}
	}

	std::string prefix;
	std::string cleanmap = mod->GetCurrentMapName(Mods::MapNameType::MAPNAME_CLEAN); // always use clean map names for prefix detection

	if (UtilHelpers::GetPrefixFromMapName(cleanmap, prefix))
	{
		// Finally, parse the per map override
		smutils->BuildPath(SourceMod::Path_SM, path, PLATFORM_MAX_PATH, "configs/navbot/%s/maps/settings.%s_.cfg", modfolder, prefix.c_str());

		if (std::filesystem::exists(path))
		{
			ParseFile(path);
		}
		else
		{
			META_CONPRINTF("[NavBot] Map prefix specific mod settings file at \"%s\" not found, not parsing. \n", path);
		}
	}

	// Finally, parse the per map override

	Mods::MapNameType type = Mods::MapNameType::MAPNAME_CLEAN;

	if (CExtManager::ModUsesWorkshopMaps())
	{
		if (CExtManager::ShouldPreferUniqueMapNames())
		{
			type = Mods::MapNameType::MAPNAME_UNIQUE;
		}
	}

	std::string map = mod->GetCurrentMapName(type);
	smutils->BuildPath(SourceMod::Path_SM, path, PLATFORM_MAX_PATH, "configs/navbot/%s/maps/settings.%s.cfg", modfolder, map.c_str());

	if (std::filesystem::exists(path))
	{
		ParseFile(path);
	}
	else
	{
		if (CExtManager::ModUsesWorkshopMaps())
		{
			std::string map = mod->GetCurrentMapName(Mods::InvertMapNameType(type));
			smutils->BuildPath(SourceMod::Path_SM, path, PLATFORM_MAX_PATH, "configs/navbot/%s/maps/settings.%s.cfg", modfolder, map.c_str());
		}

		META_CONPRINTF("[NavBot] Map specific mod settings file at \"%s\" not found, not parsing. \n", path);
	}

	PostParse();
}

void CModSettings::PostParse()
{
	if (rogue_min_time >= rogue_max_time)
	{
		rogue_min_time = rogue_max_time - 1.0f;
	}

	if (unstuck_teleport_threshold >= stucksuicidethreshold)
	{
		unstuck_teleport_threshold = -1;
	}

	if (class_change_chance <= 0)
	{
		allow_class_changes = false;
	}
	
	if (class_change_min_time >= class_change_max_time)
	{
		class_change_min_time = 60.0f;
		class_change_max_time = 180.0f;
	}

	if (camp_time_min >= camp_time_max)
	{
		camp_time_min = 15.0f;
		camp_time_max = 90.0f;
	}
}

SourceMod::SMCError CModSettings::ParseCustomFile(const char* file)
{
	SourceMod::SMCStates states;
	return textparsers->ParseFile_SMC(file, this, &states);
}

SourceMod::SMCResult CModSettings::ReadSMC_NewSection(const SourceMod::SMCStates* states, const char* name)
{
	if (cfg_parser_depth == 0)
	{
		if (strncasecmp(name, "ModSettings", 11) == 0)
		{
			cfg_parser_depth++;
		}
		else
		{
			smutils->LogError(myself, "Invalid mod settings file (%s) at line %i col %i", name, states->line, states->col);
			return SourceMod::SMCResult_HaltFail;
		}
	}

	return SourceMod::SMCResult_Continue;
}

SourceMod::SMCResult CModSettings::ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value)
{
	if (cfg_parser_depth == 1)
	{
		if (strncasecmp(key, "defend_rate", 11) == 0)
		{
			int v = atoi(value);
			v = std::clamp(v, 0, 100);
			SetDefendRate(v);
		}
		else if (strncasecmp(key, "stuck_suicide_threshold", 23) == 0)
		{
			int v = atoi(value);
			v = std::clamp(v, 5, 60);
			SetStuckSuicideThreshold(v);
		}
		else if (strncasecmp(key, "update_rate", 11) == 0)
		{
			float v = atof(value);
			v = std::clamp(v, 0.0f, 0.5f);
			SetUpdateRate(v);
		}
		else if (strncasecmp(key, "vision_npc_update_rate", 22) == 0)
		{
			float v = atof(value);
			v = std::clamp(v, 0.0f, 2.0f);
			SetVisionNPCUpdateRate(v);
		}
		else if (strncasecmp(key, "inventory_update_rate", 21) == 0)
		{
			float v = atof(value);
			v = std::clamp(v, 0.05f, 120.0f);
			SetInventoryUpdateRate(v);
		}
		else if (strncasecmp(key, "vision_statistics_update", 24) == 0)
		{
			float v = atof(value);
			v = std::clamp(v, 0.05f, 2.0f);
			SetVisionStatisticsUpdateRate(v);
		}
		else if (strncasecmp(key, "collect_item_max_distance", 25) == 0)
		{
			float v = atof(value);
			v = std::clamp(v, 2048.0f, 16384.0f);
			SetCollectItemMaxDistance(v);
		}
		else if (strncasecmp(key, "max_defend_distance", 19) == 0)
		{
			float v = atof(value);
			v = std::clamp(v, 1024.0f, MAX_COORD_FLOAT);
			SetMaxDefendDistance(v);
		}
		else if (strncasecmp(key, "max_sniper_distance", 19) == 0)
		{
			float v = atof(value);
			v = std::clamp(v, 1024.0f, MAX_COORD_FLOAT);
			SetMaxDefendDistance(v);
		}
		else if (strncasecmp(key, "rogue_chance", 12) == 0)
		{
			int v = atoi(value);
			v = std::clamp(v, 0, 100);
			SetRogueChance(v);
		}
		else if (strncasecmp(key, "rogue_max_time", 14) == 0)
		{
			float v = atof(value);
			v = std::clamp(v, 90.0f, 1200.0f);
			SetRogueMaxTime(v);
		}
		else if (strncasecmp(key, "rogue_min_time", 14) == 0)
		{
			float v = atof(value);
			v = std::clamp(v, 30.0f, 600.0f);
			SetRogueMinTime(v);
		}
		else if (strncasecmp(key, "movement_break_assist", 21) == 0)
		{
			SetBreakAssist(UtilHelpers::StringToBoolean(value));
		}
		else if (strncasecmp(key, "movement_jump_assist", 20) == 0)
		{
			SetAllowJumpAssist(UtilHelpers::StringToBoolean(value));
		}
		else if (strncasecmp(key, "unstuck_cheats", 14) == 0)
		{
			SetAllowUnstuckCheats(UtilHelpers::StringToBoolean(value));
		}
		else if (strncasecmp(key, "unstuck_teleport_threshold", 26) == 0)
		{
			int v = atoi(value);
			v = std::clamp(v, -1, 60);
			SetUnstuckTeleportThreshold(v);
		}
		else if (std::strcmp(key, "allow_class_changes") == 0)
		{
			SetAllowClassChanges(UtilHelpers::StringToBoolean(value));
		}
		else if (std::strcmp(key, "class_change_min_time") == 0)
		{
			float v = atof(value);
			v = std::clamp(v, 30.0f, 300.0f);
			SetMinClassChangeTime(v);
		}
		else if (std::strcmp(key, "class_change_max_time") == 0)
		{
			float v = atof(value);
			v = std::clamp(v, 60.0f, 1200.0f);
			SetMaxClassChangeTime(v);
		}
		else if (std::strcmp(key, "class_change_chance") == 0)
		{
			int v = atoi(value);
			v = std::clamp(v, 0, 100);
			SetChangeClassChance(v);
		}
		else if (std::strcmp(key, "camp_time_min") == 0)
		{
			float v = atof(value);
			v = std::clamp(v, 15.0f, 300.0f);
			SetCampMinTime(v);
		}
		else if (std::strcmp(key, "camp_time_max") == 0)
		{
			float v = atof(value);
			v = std::clamp(v, 30.0f, 1200.0f);
			SetCampMaxTime(v);
		}
		else if (std::strcmp(key, "breakable_max_health") == 0)
		{
			int v = atoi(value);
			v = std::clamp(v, 500, 10000);
			SetBreakableMaxHealth(v);
		}
		else
		{
			smutils->LogError(myself, "[MOD SETTINGS] Unknown Key Value pair (\"%s\"    \"%s\") at line %i col %i", key, value, states->line, states->col);
		}
	}

	return SourceMod::SMCResult_Continue;
}

SourceMod::SMCResult CModSettings::ReadSMC_LeavingSection(const SourceMod::SMCStates* states)
{
	if (--cfg_parser_depth < 0)
	{
		smutils->LogError(myself, "Invalid mod settings file at line %i col %i", states->line, states->col);
		return SourceMod::SMCResult_HaltFail;
	}

	return SourceMod::SMCResult_Continue;
}

void CModSettings::ParseFile(const char* file)
{
	META_CONPRINTF("[NavBot] Parsing mod settings file: %s \n", file);

	SourceMod::SMCStates states;
	SourceMod::SMCError result = textparsers->ParseFile_SMC(file, this, &states);

	if (result != SourceMod::SMCError::SMCError_Okay)
	{
		smutils->LogError(myself, "Error parsing mod settings file \"%s\"!", file);
	}
}

bool CModSettings::RollDefendChance() const
{
	if (defendrate >= randomgen->GetRandomInt<int>(1, 100))
	{
		return true;
	}

	return false;
}
