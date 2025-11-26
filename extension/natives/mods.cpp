#include NAVBOT_PCH_FILE
#include <extension.h>
#include <mods/basemod.h>
#include "mods.h"

static cell_t Native_GetModName(IPluginContext* context, const cell_t* params)
{
	char* buffer;
	context->LocalToString(params[1], &buffer);
	std::size_t size = static_cast<std::size_t>(params[2]);
	smutils->Format(buffer, size, "%s", extmanager->GetMod()->GetModName());
	return 0;
}

static cell_t Native_ParseModSettingsFile(IPluginContext* context, const cell_t* params)
{
	char* path;
	context->LocalToString(params[1], &path);

	if (!std::filesystem::exists(path))
	{
		context->ReportError("File \"%s\" does not exists!", path);
		return 0;
	}

	SourceMod::SMCError error = extmanager->GetMod()->ParseCustomModSettingsFile(path);

	return error == SourceMod::SMCError::SMCError_Okay ? 1 : 0;
}

static cell_t Native_ParseDifficultyProfileFile(IPluginContext* context, const cell_t* params)
{
	char* path;
	context->LocalToString(params[1], &path);

	if (!std::filesystem::exists(path))
	{
		context->ReportError("File \"%s\" does not exists!", path);
		return 0;
	}

	SourceMod::SMCError error = extmanager->GetMod()->ParseCustomBotDifficultyProfileFile(path, params[2] != 0, params[3] != 0);

	return error == SourceMod::SMCError::SMCError_Okay ? 1 : 0;
}

void natives::mods::setup(std::vector<sp_nativeinfo_t>& nv)
{
	sp_nativeinfo_t list[] = {
		{"NavBotModInterface.GetModName", Native_GetModName},
		{"NavBotModInterface.ParseModSettingsFile", Native_ParseModSettingsFile},
		{"NavBotModInterface.ParseDifficultyProfileFile", Native_ParseDifficultyProfileFile},
	};

	nv.insert(nv.end(), std::begin(list), std::end(list));
}