#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/pawnutils.h>
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

static cell_t Native_ParseWeaponConfigFile(IPluginContext* context, const cell_t* params)
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

static cell_t Native_GetCurrentMapName(IPluginContext* context, const cell_t* params)
{
	if (params[1] < static_cast<cell_t>(Mods::MapNameType::MAPNAME_RAW) || params[1] > static_cast<cell_t>(Mods::MapNameType::MAPNAME_UNIQUE))
	{
		context->ReportError("Invalid map name type!");
		return 0;
	}

	Mods::MapNameType type = static_cast<Mods::MapNameType>(params[1]);
	std::string mapname = extmanager->GetMod()->GetCurrentMapName(type);
	context->StringToLocal(params[2], static_cast<size_t>(params[3]), mapname.c_str());
	return 0;
}

static cell_t Native_GetModFolder(IPluginContext* context, const cell_t* params)
{
	const std::string& folder = extmanager->GetMod()->GetModFolder();
	context->StringToLocal(params[1], static_cast<size_t>(params[2]), folder.c_str());
	return 0;
}

static cell_t Native_GetFallbackModFolder(IPluginContext* context, const cell_t* params)
{
	const std::string& folder = extmanager->GetMod()->GetModFallbackFolder();
	
	if (folder.empty())
	{
		char buffer[32];
		ke::SafeSprintf(buffer, sizeof(buffer), "STOP_IGNORING_RETVALS");
		context->StringToLocal(params[1], static_cast<size_t>(params[2]), buffer);
	}
	else
	{
		context->StringToLocal(params[1], static_cast<size_t>(params[2]), folder.c_str());
	}

	return pawnutils::ReturnBool((!folder.empty()));
}

static cell_t Native_AddClassnameToNPCScanList(IPluginContext* context, const cell_t* params)
{
	char* str = nullptr;
	context->LocalToString(params[1], &str);
	extmanager->GetMod()->AddNPCClassnameToList(str);
	return 0;
}

static cell_t Native_RemoveClassnameFromNPCScanList(IPluginContext* context, const cell_t* params)
{
	char* str = nullptr;
	context->LocalToString(params[1], &str);
	extmanager->GetMod()->RemoveNPCClassnameFromList(str);
	return 0;
}

namespace sbm
{
	static inline bool IsValidTeamIndex(IPluginContext* context, int teamIndex)
	{
		if (teamIndex >= 0 && teamIndex < MAX_TEAMS) { return true; }
		context->ReportError("Team index %i is out of bounds!", teamIndex);
		return false;
	}

	static cell_t Native_ReportEntity(IPluginContext* context, const cell_t* params)
	{
		int teamIndex = static_cast<int>(params[1]);

		if (!IsValidTeamIndex(context, teamIndex)) { return 0; }

		CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(params[2]);

		if (!pEntity)
		{
			context->ReportError("NULL entity (%i)", params[2]);
			return 0;
		}

		if (!UtilHelpers::IsEntNetworkable(pEntity))
		{
			context->ReportError("Cannot report server side only entity %s!", UtilHelpers::textformat::FormatEntity(pEntity));
			return 0;
		}

		ISharedBotMemory* sbm = extmanager->GetMod()->GetSharedBotMemory(teamIndex);
		sbm->ReportEntityVisible(pEntity);
		return 0;
	}

	static cell_t Native_ForgetEntity(IPluginContext* context, const cell_t* params)
	{
		int teamIndex = static_cast<int>(params[1]);

		if (!IsValidTeamIndex(context, teamIndex)) { return 0; }

		CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(params[2]);

		if (!pEntity)
		{
			context->ReportError("NULL entity (%i)", params[2]);
			return 0;
		}

		if (!UtilHelpers::IsEntNetworkable(pEntity))
		{
			context->ReportError("Cannot forget server side only entity %s!", UtilHelpers::textformat::FormatEntity(pEntity));
			return 0;
		}

		ISharedBotMemory* sbm = extmanager->GetMod()->GetSharedBotMemory(teamIndex);
		sbm->ForgetEntity(pEntity);
		return 0;
	}
}

void natives::mods::setup(std::vector<sp_nativeinfo_t>& nv)
{
	sp_nativeinfo_t list[] = {
		/* CBaseMod */
		{"NavBotModInterface.GetModName", Native_GetModName},
		{"NavBotModInterface.ParseModSettingsFile", Native_ParseModSettingsFile},
		{"NavBotModInterface.ParseDifficultyProfileFile", Native_ParseDifficultyProfileFile},
		{"NavBotModInterface.ParseWeaponConfigFile", Native_ParseWeaponConfigFile},
		{"NavBotModInterface.GetCurrentMapName", Native_GetCurrentMapName},
		{"NavBotModInterface.GetModFolder", Native_GetModFolder},
		{"NavBotModInterface.GetFallbackModFolder", Native_GetFallbackModFolder},
		{"NavBotModInterface.AddClassnameToNPCScanList", Native_AddClassnameToNPCScanList},
		{"NavBotModInterface.RemoveClassnameFromNPCScanList", Native_RemoveClassnameFromNPCScanList},

		/* ISharedBotMemory */
		{"NavBotSharedBotMemory.ReportEntity", sbm::Native_ReportEntity},
		{"NavBotSharedBotMemory.ForgetEntity", sbm::Native_ForgetEntity},
	};

	nv.insert(nv.end(), std::begin(list), std::end(list));
}