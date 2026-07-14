#include NAVBOT_PCH_FILE
#include <bot/basebot.h>
#include <util/pawnutils.h>
#include "squad.h"

namespace natives::bots::interfaces::squad
{
	static cell_t IsInASquad(IPluginContext* context, const cell_t* params)
	{
		ISquad* iface = pawnutils::UnsafeCastPawnAddressToObject<ISquad>(context, params, 1);
		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		return pawnutils::ReturnBool(iface->IsSquadValid());
	}
	static cell_t CanJoinSquads(IPluginContext* context, const cell_t* params)
	{
		ISquad* iface = pawnutils::UnsafeCastPawnAddressToObject<ISquad>(context, params, 1);
		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		return pawnutils::ReturnBool(iface->CanJoinSquads());
	}
	static cell_t CanLeadSquads(IPluginContext* context, const cell_t* params)
	{
		ISquad* iface = pawnutils::UnsafeCastPawnAddressToObject<ISquad>(context, params, 1);
		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		return pawnutils::ReturnBool(iface->CanLeadSquads());
	}
	static cell_t IsSquadLeader(IPluginContext* context, const cell_t* params)
	{
		ISquad* iface = pawnutils::UnsafeCastPawnAddressToObject<ISquad>(context, params, 1);
		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		return pawnutils::ReturnBool(iface->IsSquadLeader());
	}
	static cell_t IsHumanLedSquad(IPluginContext* context, const cell_t* params)
	{
		ISquad* iface = pawnutils::UnsafeCastPawnAddressToObject<ISquad>(context, params, 1);
		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		return pawnutils::ReturnBool(iface->IsHumanLedSquad());
	}
	static cell_t GetSquadLeader(IPluginContext* context, const cell_t* params)
	{
		ISquad* iface = pawnutils::UnsafeCastPawnAddressToObject<ISquad>(context, params, 1);
		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		if (!iface->IsSquadValid())
		{
			return 0;
		}

		CBaseEntity* leader = iface->GetSquadData()->GetSquadLeader()->GetPlayerEntity();

		if (!leader)
		{
			return 0;
		}

		return gamehelpers->EntityToBCompatRef(leader);
	}
	static cell_t GetBotSquadLeader(IPluginContext* context, const cell_t* params)
	{
		ISquad* iface = pawnutils::UnsafeCastPawnAddressToObject<ISquad>(context, params, 1);
		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		if (!iface->IsSquadValid())
		{
			return 0;
		}

		CBaseEntity* leader = iface->GetSquadData()->GetBotLeader()->GetPlayerEntity();

		if (!leader)
		{
			return 0;
		}

		return gamehelpers->EntityToBCompatRef(leader);
	}
	static cell_t GetSquadMemberCount(IPluginContext* context, const cell_t* params)
	{
		ISquad* iface = pawnutils::UnsafeCastPawnAddressToObject<ISquad>(context, params, 1);
		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		if (!iface->IsSquadValid())
		{
			return 0;
		}

		return static_cast<cell_t>(iface->GetSquadData()->GetMemberCount());
	}
	static cell_t GetSquadMemberEntity(IPluginContext* context, const cell_t* params)
	{
		ISquad* iface = pawnutils::UnsafeCastPawnAddressToObject<ISquad>(context, params, 1);
		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		if (!iface->IsSquadValid())
		{
			return 0;
		}

		auto data = iface->GetSquadData();

		if (params[2] < 0 || params[2] >= static_cast<cell_t>(data->GetMemberCount()))
		{
			context->ReportError("Index %i is out of bounds! Must be larger than 0 and less than %zu.", params[2], data->GetMemberCount());
			return 0;
		}

		auto member = data->GetMemberOfIndex(static_cast<std::size_t>(params[2]));
		CBaseEntity* entity = member->GetPlayerEntity();

		if (!entity)
		{
			return 0;
		}

		return gamehelpers->EntityToBCompatRef(entity);
	}
	static cell_t CreateSquad(IPluginContext* context, const cell_t* params)
	{
		ISquad* iface = pawnutils::UnsafeCastPawnAddressToObject<ISquad>(context, params, 1);
		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		// don't create if already in a squad
		if (iface->IsSquadValid())
		{
			return pawnutils::ReturnBool(false);
		}

		CBaseEntity* entity = gamehelpers->ReferenceToEntity(params[2]);

		if (entity)
		{
			if (extmanager->GetBotFromEntity(entity) != nullptr)
			{
				context->ReportError("\"client\" param cannot be a NavBot instance!");
				return 0;
			}

			// check if the passed entity is actually a player index
			if (!pawnutils::IsValidClientIndex(context, params[2]))
			{
				return 0;
			}

			// no need to check for in-game since they already have a valid CBaseEntity

			CBaseBot* bot = iface->GetBot<CBaseBot>();

			// return false if an enemy player was passed.
			if (!bot->GetSensorInterface()->IsFriendly(entity))
			{
				return pawnutils::ReturnBool(false);
			}
		}

		return pawnutils::ReturnBool(iface->CreateSquad(entity));
	}
	static cell_t DestroySquad(IPluginContext* context, const cell_t* params)
	{
		ISquad* iface = pawnutils::UnsafeCastPawnAddressToObject<ISquad>(context, params, 1);
		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		iface->DestroySquad();
		return 0;
	}
	static cell_t AddMemberToSquad(IPluginContext* context, const cell_t* params)
	{
		ISquad* iface = pawnutils::UnsafeCastPawnAddressToObject<ISquad>(context, params, 1);
		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		int client = static_cast<int>(params[2]);

		if (!pawnutils::IsValidClientIndex(context, client))
		{
			return 0;
		}

		if (!pawnutils::IsClientInGame(context, client))
		{
			return 0;
		}

		CBaseEntity* entity = pawnutils::ReadEntity(context, params, 2);

		if (!entity)
		{
			return 0;
		}

		CBaseBot* bot = extmanager->GetBotByIndex(client);
		bool result = false;

		if (bot)
		{
			result = iface->AddMemberToSquad(bot);
		}
		else
		{
			result = iface->AddMemberToSquad(entity);
		}

		return pawnutils::ReturnBool(result);
	}
	static cell_t RemoveMemberFromSquad(IPluginContext* context, const cell_t* params)
	{
		ISquad* iface = pawnutils::UnsafeCastPawnAddressToObject<ISquad>(context, params, 1);
		if (!iface) { context->ReportError("NULL interface ptr!"); return 0; }

		int client = static_cast<int>(params[2]);

		if (!pawnutils::IsValidClientIndex(context, client))
		{
			return 0;
		}

		if (!pawnutils::IsClientInGame(context, client))
		{
			return 0;
		}

		CBaseEntity* entity = pawnutils::ReadEntity(context, params, 2);

		if (!entity)
		{
			return 0;
		}

		CBaseBot* bot = extmanager->GetBotByIndex(client);
		bool result = false;

		if (bot)
		{
			result = iface->AddMemberToSquad(bot);
		}
		else
		{
			result = iface->AddMemberToSquad(entity);
		}

		return pawnutils::ReturnBool(result);
	}
	static cell_t GetHumanPlayerSquad(IPluginContext* context, const cell_t* params)
	{
		const std::size_t firstparam = pawnutils::GetIndexOfParam(context, 1);

		int client = static_cast<int>(params[firstparam]);

		if (!pawnutils::IsValidClientIndex(context, client))
		{
			return 0;
		}

		if (!pawnutils::IsClientInGame(context, client))
		{
			return 0;
		}

		if (extmanager->IsNavBot(client))
		{
			context->ReportError("Client cannot be a NavBot instance.");
			return 0;
		}

		CBaseEntity* player = gamehelpers->ReferenceToEntity(client);
		bool leaderonly = pawnutils::ReadBool(params, firstparam + 1U);
		ISquad* squad = ISquad::GetHumanPlayerSquad(player, leaderonly);
		return pawnutils::ReturnPointerToPawn(context, params, squad);
	}
	void setup(std::vector<sp_nativeinfo_t>& nv)
	{
		sp_nativeinfo_t list[] = {
			{"NavBotSquadInterface.IsInASquad", IsInASquad},
			{"NavBotSquadInterface.CanJoinSquads", CanJoinSquads},
			{"NavBotSquadInterface.CanLeadSquads", CanLeadSquads},
			{"NavBotSquadInterface.IsSquadLeader", IsSquadLeader},
			{"NavBotSquadInterface.IsHumanLedSquad", IsHumanLedSquad},
			{"NavBotSquadInterface.GetSquadLeader", GetSquadLeader},
			{"NavBotSquadInterface.GetBotSquadLeader", GetBotSquadLeader},
			{"NavBotSquadInterface.GetSquadMemberCount", GetSquadMemberCount},
			{"NavBotSquadInterface.GetSquadMemberEntity", GetSquadMemberEntity},
			{"NavBotSquadInterface.CreateSquad", CreateSquad},
			{"NavBotSquadInterface.DestroySquad", DestroySquad},
			{"NavBotSquadInterface.AddMemberToSquad", AddMemberToSquad},
			{"NavBotSquadInterface.RemoveMemberFromSquad", RemoveMemberFromSquad},
			{"NavBotSquadInterface.GetHumanPlayerSquad", GetHumanPlayerSquad},
		};

		nv.insert(nv.end(), std::begin(list), std::end(list));
	}
}