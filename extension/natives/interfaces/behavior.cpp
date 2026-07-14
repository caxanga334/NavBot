#include NAVBOT_PCH_FILE
#include <bot/basebot.h>
#include <util/pawnutils.h>
#include "behavior.h"

namespace natives::bots::interfaces::behavior
{
	// Value for when a valid answer cannot be obtained for a query.
	constexpr cell_t BEHAVIOR_DECISION_QUERY_INVALID_ANSWER = -1;

	static cell_t ShouldAttack(IPluginContext* context, const cell_t* params)
	{
		IBehavior* iface = pawnutils::UnsafeCastPawnAddressToObject<IBehavior>(context, params, 1);

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

		CBaseBot* bot = iface->GetBot<CBaseBot>();
		const CKnownEntity* known = bot->GetSensorInterface()->GetKnown(entity);

		if (!known)
		{
			return BEHAVIOR_DECISION_QUERY_INVALID_ANSWER;
		}

		return static_cast<cell_t>(iface->ShouldAttack(bot, known));
	}
	static cell_t ShouldHurry(IPluginContext* context, const cell_t* params)
	{
		IBehavior* iface = pawnutils::UnsafeCastPawnAddressToObject<IBehavior>(context, params, 1);

		if (!iface)
		{
			context->ReportError("NULL bot interface!");
			return 0;
		}

		CBaseBot* bot = iface->GetBot<CBaseBot>();
		return static_cast<cell_t>(iface->ShouldHurry(bot));
	}
	static cell_t ShouldRetreat(IPluginContext* context, const cell_t* params)
	{
		IBehavior* iface = pawnutils::UnsafeCastPawnAddressToObject<IBehavior>(context, params, 1);

		if (!iface)
		{
			context->ReportError("NULL bot interface!");
			return 0;
		}

		CBaseBot* bot = iface->GetBot<CBaseBot>();
		return static_cast<cell_t>(iface->ShouldRetreat(bot));
	}
	static cell_t IsRunningPluginCommand(IPluginContext* context, const cell_t* params)
	{
		IBehavior* iface = pawnutils::UnsafeCastPawnAddressToObject<IBehavior>(context, params, 1);

		if (!iface)
		{
			context->ReportError("NULL bot interface!");
			return 0;
		}

		return pawnutils::ReturnBool(iface->IsRunningPluginCommand());
	}
	static cell_t ShouldAssistTeammate(IPluginContext* context, const cell_t* params)
	{
		IBehavior* iface = pawnutils::UnsafeCastPawnAddressToObject<IBehavior>(context, params, 1);

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

		CBaseBot* bot = iface->GetBot<CBaseBot>();

		// Return invalid if the given entity is not friendly
		if (!bot->GetSensorInterface()->IsFriendly(entity))
		{
			return BEHAVIOR_DECISION_QUERY_INVALID_ANSWER;
		}

		return static_cast<cell_t>(iface->ShouldAssistTeammate(bot, entity));
	}

	void setup(std::vector<sp_nativeinfo_t>& nv)
	{
		sp_nativeinfo_t list[] = {
			{"NavBotBehaviorInterface.ShouldAttack", ShouldAttack},
			{"NavBotBehaviorInterface.ShouldHurry", ShouldHurry},
			{"NavBotBehaviorInterface.ShouldRetreat", ShouldRetreat},
			{"NavBotBehaviorInterface.ShouldAssistTeammate", ShouldAssistTeammate},
			{"NavBotBehaviorInterface.IsRunningPluginCommand", IsRunningPluginCommand},
		};

		nv.insert(nv.end(), std::begin(list), std::end(list));
	}
}