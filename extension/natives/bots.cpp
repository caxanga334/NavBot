#include NAVBOT_PCH_FILE
#include <extension.h>
#include <manager.h>
#include <util/helpers.h>
#include <util/pawnutils.h>
#include <bot/basebot.h>
#include <bot/interfaces/knownentity.h>
#include <bot/pluginbot/pluginbot.h>
#include <mods/basemod.h>
#include "interfaces/movement.h"
#include "interfaces/behavior.h"
#include "interfaces/combat.h"
#include "interfaces/squad.h"
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
	static cell_t Native_GetBot(IPluginContext* context, const cell_t* params)
	{
		IBotInterface* interface = pawnutils::UnsafeCastPawnAddressToObject<IBotInterface>(context, params, 1);

		if (!interface)
		{
			context->ReportError("Got NULL pointer from address %i!", params[1]);
			return 0;
		}

		return static_cast<cell_t>(interface->GetBot<CBaseBot>()->GetIndex());
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
	static cell_t Native_GetBestLowAmmoWeapon(IPluginContext* context, const cell_t* params)
	{
		IInventory* iface = pawnutils::UnsafeCastPawnAddressToObject<IInventory>(context, params, 1);

		if (!iface)
		{
			context->ReportError("NULL bot interface!");
			return 0;
		}

		const CBotWeapon* weapon = iface->GetBestLowAmmoWeapon();

		if (!weapon)
		{
			return gamehelpers->EntityToBCompatRef(nullptr);
		}

		return static_cast<cell_t>(weapon->GetIndex());
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
	static cell_t Native_GetBehaviorInterface(IPluginContext* context, const cell_t* params)
	{
		std::size_t index = pawnutils::GetIndexOfParam(context, 1);
		CBaseBot* bot = pawnutils::GetBotOfIndex<CBaseBot>(params[index]);

		if (!bot)
		{
			context->ReportError("Invalid bot of index %i!", params[index]);
			return 0;
		}

		return pawnutils::ReturnPointerToPawn(context, params, bot->GetBehaviorInterface());
	}
	static cell_t Native_GetCombatInterface(IPluginContext* context, const cell_t* params)
	{
		std::size_t index = pawnutils::GetIndexOfParam(context, 1);
		CBaseBot* bot = pawnutils::GetBotOfIndex<CBaseBot>(params[index]);

		if (!bot)
		{
			context->ReportError("Invalid bot of index %i!", params[index]);
			return 0;
		}

		return pawnutils::ReturnPointerToPawn(context, params, bot->GetCombatInterface());
	}
	static cell_t Native_GetSquadInterface(IPluginContext* context, const cell_t* params)
	{
		std::size_t index = pawnutils::GetIndexOfParam(context, 1);
		CBaseBot* bot = pawnutils::GetBotOfIndex<CBaseBot>(params[index]);

		if (!bot)
		{
			context->ReportError("Invalid bot of index %i!", params[index]);
			return 0;
		}

		return pawnutils::ReturnPointerToPawn(context, params, bot->GetSquadInterface());
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

	static cell_t Native_SendBehaviorPluginCommand(IPluginContext* context, const cell_t* params)
	{
		CBaseBot* bot = pawnutils::GetBotOfIndex<CBaseBot>(params[1]);

		if (!bot)
		{
			context->ReportError("Invalid bot of index %i!", params[1]);
			return 0;
		}

		if (params[2] < 0 || params[2] >= static_cast<cell_t>(IEventListener::PluginCommandTypes::MAX_PLUGIN_COMMAND_TYPES))
		{
			context->ReportError("%i is not a valid plugin command!", params[2]);
			return 0;
		}

		IEventListener::PluginCommandTypes cmd = static_cast<IEventListener::PluginCommandTypes>(params[2]);
		IEventListener::PluginCommandData data;
		constexpr std::size_t startparam = 3;
		std::size_t numparams = static_cast<std::size_t>(params[0]);

		switch (cmd)
		{
		case IEventListener::PluginCommandTypes::PLUGINCMD_SCRIPTED:
		{
			context->ReportError("Use SendScriptedPluginCommand!");
			return 0;
		}
		case IEventListener::PluginCommandTypes::PLUGINCMD_ATTACK_MOVE:
			[[fallthrough]];
		case IEventListener::PluginCommandTypes::PLUGINCMD_MOVE_TO:
		{
			if (startparam > numparams)
			{
				context->ReportError("This command requires a vector (float[3]) to be passed!");
				return 0;
			}

			data.movegoal = pawnutils::ReadVector(context, params, startparam);
			break;
		}
		case IEventListener::PluginCommandTypes::PLUGINCMD_SEEK_AND_DESTROY:
			[[fallthrough]];
		case IEventListener::PluginCommandTypes::PLUGINCMD_USE_ENTITY:
		{
			if (startparam > numparams)
			{
				context->ReportError("This command requires an entity to be passed!");
				return 0;
			}

			CBaseEntity* entity = gamehelpers->ReferenceToEntity(pawnutils::ReadIntByRef(context, params, startparam));

			if (!entity)
			{
				context->ReportError("NULL entity!");
				return 0;
			}

			if (entity == bot->GetEntity())
			{
				context->ReportError("Given entity cannot be the bot!");
				return 0;
			}

			data.entdata = entity;

			break;
		}
		case IEventListener::PluginCommandTypes::PLUGINCMD_WAIT:
		{
			if (startparam > numparams)
			{
				context->ReportError("This command requires a float to be passed!");
				return 0;
			}

			data.fldata = pawnutils::ReadFloatByRef(context, params, startparam);

			if (data.fldata < 0.0f)
			{
				context->ReportError("Wait time cannot be negative!");
				return 0;
			}

			break;
		}
		case IEventListener::PluginCommandTypes::PLUGINCMD_PATROL:
		{
			break;
		}
		case IEventListener::PluginCommandTypes::PLUGINCMD_ROAM:
		{
			if (startparam > numparams)
			{
				context->ReportError("This command requires a float to be passed!");
				return 0;
			}

			data.fldata = pawnutils::ReadFloatByRef(context, params, startparam);

			if (data.fldata < 256.0f)
			{
				context->ReportError("Roam search distance cannot be smaller than 256 (got %g)!", data.fldata);
				return 0;
			}

			break;
		}
		case IEventListener::PluginCommandTypes::PLUGINCMD_FOLLOW_ENTITY:
		{
			if (startparam + 2U > numparams)
			{
				context->ReportError("This command requires three arguments to be passed!");
				return 0;
			}

			CBaseEntity* entity = gamehelpers->ReferenceToEntity(pawnutils::ReadIntByRef(context, params, startparam));

			if (!entity)
			{
				context->ReportError("NULL entity!");
				return 0;
			}

			if (entity == bot->GetEntity())
			{
				context->ReportError("Given entity cannot be the bot!");
				return 0;
			}

			data.entdata.Set(entity);
			data.fldata = pawnutils::ReadFloatByRef(context, params, startparam + 1U);
			data.movegoal.x = pawnutils::ReadFloatByRef(context, params, startparam + 2U);

			break;
		}
		default:
			return 0;
		}

		bot->OnPluginCommand(cmd, data);
		return 0;
	}

	static cell_t Native_SendScriptedBehaviorPluginCommand(IPluginContext* context, const cell_t* params)
	{
		CBaseBot* bot = pawnutils::GetBotOfIndex<CBaseBot>(params[1]);

		if (!bot)
		{
			context->ReportError("Invalid bot of index %i!", params[1]);
			return 0;
		}

		constexpr auto cmd = IEventListener::PluginCommandTypes::PLUGINCMD_SCRIPTED;
		IEventListener::PluginCommandData data;

		if (!pawnutils::ReadFunctionByID(context, params, 2, &data.sb_update_callback))
		{
			return 0;
		}

		bot->OnPluginCommand(cmd, data);
		return 0;
	}
	static cell_t Native_GetHealthState(IPluginContext* context, const cell_t* params)
	{
		CBaseBot* bot = pawnutils::GetBotOfIndex<CBaseBot>(params[1]);

		if (!bot)
		{
			context->ReportError("Invalid bot of index %i!", params[1]);
			return 0;
		}

		return static_cast<cell_t>(bot->GetHealthState());
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
		{"NavBot.GetBehaviorInterface", basebot::Native_GetBehaviorInterface},
		{"NavBot.GetCombatInterface", basebot::Native_GetCombatInterface},
		{"NavBot.GetSquadInterface", basebot::Native_GetSquadInterface},
		{"NavBot.Reset", basebot::Native_Reset},
		{"NavBot.IsLineOfFireClear", basebot::Native_IsLineOfFireClear},
		{"NavBot.SendImpulse", basebot::Native_SendImpulse},
		{"NavBot.GetDebugIdentifier", basebot::Native_GetDebugIdentifier},
		{"NavBot.GetHealthState", basebot::Native_GetHealthState},
		/* this should be moved */
		{"NavBotManager.GetNavBotByIndex", GetNavBotByIndex},
		{"NavBot.DelayedFakeClientCommand", DelayedFakeClientCommand},
		{"NavBot.SendPluginCommand", basebot::Native_SendBehaviorPluginCommand},
		{"NavBot.SendScriptedPluginCommand", basebot::Native_SendScriptedBehaviorPluginCommand},
		/* base interface */
		{"NavBotBotInterface.Reset", baseinterface::Native_Reset},
		{"NavBotBotInterface.GetBot", baseinterface::Native_GetBot},
		/* inventory interface */
		{"NavBotInventoryInterface.EquipWeapon", inventory::Native_EquipWeapon},
		{"NavBotInventoryInterface.RegisterWeapon", inventory::Native_RegisterWeapon},
		{"NavBotInventoryInterface.RequestUpdate", inventory::Native_RequestUpdate},
		{"NavBotInventoryInterface.SelectFirstWeaponWithTag", inventory::Native_SelectFirstWeaponWithTag},
		{"NavBotInventoryInterface.SelectBestWeapon", inventory::Native_SelectBestWeapon},
		{"NavBotInventoryInterface.GetBestLowAmmoWeapon", inventory::Native_GetBestLowAmmoWeapon},
	};

	nv.insert(nv.end(), std::begin(list), std::end(list));

	natives::bots::interfaces::movement::setup(nv);
	natives::bots::interfaces::behavior::setup(nv);
	natives::bots::interfaces::combat::setup(nv);
	natives::bots::interfaces::squad::setup(nv);
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