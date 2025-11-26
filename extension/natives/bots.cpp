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

namespace baseinterface
{
	static cell_t Native_Reset(IPluginContext* context, const cell_t* params)
	{
		IBotInterface* interface = pawnutils::PawnAddressToPointer<IBotInterface>(context, params[1]);

		if (!interface)
		{
			context->ReportError("NULL bot interface!");
			return 0;
		}

		interface->Reset();
		return 0;
	}
}

namespace inventory
{
	static cell_t Native_EquipWeapon(IPluginContext* context, const cell_t* params)
	{
		IInventory* iface = pawnutils::PawnAddressToPointer<IInventory>(context, params[1]);

		if (!iface)
		{
			context->ReportError("NULL bot interface!");
			return 0;
		}

		CBaseEntity* weapon = gamehelpers->ReferenceToEntity(params[2]);

		if (!weapon)
		{
			context->ReportError("Entity index %i is invalid!", params[2]);
			return 0;
		}

		CBaseBot* bot = iface->GetBot<CBaseBot>();

		CBaseEntity* owner = nullptr;

		if (!entprops->GetEntPropEnt(weapon, Prop_Send, "m_hOwner", nullptr, &owner))
		{
			context->ReportError("Entity %s is not a CBaseCombatWeapon!", UtilHelpers::textformat::FormatEntity(weapon));
			return 0;
		}

		if (owner != bot->GetEntity())
		{
			context->ReportError("Entity %s is not owned by the bot!", UtilHelpers::textformat::FormatEntity(weapon));
			return 0;
		}

		return iface->EquipWeapon(weapon) ? 1 : 0;
	}

	static cell_t Native_RegisterWeapon(IPluginContext* context, const cell_t* params)
	{
		IInventory* iface = pawnutils::PawnAddressToPointer<IInventory>(context, params[1]);

		if (!iface)
		{
			context->ReportError("NULL bot interface!");
			return 0;
		}

		CBaseEntity* weapon = gamehelpers->ReferenceToEntity(params[2]);

		if (!weapon)
		{
			context->ReportError("Entity index %i is invalid!", params[2]);
			return 0;
		}

		CBaseBot* bot = iface->GetBot<CBaseBot>();

		CBaseEntity* owner = nullptr;

		if (!entprops->GetEntPropEnt(weapon, Prop_Send, "m_hOwner", nullptr, &owner))
		{
			context->ReportError("Entity %s is not a CBaseCombatWeapon!", UtilHelpers::textformat::FormatEntity(weapon));
			return 0;
		}

		if (owner != bot->GetEntity())
		{
			context->ReportError("Entity %s is not owned by the bot!", UtilHelpers::textformat::FormatEntity(weapon));
			return 0;
		}

		iface->AddWeaponToInventory(weapon);
		return 0;
	}

	static cell_t Native_RequestUpdate(IPluginContext* context, const cell_t* params)
	{
		IInventory* iface = pawnutils::PawnAddressToPointer<IInventory>(context, params[1]);

		if (!iface)
		{
			context->ReportError("NULL bot interface!");
			return 0;
		}

		iface->RequestRefresh();
		return 0;
	}
}

namespace basebot
{
	static cell_t Native_GetInventoryInterface(IPluginContext* context, const cell_t* params)
	{
		CBaseBot* bot = pawnutils::GetBotOfIndex<CBaseBot>(params[1]);

		if (!bot)
		{
			context->ReportError("Invalid bot of index %i!", params[1]);
			return 0;
		}

		return pawnutils::PointerToPawnAddress(context, bot->GetInventoryInterface());
	}
}

void natives::bots::setup(std::vector<sp_nativeinfo_t>& nv)
{
	sp_nativeinfo_t list[] = {
		{"NavBot.NavBot", AddNavBotMM},
		{"NavBot.SetSkillLevel", SetSkillLevel},
		/* this should be moved */
		{"NavBotManager.GetNavBotByIndex", GetNavBotByIndex},
		{"NavBot.DelayedFakeClientCommand", DelayedFakeClientCommand},
		/* base interface */
		{"NavBotBotInterface.Reset", baseinterface::Native_Reset},
		/* inventory interface */
		{"NavBotInventoryInterface.EquipWeapon", inventory::Native_EquipWeapon},
		{"NavBotInventoryInterface.RegisterWeapon", inventory::Native_RegisterWeapon},
		{"NavBotInventoryInterface.RequestUpdate", inventory::Native_RequestUpdate},
	};

	nv.insert(nv.end(), std::begin(list), std::end(list));
}

cell_t natives::bots::AddNavBotMM(IPluginContext* context, const cell_t* params)
{
	if (!extmanager->AreBotsSupported())
	{
		context->ReportError("Mod lacks bot suppport!");
		return 0;
	}

#if defined(KE_ARCH_X64)
	if (context->GetRuntime()->FindPubvarByName("__Virtual_Address__", nullptr) != SP_ERROR_NONE)
	{
		context->ReportError("Virtual address is required to use natives on x64!");
		return 0;
	}
#endif

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
#if defined(KE_ARCH_X64)
	if (context->GetRuntime()->FindPubvarByName("__Virtual_Address__", nullptr) != SP_ERROR_NONE)
	{
		context->ReportError("Virtual address is required to use the navigator on x64!");
		return 0;
	}
#endif

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
