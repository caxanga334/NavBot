#include NAVBOT_PCH_FILE
#include <mods/zps/zps_mod.h>
#include <bot/zps/zpsbot.h>
#include <util/pawnutils.h>
#include "zps.h"

namespace natives::mods::zps
{
#ifndef NO_SOURCEPAWN_API
	static CZombiePanicSourceMod* GetZPSMod(IPluginContext* context)
	{
		CZombiePanicSourceMod* mod = pawnutils::GetModInterfacePointerOfType<CZombiePanicSourceMod>(Mods::ModType::MOD_ZPS);

		if (!mod)
		{
			context->ReportError("This native is only available on Zombie Panic! Source.");
			return nullptr;
		}

		return mod;
	}
	static cell_t SetCurrentObjective(IPluginContext* context, const cell_t* params)
	{
		if (params[1] < 0 || params[1] >= static_cast<cell_t>(CZPSObjectiveManager::ObjectiveTypes::MAX_OBJECTIVE_TYPES))
		{
			context->ReportError("Invalid objective of type %i!", params[1]);
			return 0;
		}

		CZombiePanicSourceMod* mod = GetZPSMod(context);

		if (!mod)
		{
			return 0;
		}

		mod->SMAPI_SetCurrentObjective(static_cast<CZPSObjectiveManager::ObjectiveTypes>(params[1]));
		return 0;
	}
	static cell_t SetObjectiveMoveGoal(IPluginContext* context, const cell_t* params)
	{
		CZombiePanicSourceMod* mod = GetZPSMod(context);

		if (!mod)
		{
			return 0;
		}

		Vector goal = pawnutils::ReadVector(context, params, 1);
		mod->SMAPI_SetObjectiveMoveGoal(goal);
		return 0;
	}
	static cell_t SetObjectiveUseButton(IPluginContext* context, const cell_t* params)
	{
		CZombiePanicSourceMod* mod = GetZPSMod(context);

		if (!mod)
		{
			return 0;
		}

		CBaseEntity* entity = gamehelpers->ReferenceToEntity(params[1]);
		mod->SMAPI_SetObjectiveUseButton(entity);
		return 0;
	}
	static cell_t SetObjectiveItemSearchID(IPluginContext* context, const cell_t* params)
	{
		CZombiePanicSourceMod* mod = GetZPSMod(context);

		if (!mod)
		{
			return 0;
		}

		char* str;
		context->LocalToString(params[1], &str);
		mod->SMAPI_SetObjectiveItemSearchID(str);
		return 0;
	}
	static cell_t SetObjectiveItemUseTarget(IPluginContext* context, const cell_t* params)
	{
		CZombiePanicSourceMod* mod = GetZPSMod(context);

		if (!mod)
		{
			return 0;
		}

		CBaseEntity* entity = gamehelpers->ReferenceToEntity(params[1]);
		mod->SMAPI_SetObjectiveItemUseTarget(entity);
		return 0;
	}
	static cell_t SetObjectiveDetectionRadius(IPluginContext* context, const cell_t* params)
	{
		CZombiePanicSourceMod* mod = GetZPSMod(context);

		if (!mod)
		{
			return 0;
		}

		float radius = pawnutils::ReadFloat(params, 1);

		if (radius < 128.0f)
		{
			context->ReportError("Radius cannot be smaller than 128.0! (%g)", radius);
			return 0;
		}

		mod->SMAPI_SetObjectiveDetectionRadius(radius);
		return 0;
	}
	static cell_t ResetObjective(IPluginContext* context, const cell_t* params)
	{
		CZombiePanicSourceMod* mod = GetZPSMod(context);

		if (!mod)
		{
			return 0;
		}

		mod->SMAPI_ResetObjective();
		return 0;
	}


#endif // !NO_SOURCEPAWN_API

	void setup(std::vector<sp_nativeinfo_t>& nv)
	{
#ifndef NO_SOURCEPAWN_API
		sp_nativeinfo_t list[] = {
			{"NavBotZPSModInterface.SetCurrentObjective", SetCurrentObjective},
			{"NavBotZPSModInterface.SetObjectiveMoveGoal", SetObjectiveMoveGoal},
			{"NavBotZPSModInterface.SetObjectiveUseButton", SetObjectiveUseButton},
			{"NavBotZPSModInterface.SetObjectiveItemSearchID", SetObjectiveItemSearchID},
			{"NavBotZPSModInterface.SetObjectiveItemUseTarget", SetObjectiveItemUseTarget},
			{"NavBotZPSModInterface.SetObjectiveDetectionRadius", SetObjectiveDetectionRadius},
			{"NavBotZPSModInterface.ResetObjective", ResetObjective},

		};

		nv.insert(nv.end(), std::begin(list), std::end(list));
#endif // !NO_SOURCEPAWN_API

	}
}