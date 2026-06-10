#include NAVBOT_PCH_FILE
#include <util/pawnutils.h>
#include <extplayer.h>
#include <navmesh/nav_mesh.h>
#include "baseplayer.h"

namespace natives::baseplayer
{
	static cell_t Property_IsNavBot(IPluginContext* context, const cell_t* params)
	{
		CBaseExtPlayer* player = extmanager->GetPlayerByIndex(static_cast<int>(params[1]));

		if (!player)
		{
			context->ReportError("NULL BasePlayer instance!");
			return 0;
		}

		return player->IsExtensionBot() ? 1 : 0;
	}

	static cell_t Native_GetLastKnownNavArea(IPluginContext* context, const cell_t* params)
	{
		CBaseExtPlayer* player = extmanager->GetPlayerByIndex(static_cast<int>(params[pawnutils::GetIndexOfParam(context, 1)]));

		if (!player)
		{
			context->ReportError("NULL BasePlayer instance!");
			return 0;
		}

		return pawnutils::ReturnPointerToPawn(context, params, player->GetLastKnownNavArea());
	}

	static cell_t Native_UpdateLastKnownNavArea(IPluginContext* context, const cell_t* params)
	{
		CBaseExtPlayer* player = extmanager->GetPlayerByIndex(static_cast<int>(params[1]));

		if (!player)
		{
			context->ReportError("NULL BasePlayer instance!");
			return 0;
		}

		player->UpdateLastKnownNavArea(true);
		return 0;
	}

	static cell_t Native_GetLocalEyeAngles(IPluginContext* context, const cell_t* params)
	{
		CBaseExtPlayer* player = extmanager->GetPlayerByIndex(static_cast<int>(params[1]));

		if (!player)
		{
			context->ReportError("NULL BasePlayer instance!");
			return 0;
		}


		const QAngle& eyeAngles = player->GetLocalEyeAngles();
		cell_t* arr = nullptr;
		context->LocalToPhysAddr(params[2], &arr);
		pawnutils::VectorToPawnFloatArray(arr, Vector(eyeAngles.x, eyeAngles.y, eyeAngles.z));
		return 0;
	}

	static cell_t Native_WorldSpaceCenter(IPluginContext* context, const cell_t* params)
	{
		CBaseExtPlayer* player = extmanager->GetPlayerByIndex(static_cast<int>(params[1]));

		if (!player)
		{
			context->ReportError("NULL BasePlayer instance!");
			return 0;
		}

		Vector center = player->WorldSpaceCenter();
		cell_t* arr = nullptr;
		context->LocalToPhysAddr(params[2], &arr);
		pawnutils::VectorToPawnFloatArray(arr, center);
		return 0;
	}

	static cell_t Native_SnapEyeAngles(IPluginContext* context, const cell_t* params)
	{
		CBaseExtPlayer* player = extmanager->GetPlayerByIndex(static_cast<int>(params[1]));

		if (!player)
		{
			context->ReportError("NULL BasePlayer instance!");
			return 0;
		}


		cell_t* arr = nullptr;
		context->LocalToPhysAddr(params[2], &arr);
		Vector vecAngles = pawnutils::PawnFloatArrayToVector(arr);
		QAngle angles(vecAngles.x, vecAngles.y, vecAngles.z);
		player->SnapEyeAngles(angles);
		return 0;
	}
}

void natives::baseplayer::setup(std::vector<sp_nativeinfo_t>& nv)
{
	sp_nativeinfo_t list[] = {
		{"NavBotBasePlayer.IsNavBot.get", Property_IsNavBot},
		{"NavBotBasePlayer.GetLastKnownNavArea", Native_GetLastKnownNavArea},
		{"NavBotBasePlayer.ForceUpdateLastKnownNavArea", Native_UpdateLastKnownNavArea},
		{"NavBotBasePlayer.GetLocalEyeAngles", Native_GetLocalEyeAngles},
		{"NavBotBasePlayer.WorldSpaceCenter", Native_WorldSpaceCenter},
		{"NavBotBasePlayer.SnapEyeAngles", Native_SnapEyeAngles},
	};

	nv.insert(nv.end(), std::begin(list), std::end(list));
}