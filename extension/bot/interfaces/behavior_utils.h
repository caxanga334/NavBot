#ifndef __NAVBOT_BOT_BEHAVIOR_INTERFACE_UTILS_H_
#define __NAVBOT_BOT_BEHAVIOR_INTERFACE_UTILS_H_

/*
								CTF2BotScenarioTask::OnSquadEvent(CTF2Bot* bot, SquadEventType evtype)
{
	if (evtype == SquadEventType::SQUAD_EVENT_JOINED)
	{
		AITask<CTF2Bot>* task = static_cast<AITask<CTF2Bot>*>(new CTF2BotScenarioTask);
		return TrySwitchTo(new CBotSharedSquadMemberMonitorTask<CTF2Bot, CTF2BotPathCost>(task), PRIORITY_CRITICAL, "I have joined a squad, starting squad member behavior!");
	}

	return TryContinue(PRIORITY_LOW);
}
*/

// Simple rogue behavior check.
#define BOTBEHAVIOR_IMPLEMENT_SIMPLE_ROGUE_CHECK(BOTCLASS, PATHCOST)													\
int __roguechance = extmanager->GetMod()->GetModSettings()->GetRogueBehaviorChance();									\
																														\
if (__roguechance > 0 && CBaseBot::s_botrng.GetRandomChance(__roguechance))												\
{																														\
	return PauseFor(new CBotSharedRogueBehaviorTask<BOTCLASS, PATHCOST>(), "Starting rogue behavior!");					\
}																														\
																														\

// Simple take cover from incoming danger check.
#define BOTVEHAVIOR_IMPLEMENT_SIMPLE_DANGER_COVER(BOTCLASS, PATHCOST)													\
																														\
if (bot->GetBehaviorInterface()->ShouldRetreat(bot) != ANSWER_NO)														\
{																														\
	Vector __hitpos = vec3_origin;																						\
																														\
		if (extmanager->GetMod()->IsInProjectilesPath(bot, newent, __hitpos))											\
		{																												\
			return TryPauseFor(new CBotSharedTakeCoverFromDangerTask<BOTCLASS, PATHCOST>(newent, __hitpos),				\
				PRIORITY_MEDIUM, "Taking cover from incoming projectile!");												\
		}																												\
																														\
}																														\
																														\


namespace behaviorutils
{
	template <typename BotClass, typename PathCost, typename SquadTask, typename ExitTask, typename... TArgs>
	TaskEventResponseResult<BotClass> ImplementSquadJoinEvent(AITask<BotClass>* ptrThisTask, BotClass* ptrBot, TArgs&&... _args)
	{
		if (ptrBot->GetSquadInterface()->UsesSquadBehavior())
		{
			AITask<BotClass>* task = static_cast<AITask<BotClass>*>(new ExitTask(std::forward<TArgs>(_args)...));
			return ptrThisTask->TrySwitchTo(new SquadTask(task), PRIORITY_CRITICAL, "I have joined a squad, starting squad member behavior!");
		}

		return ptrThisTask->TryContinue();
	}
}

#endif // !__NAVBOT_BOT_BEHAVIOR_INTERFACE_UTILS_H_
