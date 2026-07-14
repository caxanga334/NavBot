#include NAVBOT_PCH_FILE
#include <util/pawnutils.h>
#include <bot/basebot.h>
#include "movement.h"

namespace natives::bots::interfaces::movement
{
	static cell_t IsPotentiallyTraversable(IPluginContext* context, const cell_t* params)
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
	static cell_t IsStuck(IPluginContext* context, const cell_t* params)
	{
		IMovement* iface = pawnutils::UnsafeCastPawnAddressToObject<IMovement>(context, params, 1);

		if (!iface)
		{
			context->ReportError("NULL bot interface!");
			return 0;
		}

		return pawnutils::ReturnBool(iface->IsStuck());
	}
	static cell_t GetStuckDuration(IPluginContext* context, const cell_t* params)
	{
		IMovement* iface = pawnutils::UnsafeCastPawnAddressToObject<IMovement>(context, params, 1);

		if (!iface)
		{
			context->ReportError("NULL bot interface!");
			return 0;
		}

		return pawnutils::ReturnFloat(iface->GetStuckDuration());
	}
	static cell_t IsAreaTraversable(IPluginContext* context, const cell_t* params)
	{
		IMovement* iface = pawnutils::UnsafeCastPawnAddressToObject<IMovement>(context, params, 1);

		if (!iface)
		{
			context->ReportError("NULL bot interface!");
			return 0;
		}

		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 2);

		if (!area)
		{
			context->ReportError("NULL area!");
			return 0;
		}

		return pawnutils::ReturnBool(iface->IsAreaTraversable(area));
	}
	static cell_t ClearStuckStatus(IPluginContext* context, const cell_t* params)
	{
		IMovement* iface = pawnutils::UnsafeCastPawnAddressToObject<IMovement>(context, params, 1);

		if (!iface)
		{
			context->ReportError("NULL bot interface!");
			return 0;
		}

		char* reason = nullptr;
		context->LocalToStringNULL(params[2], &reason);
		iface->ClearStuckStatus(reason);
		return 0;
	}
	static cell_t ClimbUpToLedge(IPluginContext* context, const cell_t* params)
	{
		IMovement* iface = pawnutils::UnsafeCastPawnAddressToObject<IMovement>(context, params, 1);

		if (!iface)
		{
			context->ReportError("NULL bot interface!");
			return 0;
		}

		Vector landing = pawnutils::ReadVector(context, params, 2);
		CBaseBot* bot = iface->GetBot<CBaseBot>();
		Vector origin = bot->GetAbsOrigin();
		Vector dir = landing - origin;
		dir.z = 0.0f;
		dir.NormalizeInPlace();
		return pawnutils::ReturnBool(iface->ClimbUpToLedge(landing, dir, nullptr));
	}
	static cell_t DoubleJumpToLedge(IPluginContext* context, const cell_t* params)
	{
		IMovement* iface = pawnutils::UnsafeCastPawnAddressToObject<IMovement>(context, params, 1);

		if (!iface)
		{
			context->ReportError("NULL bot interface!");
			return 0;
		}

		Vector landing = pawnutils::ReadVector(context, params, 2);
		CBaseBot* bot = iface->GetBot<CBaseBot>();
		Vector origin = bot->GetAbsOrigin();
		Vector dir = landing - origin;
		dir.z = 0.0f;
		dir.NormalizeInPlace();
		return pawnutils::ReturnBool(iface->DoubleJumpToLedge(landing, dir, nullptr));
	}
	static cell_t JumpAcrossGap(IPluginContext* context, const cell_t* params)
	{
		IMovement* iface = pawnutils::UnsafeCastPawnAddressToObject<IMovement>(context, params, 1);

		if (!iface)
		{
			context->ReportError("NULL bot interface!");
			return 0;
		}

		Vector landing = pawnutils::ReadVector(context, params, 2);
		CBaseBot* bot = iface->GetBot<CBaseBot>();
		Vector origin = bot->GetAbsOrigin();
		Vector dir = landing - origin;
		dir.z = 0.0f;
		dir.NormalizeInPlace();
		iface->JumpAcrossGap(landing, dir);
		return 0;
	}
	static cell_t BlastJumpTo(IPluginContext* context, const cell_t* params)
	{
		IMovement* iface = pawnutils::UnsafeCastPawnAddressToObject<IMovement>(context, params, 1);

		if (!iface)
		{
			context->ReportError("NULL bot interface!");
			return 0;
		}

		Vector landing = pawnutils::ReadVector(context, params, 2);
		CBaseBot* bot = iface->GetBot<CBaseBot>();
		Vector origin = bot->GetAbsOrigin();
		Vector dir = landing - origin;
		dir.z = 0.0f;
		dir.NormalizeInPlace();
		iface->BlastJumpTo(origin, landing, dir);
		return 0;
	}
	static cell_t BreakObstacle(IPluginContext* context, const cell_t* params)
	{
		IMovement* iface = pawnutils::UnsafeCastPawnAddressToObject<IMovement>(context, params, 1);

		if (!iface)
		{
			context->ReportError("NULL bot interface!");
			return 0;
		}

		CBaseEntity* entity = pawnutils::ReadEntity(context, params, 2);

		if (!entity)
		{
			return 0;
		}

		if (params[2] == 0)
		{
			context->ReportError("worldspawn is not allowed!");
			return 0;
		}

		if (params[2] > 0 && params[2] <= gpGlobals->maxClients)
		{
			context->ReportError("Entity cannot be a player!");
			return 0;
		}

		iface->BreakObstacle(entity);
		return 0;
	}
	static cell_t AddDeadArea(IPluginContext* context, const cell_t* params)
	{
		IMovement* iface = pawnutils::UnsafeCastPawnAddressToObject<IMovement>(context, params, 1);

		if (!iface)
		{
			context->ReportError("NULL bot interface!");
			return 0;
		}

		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 2);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		float duration = pawnutils::ReadFloat(params, 3);
		iface->AddDeadArea(area, duration);
		return 0;
	}
	static cell_t AddCostModArea(IPluginContext* context, const cell_t* params)
	{
		IMovement* iface = pawnutils::UnsafeCastPawnAddressToObject<IMovement>(context, params, 1);

		if (!iface)
		{
			context->ReportError("NULL bot interface!");
			return 0;
		}

		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 2);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		float costmult = pawnutils::ReadFloat(params, 3);

		if (costmult <= 0.0f)
		{
			context->ReportError("Cost multiplier cannot be zero or negative! Got %g", costmult);
			return 0;
		}

		float duration = pawnutils::ReadFloat(params, 4);
		iface->AddCostModArea(area, costmult, duration);
		return 0;
	}

	void setup(std::vector<sp_nativeinfo_t>& nv)
	{
		sp_nativeinfo_t list[] = {
			/* IMovement */
			{ "NavBotMovementInterface.IsPotentiallyTraversable", IsPotentiallyTraversable},
			{ "NavBotMovementInterface.IsStuck", IsStuck},
			{ "NavBotMovementInterface.GetStuckDuration", GetStuckDuration},
			{ "NavBotMovementInterface.IsAreaTraversable", IsAreaTraversable},
			{ "NavBotMovementInterface.ClearStuckStatus", ClearStuckStatus},
			{ "NavBotMovementInterface.ClimbUpToLedge", ClimbUpToLedge},
			{ "NavBotMovementInterface.DoubleJumpToLedge", DoubleJumpToLedge},
			{ "NavBotMovementInterface.JumpAcrossGap", JumpAcrossGap},
			{ "NavBotMovementInterface.BlastJumpTo", BlastJumpTo},
			{ "NavBotMovementInterface.BreakObstacle", BreakObstacle},
			{ "NavBotMovementInterface.AddDeadArea", AddDeadArea},
			{ "NavBotMovementInterface.AddCostModArea", AddCostModArea},
		};

		nv.insert(nv.end(), std::begin(list), std::end(list));
	}
}