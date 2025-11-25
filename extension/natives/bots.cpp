#include NAVBOT_PCH_FILE
#include <extension.h>
#include <manager.h>
#include <util/helpers.h>
#include <util/pawnutils.h>
#include <bot/basebot.h>
#include <bot/interfaces/knownentity.h>
#include <bot/pluginbot/pluginbot.h>
#include <mods/basemod.h>
#include "bots.h"


void natives::bots::setup(std::vector<sp_nativeinfo_t>& nv)
{
	sp_nativeinfo_t list[] = {
		{"NavBot.NavBot", AddNavBotMM},
		{"NavBot.SetSkillLevel", SetSkillLevel},
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

cell_t natives::bots::SetSkillLevel(IPluginContext* context, const cell_t* params)
{
	CBaseBot* bot = pawnutils::GetBotOfIndex<CBaseBot>(params[1]);

	if (!bot)
	{
		context->ReportError("Invalid bot of index %i!", params[1]);
		return 0;
	}

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

	if (!bot)
	{
		return 0;
	}

	return bot->GetIndex();
}

cell_t natives::bots::DelayedFakeClientCommand(IPluginContext* context, const cell_t* params)
{
	CBaseBot* bot = pawnutils::GetBotOfIndex<CBaseBot>(params[1]);

	if (!bot)
	{
		context->ReportError("Invalid bot of index %i!", params[1]);
		return 0;
	}

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
	CBaseBot* bot = pawnutils::GetBotOfIndex<CBaseBot>(params[1]);

	if (!bot)
	{
		context->ReportError("Invalid bot of index %i!", params[1]);
		return 0;
	}

	CBaseEntity* entity = gamehelpers->ReferenceToEntity(params[2]);

	if (!entity)
	{
		context->ReportError("Invalid weapon entity of index %i!", static_cast<int>(params[2]));
		return 0;
	}

	return static_cast<cell_t>(bot->SelectWeapon(entity));
}
