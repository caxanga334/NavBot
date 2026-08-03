#ifndef __NAVBOT_BOT_SHARED_PREREQ_TASKS_H_
#define __NAVBOT_BOT_SHARED_PREREQ_TASKS_H_

#include "bot_shared_prereq_destroy_ent.h"
#include "bot_shared_prereq_move_to_pos.h"
#include "bot_shared_prereq_wait.h"
#include "bot_shared_prereq_use_ent.h"
#include <navmesh/nav_area.h>
#include <navmesh/nav_prereq.h>

namespace botprereqtasks
{
	template <typename BotClass, typename TaskClass, typename AreaClass, typename PathCostClass>
	inline TaskEventResponseResult<BotClass> ImplementPrereqCheck(TaskClass* task, BotClass* bot, AreaClass* area)
	{
		if (area != nullptr && area->HasPrerequisite())
		{
			const CNavPrerequisite* prereq = area->GetPrerequisite();

			if (prereq->CanBeUsedByBot(bot))
			{
				CNavPrerequisite::PrerequisiteTask tasktype = prereq->GetTask();

				switch (tasktype)
				{
				case CNavPrerequisite::TASK_WAIT:
					return task->TryPauseFor(new CBotSharedPrereqWaitTask<BotClass>(prereq->GetFloatData()), PRIORITY_HIGH, 
						"Prerequisite tells me to wait!");
				case CNavPrerequisite::TASK_MOVE_TO_POS:
					return task->TryPauseFor(new CBotSharedPrereqMoveToPositionTask<BotClass, PathCostClass>(bot, prereq), PRIORITY_HIGH, 
						"Prerequisite tells me to move to a position!");
				case CNavPrerequisite::TASK_DESTROY_ENT:
					return task->TryPauseFor(new CBotSharedPrereqDestroyEntityTask<BotClass, PathCostClass>(bot, prereq), PRIORITY_HIGH, 
						"Prerequisite tells me to destroy an entity!");
				case CNavPrerequisite::TASK_USE_ENT:
					return task->TryPauseFor(new CBotSharedPrereqUseEntityTask<BotClass, PathCostClass>(bot, prereq), PRIORITY_HIGH, 
						"Prerequisite tells me to use an entity!");
				default:
					return task->TryContinue();
				}
			}
		}

		return task->TryContinue();
	}
}


#endif // !__NAVBOT_BOT_SHARED_PREREQ_TASKS_H_
