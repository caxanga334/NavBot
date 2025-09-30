#include NAVBOT_PCH_FILE
#include <extension.h>
#include <manager.h>
#include <util/helpers.h>
#include <bot/basebot.h>
#include <bot/interfaces/knownentity.h>
#include <bot/pluginbot/pluginbot.h>
#include <mods/basemod.h>
#include "bots.h"

// gets a CBaseBot instance. Throws plugin errors if invalid
#define METHODMAP_GETVALIDBOT														\
	int client = static_cast<int>(params[1]);										\
																					\
	if (!UtilHelpers::IsPlayerIndex(client))										\
	{																				\
		context->ReportError("%i is not a valid client index!", client);			\
		return 0;																	\
	}																				\
																					\
	IGamePlayer* player = playerhelpers->GetGamePlayer(client);						\
																					\
	if (player == nullptr)															\
	{																				\
		context->ReportError("%i is not a valid player instance!", client);			\
		return 0;																	\
	}																				\
																					\
	if (!player->IsInGame())														\
	{																				\
		context->ReportError("%i is not in game!", client);							\
		return 0;																	\
	}																				\
																					\
	CBaseBot* bot = extmanager->GetBotByIndex(client);								\
																					\
	if (bot == nullptr)																\
	{																				\
		context->ReportError("%i is not a valid NavBot instance!", client);			\
		return 0;																	\
	}																				\
																					\

void natives::bots::setup(std::vector<sp_nativeinfo_t>& nv)
{
	sp_nativeinfo_t list[] = {
		{"NavBot.NavBot", AddNavBotMM},
		{"NavBot.IsPluginBot", IsPluginBot},
		{"NavBot.SetSkillLevel", SetSkillLevel},
		{"PluginBot.PluginBot", AttachNavBot},
		{"GetNavBotByIndex", GetNavBotByIndex},
		{"NavBot.DelayedFakeClientCommand", DelayedFakeClientCommand},
		{"NavBot.SelectWeapon", SelectWeapon},
	};

	nv.insert(nv.end(), std::begin(list), std::end(list));
}

cell_t natives::bots::AddNavBotMM(IPluginContext* context, const cell_t* params)
{
	edict_t* edict = nullptr;
	char* szName = nullptr;

	context->LocalToStringNULL(params[1], &szName);

	if (szName != nullptr)
	{
		std::string name(szName);

		extmanager->AddBot(&name, &edict);
	}
	else
	{
		extmanager->AddBot(nullptr, &edict);
	}

	if (edict == nullptr)
	{
		return 0;
	}

	CBaseBot* bot = extmanager->GetBotByIndex(gamehelpers->IndexOfEdict(edict));

	return bot->GetIndex();
}

cell_t natives::bots::AttachNavBot(IPluginContext* context, const cell_t* params)
{
	int client = static_cast<int>(params[1]);

	if (!UtilHelpers::IsPlayerIndex(client))
	{
		context->ReportError("%i is not a valid client index!", client);
		return 0;
	}

	IGamePlayer* player = playerhelpers->GetGamePlayer(client);

	if (player == nullptr)
	{
		context->ReportError("%i is not a valid player instance!", client);
		return 0;
	}

	if (!player->IsInGame())
	{
		context->ReportError("%i is not in game!", client);
		return 0;
	}

	if (player->IsSourceTV() || player->IsReplay())
	{
		context->ReportError("Cannot attach NavBot to SourceTV/Replay bots!");
		return 0;
	}

	CBaseBot* bot = extmanager->GetBotByIndex(client);

	if (bot != nullptr)
	{
		if (!bot->IsPluginBot())
		{
			return 0; // return NULL for non plugin bots.
		}

		// bot already exists, return the index
		return static_cast<cell_t>(bot->GetIndex()); // sanity, should be the same as the param passed from the plugin.
	}

	edict_t* entity = gamehelpers->EdictOfIndex(client);

	if (entity == nullptr)
	{
		context->ReportError("Got NULL edict_t for client %i", client);
		return 0;
	}

	bot = extmanager->AttachBotInstanceToEntity(entity);

	if (bot == nullptr)
	{
		return 0;
	}

	smutils->LogMessage(myself, "Attached NavBot CPluginBot instance to entity #%i.", client);

	return static_cast<cell_t>(bot->GetIndex());
}

cell_t natives::bots::IsPluginBot(IPluginContext* context, const cell_t* params)
{
	METHODMAP_GETVALIDBOT;

	return bot->IsPluginBot() ? 1 : 0;
}

cell_t natives::bots::SetSkillLevel(IPluginContext* context, const cell_t* params)
{
	METHODMAP_GETVALIDBOT;

	int skill = static_cast<int>(params[2]);

	auto profile = extmanager->GetMod()->GetBotDifficultyManager()->GetProfileForSkillLevel(skill);

	bot->SetDifficultyProfile(profile);

	return 0;
}

cell_t natives::bots::GetNavBotByIndex(IPluginContext* context, const cell_t* params)
{
	int client = static_cast<int>(params[1]);

	if (!UtilHelpers::IsPlayerIndex(client))
	{
		context->ReportError("%i is not a valid client index!", client);
		return 0;
	}

	IGamePlayer* player = playerhelpers->GetGamePlayer(client);

	if (player == nullptr)
	{
		context->ReportError("%i is not a valid player instance!", client);
		return 0;
	}

	if (!player->IsInGame())
	{
		context->ReportError("%i is not in game!", client);
		return 0;
	}

	CBaseBot* bot = extmanager->GetBotByIndex(client);

	if (bot == nullptr)
	{
		return 0;
	}

	return static_cast<cell_t>(bot->GetIndex());
}

cell_t natives::bots::SetRunPlayerCommands(IPluginContext* context, const cell_t* params)
{
	METHODMAP_GETVALIDBOT;

	if (!bot->IsPluginBot())
	{
		context->ReportError("%i Not a PluginBot instance!", client);
		return 0;
	}

	if (!player->IsFakeClient())
	{
		context->ReportError("SetRunPlayerCommands can only be used on fake clients! %i is a human client!", client);
		return 0;
	}

	if (bot->GetController() == nullptr)
	{
		context->ReportError("Can't enable player commands for %i. NULL IBotController!", client);
		return 0;
	}

	CPluginBot* pluginbot = static_cast<CPluginBot*>(bot);

	bool enablecommands = params[2] != 0;
	pluginbot->SetRunPlayerCommands(enablecommands);

	return 0;
}

cell_t natives::bots::DelayedFakeClientCommand(IPluginContext* context, const cell_t* params)
{
	METHODMAP_GETVALIDBOT;

	char* command = nullptr;
	context->LocalToStringNULL(params[2], &command);

	if (!command)
	{
		context->ReportError("Command cannot be a NULL string!");
		return 0;
	}

	if (std::strlen(command) < 1)
	{
		context->ReportError("Command string length == 0!");
		return 0;
	}

	bot->DelayedFakeClientCommand(command);
	return 0;
}

cell_t natives::bots::SelectWeapon(IPluginContext* context, const cell_t* params)
{
	METHODMAP_GETVALIDBOT;

	CBaseEntity* entity = gamehelpers->ReferenceToEntity(params[2]);

	if (!entity)
	{
		context->ReportError("Invalid weapon entity of index %i!", static_cast<int>(params[2]));
		return 0;
	}

	return static_cast<cell_t>(bot->SelectWeapon(entity));
}
