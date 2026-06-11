#include NAVBOT_PCH_FILE
#include <util/pawnutils.h>
#include <bot/basebot.h>
#include "sensor.h"

namespace natives::bots::interfaces::sensor
{
	static cell_t Native_IsIgnored(IPluginContext* context, const cell_t* params)
	{
		ISensor* iface = pawnutils::UnsafeCastPawnAddressToObject<ISensor>(context, params, 1);

		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		CBaseEntity* pEntity = pawnutils::ReadEntity(context, params, 2);

		if (!pEntity) { return 0; }

		return pawnutils::ReturnBool(iface->IsIgnored(pEntity));
	}
	static cell_t Native_IsFriendly(IPluginContext* context, const cell_t* params)
	{
		ISensor* iface = pawnutils::UnsafeCastPawnAddressToObject<ISensor>(context, params, 1);

		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		CBaseEntity* pEntity = pawnutils::ReadEntity(context, params, 2);

		if (!pEntity) { return 0; }

		return pawnutils::ReturnBool(iface->IsFriendly(pEntity));
	}
	static cell_t Native_IsEnemy(IPluginContext* context, const cell_t* params)
	{
		ISensor* iface = pawnutils::UnsafeCastPawnAddressToObject<ISensor>(context, params, 1);

		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		CBaseEntity* pEntity = pawnutils::ReadEntity(context, params, 2);

		if (!pEntity) { return 0; }

		return pawnutils::ReturnBool(iface->IsEnemy(pEntity));
	}
	static cell_t Native_IsAbleToSeePos(IPluginContext* context, const cell_t* params)
	{
		ISensor* iface = pawnutils::UnsafeCastPawnAddressToObject<ISensor>(context, params, 1);

		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		Vector pos = pawnutils::ReadVector(context, params, 2);
		bool checkFOV = pawnutils::ReadBool(params, 3);

		return pawnutils::ReturnBool(iface->IsAbleToSee(pos, checkFOV));
	}
	static cell_t Native_IsAbleToSeeEntity(IPluginContext* context, const cell_t* params)
	{
		ISensor* iface = pawnutils::UnsafeCastPawnAddressToObject<ISensor>(context, params, 1);

		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		CBaseEntity* pEntity = pawnutils::ReadEntity(context, params, 2);

		if (!pEntity) { return 0; }

		bool checkFOV = pawnutils::ReadBool(params, 3);

		return pawnutils::ReturnBool(iface->IsAbleToSee(pEntity, checkFOV));
	}
	static cell_t Native_IsLineOfSightClearPos(IPluginContext* context, const cell_t* params)
	{
		ISensor* iface = pawnutils::UnsafeCastPawnAddressToObject<ISensor>(context, params, 1);

		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		Vector pos = pawnutils::ReadVector(context, params, 2);

		return pawnutils::ReturnBool(iface->IsLineOfSightClear(pos));
	}
	static cell_t Native_IsLineOfSightClearEntity(IPluginContext* context, const cell_t* params)
	{
		ISensor* iface = pawnutils::UnsafeCastPawnAddressToObject<ISensor>(context, params, 1);

		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		CBaseEntity* pEntity = pawnutils::ReadEntity(context, params, 2);

		if (!pEntity) { return 0; }

		return pawnutils::ReturnBool(iface->IsLineOfSightClear(pEntity));
	}
	static cell_t Native_IsInFieldOfView(IPluginContext* context, const cell_t* params)
	{
		ISensor* iface = pawnutils::UnsafeCastPawnAddressToObject<ISensor>(context, params, 1);

		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		Vector pos = pawnutils::ReadVector(context, params, 2);

		return pawnutils::ReturnBool(iface->IsInFieldOfView(pos));
	}
	static cell_t Native_SetPrimaryThreatOverride(IPluginContext* context, const cell_t* params)
	{
		ISensor* iface = pawnutils::UnsafeCastPawnAddressToObject<ISensor>(context, params, 1);

		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		// NULL is allowed
		CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(params[2]);

		// Don't allow setting the world as a primary threat override
		if (pEntity && UtilHelpers::IndexOfEntity(pEntity) == 0)
		{
			context->ReportError("worldspawn cannot be set as the primary threat override!");
			return 0;
		}

		iface->SetPrimaryThreatOverride(pEntity);
		return 0;
	}
	static cell_t Native_GetPrimaryKnownThreatIndex(IPluginContext* context, const cell_t* params)
	{
		ISensor* iface = pawnutils::UnsafeCastPawnAddressToObject<ISensor>(context, params, 1);

		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		const CKnownEntity* threat = iface->GetPrimaryKnownThreat(pawnutils::ReadBool(params, 2));

		if (!threat)
		{
			// EntityToBCompatRef accepts NULL
			return gamehelpers->EntityToBCompatRef(nullptr);
		}

		return gamehelpers->EntityToBCompatRef(threat->GetEntity());
	}

	void setup(std::vector<sp_nativeinfo_t>& nv)
	{
		sp_nativeinfo_t list[] = {
			{"NavBotSensorInterface.IsIgnored", Native_IsIgnored},
			{"NavBotSensorInterface.IsFriendly", Native_IsFriendly},
			{"NavBotSensorInterface.IsEnemy", Native_IsEnemy},
			{"NavBotSensorInterface.IsAbleToSeePos", Native_IsAbleToSeePos},
			{"NavBotSensorInterface.IsAbleToSeeEntity", Native_IsAbleToSeeEntity},
			{"NavBotSensorInterface.IsLineOfSightClearPos", Native_IsLineOfSightClearPos},
			{"NavBotSensorInterface.IsLineOfSightClearEntity", Native_IsLineOfSightClearEntity},
			{"NavBotSensorInterface.IsInFieldOfView", Native_IsInFieldOfView},
			{"NavBotSensorInterface.SetPrimaryThreatOverride", Native_SetPrimaryThreatOverride},
			{"NavBotSensorInterface.GetPrimaryKnownThreatIndex", Native_GetPrimaryKnownThreatIndex},
		};

		nv.insert(nv.end(), std::begin(list), std::end(list));
	}
}