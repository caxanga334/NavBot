#include NAVBOT_PCH_FILE
#include <util/pawnutils.h>
#include <bot/basebot.h>
#include "playercontroller.h"

namespace natives::bots::interfaces::playercontroller
{
	static cell_t Native_AimAtEntity(IPluginContext* context, const cell_t* params)
	{
		IPlayerController* iface = pawnutils::UnsafeCastPawnAddressToObject<IPlayerController>(context, params, 1);

		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		CBaseEntity* pEntity = pawnutils::ReadEntity(context, params, 2);

		if (!pEntity) { return 0; }

		if (!IPlayerController::SMAPI_IsPriorityValid(params[3]))
		{
			context->ReportError("Priority value %i is invalid!", params[3]);
			return 0;
		}

		IPlayerController::LookPriority priority = static_cast<IPlayerController::LookPriority>(params[3]);
		float duration = pawnutils::ReadFloat(params, 4);
		char* str;
		context->LocalToStringNULL(params[5], &str);
		iface->AimAt(pEntity, priority, duration, str);
		return 0;
	}
	static cell_t Native_AimAtPosition(IPluginContext* context, const cell_t* params)
	{
		IPlayerController* iface = pawnutils::UnsafeCastPawnAddressToObject<IPlayerController>(context, params, 1);

		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		Vector pos = pawnutils::ReadVector(context, params, 2);

		if (!IPlayerController::SMAPI_IsPriorityValid(params[3]))
		{
			context->ReportError("Priority value %i is invalid!", params[3]);
			return 0;
		}

		IPlayerController::LookPriority priority = static_cast<IPlayerController::LookPriority>(params[3]);
		float duration = pawnutils::ReadFloat(params, 4);
		char* str;
		context->LocalToStringNULL(params[5], &str);
		iface->AimAt(pos, priority, duration, str);
		return 0;
	}
	static cell_t Native_IsAimOnTarget(IPluginContext* context, const cell_t* params)
	{
		IPlayerController* iface = pawnutils::UnsafeCastPawnAddressToObject<IPlayerController>(context, params, 1);

		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		return pawnutils::ReturnBool(iface->IsAimOnTarget());
	}
	static cell_t Native_PressButtonByID(IPluginContext* context, const cell_t* params)
	{
		IPlayerController* iface = pawnutils::UnsafeCastPawnAddressToObject<IPlayerController>(context, params, 1);

		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		if (!IPlayerInput::IsValidButtonID(static_cast<int>(params[2])))
		{
			context->ReportError("Button ID %i is invalid!", params[2]);
			return 0;
		}

		float duration = pawnutils::ReadFloat(params, 3);
		iface->PressButtonByID(static_cast<IPlayerInput::ButtonID>(params[2]), duration);
		return 0;
	}
	static cell_t Native_ReleaseButtonByID(IPluginContext* context, const cell_t* params)
	{
		IPlayerController* iface = pawnutils::UnsafeCastPawnAddressToObject<IPlayerController>(context, params, 1);

		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		if (!IPlayerInput::IsValidButtonID(static_cast<int>(params[2])))
		{
			context->ReportError("Button ID %i is invalid!", params[2]);
			return 0;
		}

		iface->ReleaseButtonByID(static_cast<IPlayerInput::ButtonID>(params[2]));
		return 0;
	}

	void setup(std::vector<sp_nativeinfo_t>& nv)
	{
		sp_nativeinfo_t list[] = {
			{"NavBotPlayerControllerInterface.AimAtEntity", Native_AimAtEntity},
			{"NavBotPlayerControllerInterface.AimAtPos", Native_AimAtPosition},
			{"NavBotPlayerControllerInterface.IsAimOnTarget", Native_IsAimOnTarget},
			{"NavBotPlayerControllerInterface.PressButtonByID", Native_PressButtonByID},
			{"NavBotPlayerControllerInterface.ReleaseButtonByID", Native_ReleaseButtonByID},
		};

		nv.insert(nv.end(), std::begin(list), std::end(list));
	}
}
