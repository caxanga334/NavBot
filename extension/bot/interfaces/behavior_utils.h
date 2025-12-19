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

#define BOTBEHAVIOR_IMPLEMENT_PREREQUISITE_CHECK(BOTCLASS, PATHCOST)			\
if (newArea && newArea->HasPrerequisite())										\
{																				\
	const CNavPrerequisite* prereq = newArea->GetPrerequisite();				\
																				\
	if (prereq->IsEnabled() && prereq != bot->GetLastUsedPrerequisite())		\
	{																			\
		CNavPrerequisite::PrerequisiteTask task = prereq->GetTask();			\
																				\
		switch (task)															\
		{																		\
		case CNavPrerequisite::TASK_WAIT:										\
			return TryPauseFor(new CBotSharedPrereqWaitTask<BOTCLASS>(prereq->GetFloatData()), PRIORITY_HIGH, "Prerequisite tells me to wait!");	\
		case CNavPrerequisite::TASK_MOVE_TO_POS:								\
			return TryPauseFor(new CBotSharedPrereqMoveToPositionTask<BOTCLASS, PATHCOST>(bot, prereq), PRIORITY_HIGH, "Prerequisite tells me to move to a position!");	\
		case CNavPrerequisite::TASK_DESTROY_ENT:								\
			return TryPauseFor(new CBotSharedPrereqDestroyEntityTask<BOTCLASS, PATHCOST>(bot, prereq), PRIORITY_HIGH, "Prerequisite tells me to destroy an entity!");	\
		case CNavPrerequisite::TASK_USE_ENT:									\
			return TryPauseFor(new CBotSharedPrereqUseEntityTask<BOTCLASS, PATHCOST>(bot, prereq), PRIORITY_HIGH, "Prerequisite tells me to use an entity!");			\
		default:																\
			break;																\
		}																		\
	}																			\
}																				\
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
