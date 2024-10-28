#include <filesystem>
#include <algorithm>
#include <extension.h>
#include "badplugins.h"


// from Sourcemod bridge\include\IScriptManager.h
// evil hack
class SMPlugin : public IPlugin
{
public:
	virtual size_t GetConfigCount() = 0;
	virtual void* GetConfig(size_t i) = 0;
	virtual void AddLibrary(const char* name) = 0;
	virtual void AddConfig(bool create, const char* cfg, const char* folder) = 0;
	virtual void EvictWithError(PluginStatus status, const char* fmt, ...) = 0;
};

CBadPluginChecker::CBadPluginChecker()
{
	parser_depth = 0;
	current = nullptr;
}

CBadPluginChecker::~CBadPluginChecker()
{
}

bool CBadPluginChecker::LoadConfigFile(char* error, size_t maxlen)
{
	char path[PLATFORM_MAX_PATH]{};

	smutils->BuildPath(SourceMod::Path_SM, path, sizeof(path), "configs/navbot/bad_plugins.cfg");

	if (std::filesystem::exists(path) == false)
	{
		ke::SafeSprintf(error, maxlen, "Failed to parse \"%s\". File does not exists!", path);
		return false;
	}

	SourceMod::SMCStates states;
	auto parse_result = textparsers->ParseFile_SMC(path, this, &states);

	if (parse_result != SourceMod::SMCError_Okay)
	{
		ke::SafeSprintf(error, maxlen, "Failed to parse \"%s\". Line: %i Col: %i", path, states.line, states.col);
		return false;
	}

	RemoveBadEntries();

	return true;
}

void CBadPluginChecker::ValidatePlugin(IPlugin* plugin) const
{
	if (cfg_entries.empty())
	{
		return;
	}

	if (plugin->GetStatus() != SourceMod::Plugin_Loaded)
	{
		return;
	}

	if (plugin->GetFilename() == nullptr)
	{
		return;
	}

	std::filesystem::path pluginPath{ plugin->GetFilename() };

	pluginPath = pluginPath.replace_extension(); // removes the .smx extension
	pluginPath = pluginPath.filename(); // remove folders if any
	std::string pluginName = pluginPath.string();

	std::transform(pluginName.begin(), pluginName.end(), pluginName.begin(), [](unsigned char c) {
		return std::tolower(c);
	});

	for (auto& entry : cfg_entries)
	{
		switch (entry.matchType)
		{
		case MATCH_FULL:
		{
			if (pluginName == entry.matchString)
			{
				TakeActionAgainstPlugin(plugin, entry.action);
				return;
			}
			break;
		}
		
		case MATCH_PARTIAL:
		default:
		{
			if (pluginName.find(entry.matchString) != std::string::npos)
			{
				TakeActionAgainstPlugin(plugin, entry.action);
				return;
			}

			break;
		}
		}
	}
}

void CBadPluginChecker::TakeActionAgainstPlugin(IPlugin* plugin, Action action) const
{
	switch (action)
	{
	case CBadPluginChecker::ACTION_LOG:
	{
		auto info = plugin->GetPublicInfo();
		smutils->LogError(myself, "Plugin %s <%s> is known to have issues with NavBot, please remove it.", info->name, plugin->GetFilename());
	}
		break;
	case CBadPluginChecker::ACTION_EVICT:
	default:
	{
		auto info = plugin->GetPublicInfo();
		SMPlugin* evil = reinterpret_cast<SMPlugin*>(plugin);
		evil->EvictWithError(SourceMod::Plugin_Error, "Plugin %s <%s> is known to have issues with NavBot and was blocked.", info->name, plugin->GetFilename());
	}
	}
}

SourceMod::SMCResult CBadPluginChecker::ReadSMC_NewSection(const SourceMod::SMCStates* states, const char* name)
{
	if (strncasecmp(name, "BadPlugins", 10) == 0)
	{
		parser_depth++;
		return SourceMod::SMCResult_Continue;
	}

	if (parser_depth == 1) // we are inside the badplugins section
	{
		parser_depth++;
		auto& entry = cfg_entries.emplace_back();
		current = &entry;

		current->entryName.assign(name);
	}

	return SourceMod::SMCResult_Continue;
}

SourceMod::SMCResult CBadPluginChecker::ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value)
{
	if (parser_depth != 2) // there shouldn't be any KV outside of depth 2
	{
		return SourceMod::SMCResult_HaltFail;
	}

	if (strncasecmp(key, "action_type", 11) == 0)
	{
		if (strncasecmp(value, "log", 3) == 0)
		{
			current->action = ACTION_LOG;
		}
		else if (strncasecmp(value, "evict", 5) == 0)
		{
			current->action = ACTION_EVICT;
		}
		else
		{
			smutils->LogError(myself, "Unknown value for \"action_type\" at line %d col %d", states->line, states->col);
		}
	}
	else if (strncasecmp(key, "match_type", 10) == 0)
	{
		if (strncasecmp(value, "full", 4) == 0)
		{
			current->matchType = MATCH_FULL;
		}
		else if (strncasecmp(value, "partial", 7) == 0)
		{
			current->matchType = MATCH_PARTIAL;
		}
		else
		{
			smutils->LogError(myself, "Unknown value for \"match_type\" at line %d col %d", states->line, states->col);
		}
	}
	else if (strncasecmp(key, "match_string", 10) == 0)
	{
		current->matchString.assign(value);
		std::transform(current->matchString.begin(), current->matchString.end(), current->matchString.begin(), [](unsigned char c) {
			return std::tolower(c);
		});
	}

	return SourceMod::SMCResult_Continue;
}

SourceMod::SMCResult CBadPluginChecker::ReadSMC_LeavingSection(const SourceMod::SMCStates* states)
{
	if (parser_depth == 2)
	{
		current = nullptr;
	}

	parser_depth--;
	return SourceMod::SMCResult_Continue;
}

void CBadPluginChecker::RemoveBadEntries()
{
	cfg_entries.erase(std::remove_if(cfg_entries.begin(), cfg_entries.end(), [](const CBadPluginChecker::ConfigEntry& object) {
		if (object.entryName.empty() || object.matchString.empty())
		{
			return true;
		}

		return false;
	}), cfg_entries.end());
}
