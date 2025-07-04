#include NAVBOT_PCH_FILE
#include <extension.h>
#include <manager.h>
#include <util/librandom.h>
#include "basemod.h"
#include "modsettings.h"

void CModSettings::ParseConfigFile()
{
	char path[PLATFORM_MAX_PATH]{};
	const char* modfolder = smutils->GetGameFolderName();
	const CBaseMod* mod = extmanager->GetMod();
	std::string map = mod->GetCurrentMapName();

	smutils->BuildPath(SourceMod::Path_SM, path, PLATFORM_MAX_PATH, "configs/navbot/%s/maps/settings.%s.cfg", modfolder, map.c_str());

	if (!std::filesystem::exists(path))
	{
		META_CONPRINTF("[NavBot] Map specific mod settings file at \"%s\" not found, not parsing. \n", path);

		smutils->BuildPath(SourceMod::Path_SM, path, PLATFORM_MAX_PATH, "configs/navbot/%s/settings.custom.cfg", modfolder);

		if (!std::filesystem::exists(path))
		{
			smutils->BuildPath(SourceMod::Path_SM, path, PLATFORM_MAX_PATH, "configs/navbot/%s/settings.cfg", modfolder);

			if (!std::filesystem::exists(path))
			{
				smutils->BuildPath(SourceMod::Path_SM, path, PLATFORM_MAX_PATH, "configs/navbot/settings.custom.cfg");

				if (!std::filesystem::exists(path))
				{
					smutils->BuildPath(SourceMod::Path_SM, path, PLATFORM_MAX_PATH, "configs/navbot/settings.cfg");

					if (!std::filesystem::exists(path))
					{
						return; // Use default values set by the constructor
					}
				}
			}
		}
	}

	META_CONPRINTF("[NavBot] Parsing mod settings file: %s \n", path);

	SourceMod::SMCStates states;
	SourceMod::SMCError result = textparsers->ParseFile_SMC(path, this, &states);

	if (result != SourceMod::SMCError::SMCError_Okay)
	{
		smutils->LogError(myself, "Error parsing mod settings file \"%s\"!", path);
	}
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

bool CModSettings::RollDefendChance() const
{
	if (defendrate >= randomgen->GetRandomInt<int>(1, 100))
	{
		return true;
	}

	return false;
}
