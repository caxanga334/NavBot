#include <extension.h>
#include <manager.h>
#include <util/helpers.h>
#include <bot/basebot.h>
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
		{"NavBot.NavBot", AttachNavBot},
		{"NavBot.IsPluginBot", IsPluginBot},
		{"GetNavBotByIndex", GetNavBotByIndex},
	};

	nv.insert(nv.end(), std::begin(list), std::end(list));
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

	CBaseBot* bot = extmanager->GetBotByIndex(client);

	if (bot != nullptr)
	{
		// bot already exists, return the index
		return params[1];
	}

	edict_t* entity = gamehelpers->EdictOfIndex(client);

	if (entity == nullptr)
	{
		context->ReportError("Got NULL edict_t for client %i", client);
		return 0;
	}

	bot = extmanager->AttachBotInstanceToEntity(entity);
	smutils->LogMessage(myself, "Attached NavBot CPluginBot instance to entity #%i.", client);

	return static_cast<cell_t>(bot->GetIndex());
}

cell_t natives::bots::IsPluginBot(IPluginContext* context, const cell_t* params)
{
	METHODMAP_GETVALIDBOT;

	return bot->IsPluginBot() ? 1 : 0;
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
