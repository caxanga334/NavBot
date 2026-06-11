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
		IBotInterface* interface = pawnutils::UnsafeCastPawnAddressToObject<IBotInterface>(context, params, 1);

		if (!interface)
		{
			context->ReportError("Got NULL pointer from address %i!", params[1]);
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
		IInventory* iface = pawnutils::UnsafeCastPawnAddressToObject<IInventory>(context, params, 1);

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

		const CBotWeapon* botweapon = iface->GetWeaponOfEntity(weapon);

		if (!botweapon)
		{
			context->ReportError("Weapon %s is not registered in the bot's inventory!", UtilHelpers::textformat::FormatEntity(weapon));
			return 0;
		}

		CBaseBot* bot = iface->GetBot<CBaseBot>();

		if (!botweapon->IsOwnedByBot(bot))
		{
			context->ReportError("Entity %s is not owned by the bot!", UtilHelpers::textformat::FormatEntity(weapon));
			return 0;
		}

		return iface->EquipWeapon(botweapon) ? 1 : 0;
	}

	static cell_t Native_RegisterWeapon(IPluginContext* context, const cell_t* params)
	{
		IInventory* iface = pawnutils::UnsafeCastPawnAddressToObject<IInventory>(context, params, 1);

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
		IInventory* iface = pawnutils::UnsafeCastPawnAddressToObject<IInventory>(context, params, 1);

		if (!iface)
		{
			context->ReportError("NULL bot interface!");
			return 0;
		}

		iface->RequestRefresh();
		return 0;
	}

	static cell_t Native_SelectFirstWeaponWithTag(IPluginContext* context, const cell_t* params)
	{
		IInventory* iface = pawnutils::UnsafeCastPawnAddressToObject<IInventory>(context, params, 1);

		if (!iface)
		{
			context->ReportError("NULL bot interface!");
			return 0;
		}

		char* tag;
		context->LocalToString(params[2], &tag);

		const CBotWeapon* weapon = iface->FindWeaponByTag(tag);

		if (!weapon)
		{
			return 0;
		}

		return pawnutils::ReturnBool(iface->EquipWeapon(weapon));
	}

	static cell_t Native_SelectBestWeapon(IPluginContext* context, const cell_t* params)
	{
		IInventory* iface = pawnutils::UnsafeCastPawnAddressToObject<IInventory>(context, params, 1);

		if (!iface)
		{
			context->ReportError("NULL bot interface!");
			return 0;
		}

		iface->SelectBestWeapon();
		return 0;
	}
}

namespace movement
{
	static cell_t Native_IsPotentiallyTraversable(IPluginContext* context, const cell_t* params)
	{
		IMovement* iface = pawnutils::UnsafeCastPawnAddressToObject<IMovement>(context, params, 1);

		if (!iface)
		{
			context->ReportError("NULL bot interface!");
			return 0;
		}

		cell_t* fromArr = nullptr;
		context->LocalToPhysAddr(params[2], &fromArr);
		Vector vecFrom = pawnutils::PawnFloatArrayToVector(fromArr);
		cell_t* toArr = nullptr;
		context->LocalToPhysAddr(params[3], &toArr);
		Vector vecTo = pawnutils::PawnFloatArrayToVector(toArr);
		float fraction = 0.0f;
		bool now = params[5] != 0;
		CBaseEntity* pEntity = nullptr;
		bool result = iface->IsPotentiallyTraversable(vecFrom, vecTo, &fraction, now, &pEntity);

		cell_t* flAddr;
		context->LocalToPhysAddr(params[4], &flAddr);
		*flAddr = sp_ftoc(fraction);

		cell_t* entAddr;
		context->LocalToPhysAddr(params[6], &entAddr);
		
		if (!pEntity)
		{
			*entAddr = -1;
		}
		else
		{
			*entAddr = gamehelpers->EntityToBCompatRef(pEntity);
		}

		return pawnutils::ReturnBool(result);
	}
	static cell_t Native_IsStuck(IPluginContext* context, const cell_t* params)
	{
		IMovement* iface = pawnutils::UnsafeCastPawnAddressToObject<IMovement>(context, params, 1);

		if (!iface)
		{
			context->ReportError("NULL bot interface!");
			return 0;
		}

		return pawnutils::ReturnBool(iface->IsStuck());
	}
	static cell_t Native_GetStuckDuration(IPluginContext* context, const cell_t* params)
	{
		IMovement* iface = pawnutils::UnsafeCastPawnAddressToObject<IMovement>(context, params, 1);

		if (!iface)
		{
			context->ReportError("NULL bot interface!");
			return 0;
		}

		return sp_ftoc(iface->GetStuckDuration());
	}
}

namespace basebot
{
	static cell_t Native_GetInventoryInterface(IPluginContext* context, const cell_t* params)
	{
		std::size_t index = pawnutils::GetIndexOfParam(context, 1);
		CBaseBot* bot = pawnutils::GetBotOfIndex<CBaseBot>(params[index]);

		if (!bot)
		{
			context->ReportError("Invalid bot of index %i!", params[index]);
			return 0;
		}

		return pawnutils::ReturnPointerToPawn(context, params, bot->GetInventoryInterface());
	}
	static cell_t Native_GetMovementInterface(IPluginContext* context, const cell_t* params)
	{
		std::size_t index = pawnutils::GetIndexOfParam(context, 1);
		CBaseBot* bot = pawnutils::GetBotOfIndex<CBaseBot>(params[index]);

		if (!bot)
		{
			context->ReportError("Invalid bot of index %i!", params[index]);
			return 0;
		}

		return pawnutils::ReturnPointerToPawn(context, params, bot->GetMovementInterface());
	}
	static cell_t Native_GetSensorInterface(IPluginContext* context, const cell_t* params)
	{
		std::size_t index = pawnutils::GetIndexOfParam(context, 1);
		CBaseBot* bot = pawnutils::GetBotOfIndex<CBaseBot>(params[index]);

		if (!bot)
		{
			context->ReportError("Invalid bot of index %i!", params[index]);
			return 0;
		}

		return pawnutils::ReturnPointerToPawn(context, params, bot->GetSensorInterface());
	}
	static cell_t Native_GetPlayerControllerInterface(IPluginContext* context, const cell_t* params)
	{
		std::size_t index = pawnutils::GetIndexOfParam(context, 1);
		CBaseBot* bot = pawnutils::GetBotOfIndex<CBaseBot>(params[index]);

		if (!bot)
		{
			context->ReportError("Invalid bot of index %i!", params[index]);
			return 0;
		}

		return pawnutils::ReturnPointerToPawn(context, params, bot->GetControlInterface());
	}
	static cell_t Native_Reset(IPluginContext* context, const cell_t* params)
	{
		CBaseBot* bot = pawnutils::GetBotOfIndex<CBaseBot>(params[1]);

		if (!bot)
		{
			context->ReportError("Invalid bot of index %i!", params[1]);
			return 0;
		}

		bot->Reset();
		return 0;
	}

	static cell_t Native_IsLineOfFireClear(IPluginContext* context, const cell_t* params)
	{
		CBaseBot* bot = pawnutils::GetBotOfIndex<CBaseBot>(params[1]);

		if (!bot)
		{
			context->ReportError("Invalid bot of index %i!", params[1]);
			return 0;
		}

		cell_t* arr = nullptr;
		context->LocalToPhysAddr(params[2], &arr);
		Vector vTo = pawnutils::PawnFloatArrayToVector(arr);
		return pawnutils::ReturnBool(bot->IsLineOfFireClear(vTo));
	}

	static cell_t Native_SendImpulse(IPluginContext* context, const cell_t* params)
	{
		CBaseBot* bot = pawnutils::GetBotOfIndex<CBaseBot>(params[1]);

		if (!bot)
		{
			context->ReportError("Invalid bot of index %i!", params[1]);
			return 0;
		}

		bot->SetImpulseCommand(static_cast<int>(params[2]));
		return 0;
	}

	static cell_t Native_GetDebugIdentifier(IPluginContext* context, const cell_t* params)
	{
		CBaseBot* bot = pawnutils::GetBotOfIndex<CBaseBot>(params[1]);

		if (!bot)
		{
			context->ReportError("Invalid bot of index %i!", params[1]);
			return 0;
		}

		char buffer[512];
		std::memset(buffer, 0, sizeof(buffer));
		ke::SafeSprintf(buffer, sizeof(buffer), "%s", bot->GetDebugIdentifier());
		context->StringToLocal(params[2], params[3], buffer);
		return 0;
	}
}

void natives::bots::setup(std::vector<sp_nativeinfo_t>& nv)
{
	sp_nativeinfo_t list[] = {
		{"NavBot.NavBot", AddNavBotMM},
		{"NavBot.SetSkillLevel", SetSkillLevel},
		{"NavBot.GetInventoryInterface", basebot::Native_GetInventoryInterface},
		{"NavBot.GetMovementInterface", basebot::Native_GetMovementInterface},
		{"NavBot.GetSensorInterface", basebot::Native_GetSensorInterface},
		{"NavBot.GetPlayerControllerInterface", basebot::Native_GetPlayerControllerInterface},
		{"NavBot.Reset", basebot::Native_Reset},
		{"NavBot.IsLineOfFireClear", basebot::Native_IsLineOfFireClear},
		{"NavBot.SendImpulse", basebot::Native_SendImpulse},
		{"NavBot.GetDebugIdentifier", basebot::Native_GetDebugIdentifier},
		/* this should be moved */
		{"NavBotManager.GetNavBotByIndex", GetNavBotByIndex},
		{"NavBot.DelayedFakeClientCommand", DelayedFakeClientCommand},
		/* base interface */
		{"NavBotBotInterface.Reset", baseinterface::Native_Reset},
		/* inventory interface */
		{"NavBotInventoryInterface.EquipWeapon", inventory::Native_EquipWeapon},
		{"NavBotInventoryInterface.RegisterWeapon", inventory::Native_RegisterWeapon},
		{"NavBotInventoryInterface.RequestUpdate", inventory::Native_RequestUpdate},
		{"NavBotInventoryInterface.SelectFirstWeaponWithTag", inventory::Native_SelectFirstWeaponWithTag},
		{"NavBotInventoryInterface.SelectBestWeapon", inventory::Native_SelectBestWeapon},
		/* IMovement */
		{"NavBotMovementInterface.IsPotentiallyTraversable", movement::Native_IsPotentiallyTraversable},
		{"NavBotMovementInterface.IsStuck", movement::Native_IsStuck},
		{"NavBotMovementInterface.GetStuckDuration", movement::Native_GetStuckDuration},
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
