#ifndef SMNAV_BOT_TASK_SYSTEM_H_
#define SMNAV_BOT_TASK_SYSTEM_H_
#pragma once

#include <vector>
#include <stdexcept>

#include "decisionquery.h"
#include "event_listener.h"

// Forward declaration
template <typename BotClass> class AITask;
class CPath;

// Types of results an AI Task can have
enum TaskResultType
{
	TASK_CONTINUE = 0, // Keep running the current task
	TASK_PAUSE, // Pause the current task for another task
	TASK_SWITCH, // Replaces the current task with another
	TASK_DONE, // End the current task
	TASK_MAINTAIN, // For events, try to keep the current task

	MAX_TASK_TYPES
};

// Priorities for the AI Task event response results
enum EventResultPriorityType
{
	PRIORITY_IGNORED = -1, // This result is ignored and is considered invalid
	PRIORITY_DONT_CARE = 0, // Use if possible but in the end don't care if it's not used
	PRIORITY_LOW,
	PRIORITY_MEDIUM,
	PRIORITY_HIGH,
	PRIORITY_CRITICAL,
	PRIORITY_MANDATORY, // This priority is special and will emit errors if it can't be used or collides with another

	MAX_EVENT_RESULT_PRIORITY_TYPES
};

template <typename BotClass>
class TaskResultInterface
{
public:
	TaskResultInterface(TaskResultType type = TASK_CONTINUE, AITask<BotClass>* nextTask = nullptr, const char* reason = nullptr)
	{
		m_resulttype = type;
		m_next = nextTask;
		m_reason = reason;
	}

	TaskResultType GetType() const { return m_resulttype; }
	bool IsContinue() const { return m_resulttype == TASK_CONTINUE; }
	bool IsPause() const { return m_resulttype == TASK_PAUSE; }
	bool IsSwitch() const { return m_resulttype == TASK_SWITCH; }
	bool IsDone() const { return m_resulttype == TASK_DONE; }
	bool IsMaintain() const { return m_resulttype == TASK_MAINTAIN; }
	// Returns true if a change is requested. (A change can be a pause, a switch or the task ending)
	bool IsRequestingChange() const
	{
		switch (m_resulttype)
		{
		case TASK_CONTINUE:
			return false;
		case TASK_PAUSE:
		case TASK_SWITCH:
		case TASK_DONE:
			return true;
		case TASK_MAINTAIN:
		case MAX_TASK_TYPES:
		default:
			return false;
		}
	}

	AITask<BotClass>* GetNextTask() const { return m_next; }

	const char* GetReason() const { return m_reason; }

	const char* GetTypeName() const
	{
		switch (m_resulttype)
		{
		case TASK_CONTINUE:
			return "CONTINUE";
		case TASK_PAUSE:
			return "PAUSE";
		case TASK_SWITCH:
			return "SWITCH";
		case TASK_DONE:
			return "DONE";
		case TASK_MAINTAIN:
			return "MAINTAIN";
		case MAX_TASK_TYPES:
		default:
			return "INVALID TASK TYPE";
		}
	}

	// Discard this result, deletes the stored task if any
	void DiscardResult()
	{
		if (m_next)
		{
			delete m_next;
			m_next = nullptr;
		}
	}

private:
	TaskResultType m_resulttype; // Task result type
	AITask<BotClass>* m_next; // Optional pointer to another AI Task if needed for the given result (pause and switch)
	const char* m_reason; // Reason for this change (for debugging the AI behavior)
};

template <typename BotClass>
class TaskResult : public TaskResultInterface<BotClass>
{
public:
	TaskResult(TaskResultType type = TASK_CONTINUE, AITask<BotClass>* nextTask = nullptr, const char* reason = nullptr) :
		TaskResultInterface<BotClass>(type, nextTask, reason)
	{
	}
};

template <typename BotClass>
class TaskEventResponseResult : public TaskResultInterface<BotClass>
{
public:
	TaskEventResponseResult(EventResultPriorityType priority = PRIORITY_DONT_CARE, TaskResultType type = TASK_CONTINUE, AITask<BotClass>* nextTask = nullptr, const char* reason = nullptr) : TaskResultInterface<BotClass>(type, nextTask, reason)
	{
		m_priority = priority;
	}

	EventResultPriorityType GetPriority() const { return m_priority; }

	const char* GetPriorityTypeName() const
	{
		switch (m_priority)
		{
		case PRIORITY_IGNORED:
			return "IGNORED";
		case PRIORITY_DONT_CARE:
			return "DONT CARE";
		case PRIORITY_LOW:
			return "LOW";
		case PRIORITY_MEDIUM:
			return "MEDIUM";
		case PRIORITY_HIGH:
			return "HIGH";
		case PRIORITY_CRITICAL:
			return "CRITICAL";
		case PRIORITY_MANDATORY:
			return "MANDATORY";
		case MAX_EVENT_RESULT_PRIORITY_TYPES:
		default:
			return "INVALID EVENT PRIORITY TYPE";
		}
	}

private:
	EventResultPriorityType m_priority;
};


/**
 * @brief AI Task Manager is the root of the AI Task system. It act as a container for all running tasks.
 * @tparam BotClass Class of the bot
*/
template <typename BotClass>
class AITaskManager : public IEventListener, public IDecisionQuery
{
public:
	AITaskManager(AITask<BotClass>* initialTask);
	virtual ~AITaskManager();

	virtual std::vector<IEventListener*>* GetListenerVector();

	bool IsRunningTasks() { return m_task != nullptr; }

	void Reset(AITask<BotClass>* task);
	void Update(BotClass* bot);

	// Notify that a task has ended, add it to the list of tasks for deallocation
	void NotifyTaskEnd(AITask<BotClass>* task) { m_taskbin.push_back(task); }

	// Macros for propagating decision queries between AI Tasks

#define PROPAGATE_DECISION_WITH_1ARG(DFUNC, ARG1)			\
	QueryAnswerType __result = ANSWER_UNDEFINED;					\
																\
	if (m_task)													\
	{															\
		AITask<BotClass>* __respondingTask = nullptr; \
		for (__respondingTask = m_task; __respondingTask->GetNextTask() != nullptr; __respondingTask = __respondingTask->GetNextTask()) {} \
																\
		while (__respondingTask != nullptr && __result == ANSWER_UNDEFINED) \
		{ \
			AITask<BotClass>* __previousTask = __respondingTask->GetHeadTask(); \
			while (__respondingTask != nullptr && __result == ANSWER_UNDEFINED) \
			{ \
				__result = __respondingTask->DFUNC(ARG1); \
				__respondingTask = __respondingTask->GetTaskBelowMe(); \
			} \
																 \
			__respondingTask = __previousTask; \
		} \
	} \
		\
	return __result; \
	\

#define PROPAGATE_DECISION_WITH_2ARGS(DFUNC, ARG1, ARG2)			\
	QueryAnswerType __result = ANSWER_UNDEFINED;					\
																\
	if (m_task)													\
	{															\
		AITask<BotClass>* __respondingTask = nullptr; \
		for (__respondingTask = m_task; __respondingTask->GetNextTask() != nullptr; __respondingTask = __respondingTask->GetNextTask()) {} \
																\
		while (__respondingTask != nullptr && __result == ANSWER_UNDEFINED) \
		{ \
			AITask<BotClass>* __previousTask = __respondingTask->GetHeadTask(); \
			while (__respondingTask != nullptr && __result == ANSWER_UNDEFINED) \
			{ \
				__result = __respondingTask->DFUNC(ARG1, ARG2); \
				__respondingTask = __respondingTask->GetTaskBelowMe(); \
			} \
																 \
			__respondingTask = __previousTask; \
		} \
	} \
		\
	return __result; \
	\

#define PROPAGATE_DECISION_WITH_3ARGS(DFUNC, ARG1, ARG2, ARG3)		\
	QueryAnswerType __result = ANSWER_UNDEFINED;					\
																\
	if (m_task)													\
	{															\
		AITask<BotClass>* __respondingTask = nullptr; \
		for (__respondingTask = m_task; __respondingTask->GetNextTask() != nullptr; __respondingTask = __respondingTask->GetNextTask()) {} \
																\
		while (__respondingTask != nullptr && __result == ANSWER_UNDEFINED) \
		{ \
			AITask<BotClass>* __previousTask = __respondingTask->GetHeadTask(); \
			while (__respondingTask != nullptr && __result == ANSWER_UNDEFINED) \
			{ \
				__result = __respondingTask->DFUNC(ARG1, ARG2, ARG3); \
				__respondingTask = __respondingTask->GetTaskBelowMe(); \
			} \
																 \
			__respondingTask = __previousTask; \
		} \
	} \
		\
	return __result; \
	\

#define PROPAGATE_DECISION_WITH_4ARGS(DFUNC, ARG1, ARG2, ARG3, ARG4)		\
	QueryAnswerType __result = ANSWER_UNDEFINED;					\
																\
	if (m_task)													\
	{															\
		AITask<BotClass>* __respondingTask = nullptr; \
		for (__respondingTask = m_task; __respondingTask->GetNextTask() != nullptr; __respondingTask = __respondingTask->GetNextTask()) {} \
																\
		while (__respondingTask != nullptr && __result == ANSWER_UNDEFINED) \
		{ \
			AITask<BotClass>* __previousTask = __respondingTask->GetHeadTask(); \
			while (__respondingTask != nullptr && __result == ANSWER_UNDEFINED) \
			{ \
				__result = __respondingTask->DFUNC(ARG1, ARG2, ARG3, ARG4); \
				__respondingTask = __respondingTask->GetTaskBelowMe(); \
			} \
																 \
			__respondingTask = __previousTask; \
		} \
	} \
			\
	return __result; \
			\

	virtual QueryAnswerType ShouldAttack(const CBaseBot* me, CKnownEntity* them) override
	{
		PROPAGATE_DECISION_WITH_2ARGS(ShouldAttack, me, them);
	}

	virtual QueryAnswerType ShouldPickup(const CBaseBot* me, edict_t* item) override
	{
		PROPAGATE_DECISION_WITH_2ARGS(ShouldPickup, me, item);
	}

	virtual QueryAnswerType ShouldHurry(const CBaseBot* me) override
	{
		PROPAGATE_DECISION_WITH_1ARG(ShouldHurry, me);
	}

	virtual QueryAnswerType ShouldRetreat(const CBaseBot* me) override
	{
		PROPAGATE_DECISION_WITH_1ARG(ShouldRetreat, me);
	}

	virtual QueryAnswerType ShouldUse(const CBaseBot* me, edict_t* object) override
	{
		PROPAGATE_DECISION_WITH_2ARGS(ShouldUse, me, object);
	}

	virtual QueryAnswerType ShouldFreeRoam(const CBaseBot* me) override
	{
		PROPAGATE_DECISION_WITH_1ARG(ShouldFreeRoam, me);
	}

	virtual QueryAnswerType ShouldWaitForBlocker(const CBaseBot* me, edict_t* blocker) override
	{
		PROPAGATE_DECISION_WITH_2ARGS(ShouldWaitForBlocker, me, blocker);
	}

	virtual CKnownEntity* SelectTargetThreat(const CBaseBot* me, CKnownEntity* threat1, CKnownEntity* threat2) override
	{
		CKnownEntity* result = nullptr;

		if (m_task)
		{
			AITask<BotClass>* respondingTask = nullptr;
			for (respondingTask = m_task; respondingTask->GetNextTask() != nullptr; respondingTask = respondingTask->GetNextTask()) {}

			while (respondingTask != nullptr && result == nullptr)
			{
				AITask<BotClass>* previousTask = respondingTask->GetHeadTask();
				while (respondingTask != nullptr && result == nullptr)
				{
					result = respondingTask->SelectTargetThreat(me, threat1, threat2);
					respondingTask = respondingTask->GetTaskBelowMe();
				}

				respondingTask = previousTask;
			}
		}

		return result;
	}

	virtual Vector GetTargetAimPos(const CBaseBot* me, edict_t* entity, CBaseExtPlayer* player = nullptr) override
	{
		Vector result = vec3_origin;

		if (m_task)
		{
			AITask<BotClass>* respondingTask = nullptr;
			for (respondingTask = m_task; respondingTask->GetNextTask() != nullptr; respondingTask = respondingTask->GetNextTask()) {}

			while (respondingTask != nullptr && result == vec3_origin)
			{
				AITask<BotClass>* previousTask = respondingTask->GetHeadTask();
				while (respondingTask != nullptr && result == vec3_origin)
				{
					result = respondingTask->GetTargetAimPos(me, entity, player);
					respondingTask = respondingTask->GetTaskBelowMe();
				}

				respondingTask = previousTask;
			}
		}

		return result;
	}

private:
	friend class AITask<BotClass>;

	AITask<BotClass>* m_task; // The first task in the list
	std::vector<AITask<BotClass>*> m_taskbin; // Trash bin for dead tasks
	std::vector<IEventListener*> m_listeners; // Event listeners
	BotClass* m_bot;

	void CleanUpDeadTasks();
};

template<typename BotClass>
inline AITaskManager<BotClass>::AITaskManager(AITask<BotClass>* initialTask)
{
	m_task = initialTask;
	m_taskbin.reserve(16);
	m_listeners.reserve(2);
	m_bot = nullptr;

	m_listeners.push_back(m_task);
}

template<typename BotClass>
inline AITaskManager<BotClass>::~AITaskManager()
{
	delete m_task;

	for (auto task : m_taskbin)
	{
		delete task;
	}

	m_taskbin.clear();
	m_listeners.clear();
}

template<typename BotClass>
inline std::vector<IEventListener*>* AITaskManager<BotClass>::GetListenerVector()
{
	m_listeners.clear();
	
	if (m_task == nullptr)
	{
		return nullptr;
	}

	m_listeners.push_back(m_task); // always refresh before sending

	return &m_listeners;
}

template<typename BotClass>
inline void AITaskManager<BotClass>::Reset(AITask<BotClass>* task)
{
	delete m_task;
	CleanUpDeadTasks();

	m_task = task;

	m_listeners.clear();

	if (m_task != nullptr)
	{
		m_listeners.push_back(m_task);
	}
}

template<typename BotClass>
inline void AITaskManager<BotClass>::Update(BotClass* bot)
{
	if (IsRunningTasks() == false)
	{
		return;
	}

	if (m_bot == nullptr) { m_bot = bot; }

	if (bot->IsDebugging(BOTDEBUG_TASKS))
	{
		bot->DebugDisplayText(m_task->DebugString());
	}

	m_task = m_task->RunTask(bot, this, m_task->ProcessTaskUpdate(bot, this));

	CleanUpDeadTasks();
}

template<typename BotClass>
inline void AITaskManager<BotClass>::CleanUpDeadTasks()
{
	for (auto task : m_taskbin)
	{
		delete task;
	}

	m_taskbin.clear();
}

template <typename BotClass>
class AITask : public IEventListener, public IDecisionQuery
{
public:
	AITask();
	virtual ~AITask();

	BotClass* GetBot() const { return m_bot; }

	bool HasStarted() const { return m_hasStarted; }
	bool IsPaused() const { return m_isPaused; }

	virtual TaskResult<BotClass> OnTaskStart(BotClass* bot, AITask<BotClass>* pastTask) { return Continue(); }
	virtual TaskResult<BotClass> OnTaskUpdate(BotClass* bot) { return Continue(); }
	virtual void OnTaskEnd(BotClass* bot, AITask<BotClass>* nextTask) { return; }
	virtual bool OnTaskPause(BotClass* bot, AITask<BotClass>* nextTask) { return true; }
	virtual TaskResult<BotClass> OnTaskResume(BotClass* bot, AITask<BotClass>* pastTask) { return Continue(); }

	virtual const char* GetName() const = 0;

	const char* DebugString() const;
	char* BuildDebugString(char* name, const AITask<BotClass>* task) const;

	// These functions are used to trigger a state change on a Task.

	TaskResult<BotClass> Continue() const;
	TaskResult<BotClass> SwitchTo(AITask<BotClass>* newTask, const char* reason) const;
	TaskResult<BotClass> PauseFor(AITask<BotClass>* newTask, const char* reason) const;
	TaskResult<BotClass> Done(const char* reason) const;

	TaskEventResponseResult<BotClass> TryContinue(EventResultPriorityType priority = PRIORITY_DONT_CARE) const;
	TaskEventResponseResult<BotClass> TrySwitchTo(AITask<BotClass>* task, EventResultPriorityType priority = PRIORITY_DONT_CARE, const char* reason = nullptr) const;
	TaskEventResponseResult<BotClass> TryPauseFor(AITask<BotClass>* task, EventResultPriorityType priority = PRIORITY_DONT_CARE, const char* reason = nullptr) const;
	TaskEventResponseResult<BotClass> TryDone(EventResultPriorityType priority = PRIORITY_DONT_CARE, const char* reason = nullptr) const;

	virtual TaskEventResponseResult<BotClass> OnTestEventPropagation(BotClass* bot) { return TryContinue(); }
	virtual TaskEventResponseResult<BotClass> OnStuck(BotClass* bot) { return TryContinue(); }
	virtual TaskEventResponseResult<BotClass> OnUnstuck(BotClass* bot) { return TryContinue(); }
	virtual TaskEventResponseResult<BotClass> OnMoveToFailure(BotClass* bot, CPath* path, IEventListener::MovementFailureType reason) { return TryContinue(); }
	virtual TaskEventResponseResult<BotClass> OnMoveToSuccess(BotClass* bot, CPath* path) { return TryContinue(); }
	virtual TaskEventResponseResult<BotClass> OnInjured(BotClass* bot, edict_t* attacker = nullptr) { return TryContinue(); }
	virtual TaskEventResponseResult<BotClass> OnKilled(BotClass* bot, edict_t* attacker = nullptr) { return TryContinue(); }
	virtual TaskEventResponseResult<BotClass> OnOtherKilled(BotClass* bot, edict_t* victim, edict_t* attacker = nullptr) { return TryContinue(); }
	virtual TaskEventResponseResult<BotClass> OnSight(BotClass* bot, edict_t* subject) { return TryContinue(); }
	virtual TaskEventResponseResult<BotClass> OnLostSight(BotClass* bot, edict_t* subject) { return TryContinue(); }
	virtual TaskEventResponseResult<BotClass> OnSound(BotClass* bot, edict_t* source, const Vector& position, SoundType type, const int volume) { return TryContinue(); }

	virtual AITask<BotClass>* InitialNextTask() { return nullptr; }

	AITask<BotClass>* GetHeadTask() const { return m_headTask; }
	AITask<BotClass>* GetNextTask() const { return m_nextTask; }
	AITask<BotClass>* GetTaskAboveMe() const { return m_aboveTask; }
	AITask<BotClass>* GetTaskBelowMe() const { return m_belowTask; }

private:
	friend class AITaskManager<BotClass>;
	AITaskManager<BotClass>* m_manager;
	AITask<BotClass>* m_headTask; // The task that contains this task
	AITask<BotClass>* m_nextTask; // The next *ACTIVE* task that this task contains
	AITask<BotClass>* m_aboveTask;
	AITask<BotClass>* m_belowTask;
	BotClass* m_bot;
	std::vector<IEventListener*> m_listener; // Event Listeners
	mutable TaskEventResponseResult<BotClass> m_pendingEventResult;
	bool m_hasStarted;
	bool m_isPaused;

	// Macros to help with repetitive code for event propagation between tasks

#define PROPAGATE_TASK_EVENT_WITH_NO_ARGS(EFUNC)						\
	if (m_hasStarted == false)											\
	{																	\
		return;															\
	}																	\
																		\
	AITask<BotClass>*__task = this;										\
	TaskEventResponseResult<BotClass> __eventResult;					\
																		\
	while (__task != nullptr)											\
	{																	\
		if (m_bot && m_bot->IsDebugging(BOTDEBUG_EVENTS))				\
		{																\
			m_bot->DebugPrintToConsole(BOTDEBUG_EVENTS, 100, 100, 100, "%s (%s) RECEIVED EVENT %s \n", m_bot->GetDebugIdentifier(), __task->GetName(), #EFUNC); \
		}																\
																		\
		__eventResult = __task->EFUNC(m_bot);							\
																		\
		if (__eventResult.IsContinue() == false) {						\
																		\
				break;													\
		}																\
																		\
				__task = __task->GetTaskBelowMe();						\
	}																	\
																		\
	if (__task)															\
	{																	\
																		\
		if (m_bot && __eventResult.IsRequestingChange() && m_bot->IsDebugging(BOTDEBUG_TASKS)) \
		{																						\
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, "%s ", m_bot->GetDebugIdentifier()); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, "%s ", __task->GetName()); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 0, "responded to EVENT %s with ", #EFUNC); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 0, 0, "%s %s ", __eventResult.GetTypeName(), __eventResult.GetNextTask() ? __eventResult.GetNextTask()->GetName() : ""); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 100, 255, 0, "result PRIORITY (%s) ", __eventResult.GetPriorityTypeName()); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 0, 255, 0, "(%s) \n", __eventResult.GetReason() ? __eventResult.GetReason() : ""); \
		}																\
																		\
																		\
		__task->UpdatePendingEventResult(__eventResult);				\
	}																	\
																		\
	IEventListener::EFUNC();											\

#define PROPAGATE_TASK_EVENT_WITH_1_ARGS(EFUNC, ARG1)					\
	if (m_hasStarted == false)											\
	{																	\
		return;															\
	}																	\
																		\
	AITask<BotClass>*__task = this;										\
	TaskEventResponseResult<BotClass> __eventResult;					\
																		\
	while (__task != nullptr)											\
	{																	\
		if (m_bot && m_bot->IsDebugging(BOTDEBUG_EVENTS))				\
		{																\
			m_bot->DebugPrintToConsole(BOTDEBUG_EVENTS, 100, 100, 100, "%s (%s) RECEIVED EVENT %s \n", m_bot->GetDebugIdentifier(), __task->GetName(), #EFUNC); \
		}																\
		__eventResult = __task->EFUNC(m_bot, ARG1);						\
																		\
		if (__eventResult.IsContinue() == false) {						\
																		\
				break;													\
		}																\
																		\
				__task = __task->GetTaskBelowMe();						\
	}																	\
																		\
	if (__task)															\
	{																	\
																		\
		if (m_bot && __eventResult.IsRequestingChange() && m_bot->IsDebugging(BOTDEBUG_TASKS)) \
		{																						\
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, "%s ", m_bot->GetDebugIdentifier()); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, "%s ", __task->GetName()); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 0, "responded to EVENT %s with ", #EFUNC); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 0, 0, "%s %s ", __eventResult.GetTypeName(), __eventResult.GetNextTask() ? __eventResult.GetNextTask()->GetName() : ""); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 100, 255, 0, "result PRIORITY (%s) ", __eventResult.GetPriorityTypeName()); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 0, 255, 0, "(%s) \n", __eventResult.GetReason() ? __eventResult.GetReason() : ""); \
		}																\
																		\
		__task->UpdatePendingEventResult(__eventResult);				\
	}																	\
																		\
	IEventListener::EFUNC(ARG1);										\

#define PROPAGATE_TASK_EVENT_WITH_2_ARGS(EFUNC, ARG1, ARG2)				\
	if (m_hasStarted == false)											\
	{																	\
		return;															\
	}																	\
																		\
	AITask<BotClass>*__task = this;										\
	TaskEventResponseResult<BotClass> __eventResult;					\
																		\
	while (__task != nullptr)											\
	{																	\
		if (m_bot && m_bot->IsDebugging(BOTDEBUG_EVENTS))				\
		{																\
			m_bot->DebugPrintToConsole(BOTDEBUG_EVENTS, 100, 100, 100, "%s (%s) RECEIVED EVENT %s \n", m_bot->GetDebugIdentifier(), __task->GetName(), #EFUNC); \
		}																\
		__eventResult = __task->EFUNC(m_bot, ARG1, ARG2);				\
																		\
		if (__eventResult.IsContinue() == false) {						\
																		\
				break;													\
		}																\
																		\
				__task = __task->GetTaskBelowMe();						\
	}																	\
																		\
	if (__task)															\
	{																	\
																		\
		if (m_bot && __eventResult.IsRequestingChange() && m_bot->IsDebugging(BOTDEBUG_TASKS)) \
		{																						\
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, "%s ", m_bot->GetDebugIdentifier()); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, "%s ", __task->GetName()); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 0, "responded to EVENT %s with ", #EFUNC); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 0, 0, "%s %s ", __eventResult.GetTypeName(), __eventResult.GetNextTask() ? __eventResult.GetNextTask()->GetName() : ""); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 100, 255, 0, "result PRIORITY (%s) ", __eventResult.GetPriorityTypeName()); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 0, 255, 0, "(%s) \n", __eventResult.GetReason() ? __eventResult.GetReason() : ""); \
		}																\
																		\
		__task->UpdatePendingEventResult(__eventResult);				\
	}																	\
																		\
	IEventListener::EFUNC(ARG1, ARG2);									\

#define PROPAGATE_TASK_EVENT_WITH_3_ARGS(EFUNC, ARG1, ARG2, ARG3)		\
	if (m_hasStarted == false)											\
	{																	\
		return;															\
	}																	\
																		\
	AITask<BotClass>*__task = this;										\
	TaskEventResponseResult<BotClass> __eventResult;					\
																		\
	while (__task != nullptr)											\
	{																	\
		if (m_bot && m_bot->IsDebugging(BOTDEBUG_EVENTS))				\
		{																\
			m_bot->DebugPrintToConsole(BOTDEBUG_EVENTS, 100, 100, 100, "%s (%s) RECEIVED EVENT %s \n", m_bot->GetDebugIdentifier(), __task->GetName(), #EFUNC); \
		}																\
		__eventResult = __task->EFUNC(m_bot, ARG1, ARG2, ARG3);			\
																		\
		if (__eventResult.IsContinue() == false) {						\
																		\
				break;													\
		}																\
																		\
				__task = __task->GetTaskBelowMe();						\
	}																	\
																		\
	if (__task)															\
	{																	\
																		\
		if (m_bot && __eventResult.IsRequestingChange() && m_bot->IsDebugging(BOTDEBUG_TASKS)) \
		{																						\
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, "%s ", m_bot->GetDebugIdentifier()); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, "%s ", __task->GetName()); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 0, "responded to EVENT %s with ", #EFUNC); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 0, 0, "%s %s ", __eventResult.GetTypeName(), __eventResult.GetNextTask() ? __eventResult.GetNextTask()->GetName() : ""); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 100, 255, 0, "result PRIORITY (%s) ", __eventResult.GetPriorityTypeName()); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 0, 255, 0, "(%s) \n", __eventResult.GetReason() ? __eventResult.GetReason() : ""); \
		}																\
																		\
		__task->UpdatePendingEventResult(__eventResult);				\
	}																	\
																		\
	IEventListener::EFUNC(ARG1, ARG2, ARG3);							\

#define PROPAGATE_TASK_EVENT_WITH_4_ARGS(EFUNC, ARG1, ARG2, ARG3, ARG4)	\
	if (m_hasStarted == false)											\
	{																	\
		return;															\
	}																	\
																		\
	AITask<BotClass>*__task = this;										\
	TaskEventResponseResult<BotClass> __eventResult;					\
																		\
	while (__task != nullptr)											\
	{																	\
		if (m_bot && m_bot->IsDebugging(BOTDEBUG_EVENTS))				\
		{																\
			m_bot->DebugPrintToConsole(BOTDEBUG_EVENTS, 100, 100, 100, "%s (%s) RECEIVED EVENT %s \n", m_bot->GetDebugIdentifier(), __task->GetName(), #EFUNC); \
		}																\
		__eventResult = __task->EFUNC(m_bot, ARG1, ARG2, ARG3, ARG4);	\
																		\
		if (__eventResult.IsContinue() == false) {						\
																		\
				break;													\
		}																\
																		\
				__task = __task->GetTaskBelowMe();						\
	}																	\
																		\
	if (__task)															\
	{																	\
																		\
		if (m_bot && __eventResult.IsRequestingChange() && m_bot->IsDebugging(BOTDEBUG_TASKS)) \
		{																						\
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, "%s ", m_bot->GetDebugIdentifier()); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, "%s ", __task->GetName()); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 0, "responded to EVENT %s with ", #EFUNC); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 0, 0, "%s %s ", __eventResult.GetTypeName(), __eventResult.GetNextTask() ? __eventResult.GetNextTask()->GetName() : ""); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 100, 255, 0, "result PRIORITY (%s) ", __eventResult.GetPriorityTypeName()); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 0, 255, 0, "(%s) \n", __eventResult.GetReason() ? __eventResult.GetReason() : ""); \
		}																\
																		\
		__task->UpdatePendingEventResult(__eventResult);				\
	}																	\
																		\
	IEventListener::EFUNC(ARG1, ARG2, ARG3, ARG4);						\

#define PROPAGATE_TASK_EVENT_WITH_5_ARGS(EFUNC, ARG1, ARG2, ARG3, ARG4, ARG5)	\
	if (m_hasStarted == false)											\
	{																	\
		return;															\
	}																	\
																		\
	AITask<BotClass>*__task = this;										\
	TaskEventResponseResult<BotClass> __eventResult;					\
																		\
	while (__task != nullptr)											\
	{																	\
		if (m_bot && m_bot->IsDebugging(BOTDEBUG_EVENTS))				\
		{																\
			m_bot->DebugPrintToConsole(BOTDEBUG_EVENTS, 100, 100, 100, "%s (%s) RECEIVED EVENT %s \n", m_bot->GetDebugIdentifier(), __task->GetName(), #EFUNC); \
		}																\
		__eventResult = __task->EFUNC(m_bot, ARG1, ARG2, ARG3, ARG4, ARG5);	\
																		\
		if (__eventResult.IsContinue() == false) {						\
																		\
				break;													\
		}																\
																		\
				__task = __task->GetTaskBelowMe();						\
	}																	\
																		\
	if (__task)															\
	{																	\
																		\
		if (m_bot && __eventResult.IsRequestingChange() && m_bot->IsDebugging(BOTDEBUG_TASKS)) \
		{																						\
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, "%s ", m_bot->GetDebugIdentifier()); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, "%s ", __task->GetName()); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 0, "responded to EVENT %s with ", #EFUNC); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 0, 0, "%s %s ", __eventResult.GetTypeName(), __eventResult.GetNextTask() ? __eventResult.GetNextTask()->GetName() : ""); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 100, 255, 0, "result PRIORITY (%s) ", __eventResult.GetPriorityTypeName()); \
			m_bot->DebugPrintToConsole(BOTDEBUG_TASKS, 0, 255, 0, "(%s) \n", __eventResult.GetReason() ? __eventResult.GetReason() : ""); \
		}																\
																		\
		__task->UpdatePendingEventResult(__eventResult);				\
	}																	\
																		\
	IEventListener::EFUNC(ARG1, ARG2, ARG3, ARG4, ARG5);				\


	virtual std::vector<IEventListener*>* GetListenerVector() override final
	{
		if (m_nextTask == nullptr)
		{
			return nullptr;
		}

		// Next task pointers might change so always refresh the list before sending
		m_listener.clear();
		m_listener.push_back(m_nextTask);

		return &m_listener;
	}

	// If any task below me is done or switching to another task, then I am obsolete.
	bool IsObsolete()
	{
		for (AITask<BotClass>* task = GetTaskBelowMe(); task != nullptr; task = task->GetTaskBelowMe())
		{
			if (task->m_pendingEventResult.IsDone() || task->m_pendingEventResult.IsSwitch())
			{
				return true;
			}
		}

		return false;
	}

	void ClearPendingEventResult() const
	{
		m_pendingEventResult = TryContinue(PRIORITY_IGNORED);
	}

	TaskResult<BotClass> ProcessPendingEvent()
	{
		if (m_pendingEventResult.IsRequestingChange())
		{
			TaskResult<BotClass> result(m_pendingEventResult.GetType(), m_pendingEventResult.GetNextTask(), m_pendingEventResult.GetReason());

			// clear event result
			ClearPendingEventResult();

			return result;
		}

		AITask<BotClass>* below = GetTaskBelowMe();

		while (below != nullptr)
		{
			if (below->m_pendingEventResult.IsPause())
			{
				TaskResult<BotClass> result(below->m_pendingEventResult.GetType(), below->m_pendingEventResult.GetNextTask(), below->m_pendingEventResult.GetReason());
				// clear the pending result of the task below me
				below->ClearPendingEventResult();
				return result;
			}

			below = below->GetTaskBelowMe();
		}

		return Continue();
	}

	void UpdatePendingEventResult(TaskEventResponseResult<BotClass>& result)
	{
		if (result.IsContinue() == true)
		{
			return;
		}

		if (result.GetPriority() >= m_pendingEventResult.GetPriority())
		{
			if (m_pendingEventResult.GetPriority() == PRIORITY_MANDATORY)
			{
				throw std::runtime_error("PRIORITY_MANDATORY collision!");
			}

			// We received a new result with higher priority, discard old result
			if (m_pendingEventResult.GetNextTask() != nullptr)
			{
				m_pendingEventResult.DiscardResult();
			}

			m_pendingEventResult = result;
		}
		else // New result has lower priority than the current one, just discard it
		{
			result.DiscardResult();
		}
	}

	AITask<BotClass>* RunTask(BotClass* bot, AITaskManager<BotClass>* manager, TaskResult<BotClass> result);

	TaskResult<BotClass> ProcessTaskStart(BotClass* bot, AITaskManager<BotClass>* manager, AITask<BotClass>* pastTask, AITask<BotClass>* belowTask);
	TaskResult<BotClass> ProcessTaskUpdate(BotClass* bot, AITaskManager<BotClass>* manager);
	void ProcessTaskEnd(BotClass* bot, AITaskManager<BotClass>* manager, AITask<BotClass>* nextTask);
	AITask<BotClass>* ProcessTaskPause(BotClass* bot, AITaskManager<BotClass>* manager, AITask<BotClass>* task);
	TaskResult<BotClass> ProcessTaskResume(BotClass* bot, AITaskManager<BotClass>* manager, AITask<BotClass>* task);

	virtual void OnTestEventPropagation() override final
	{
		PROPAGATE_TASK_EVENT_WITH_NO_ARGS(OnTestEventPropagation);
	}

	virtual void OnStuck() override final
	{
		PROPAGATE_TASK_EVENT_WITH_NO_ARGS(OnStuck);
	}

	virtual void OnUnstuck() override final
	{
		PROPAGATE_TASK_EVENT_WITH_NO_ARGS(OnUnstuck);
	}

	virtual void OnMoveToFailure(CPath* path, IEventListener::MovementFailureType reason) override final
	{
		PROPAGATE_TASK_EVENT_WITH_2_ARGS(OnMoveToFailure, path, reason);
	}

	virtual void OnMoveToSuccess(CPath* path) override final
	{
		PROPAGATE_TASK_EVENT_WITH_1_ARGS(OnMoveToSuccess, path);
	}

	virtual void OnInjured(edict_t* attacker = nullptr) override final
	{
		PROPAGATE_TASK_EVENT_WITH_1_ARGS(OnInjured, attacker);
	}

	virtual void OnKilled(edict_t* attacker = nullptr) override final
	{
		PROPAGATE_TASK_EVENT_WITH_1_ARGS(OnKilled, attacker);
	}

	virtual void OnOtherKilled(edict_t* victim, edict_t* attacker = nullptr) override final
	{
		PROPAGATE_TASK_EVENT_WITH_2_ARGS(OnOtherKilled, victim, attacker);
	}

	virtual void OnSight(edict_t* subject) override final
	{
		PROPAGATE_TASK_EVENT_WITH_1_ARGS(OnSight, subject);
	}

	virtual void OnLostSight(edict_t* subject) override final
	{
		PROPAGATE_TASK_EVENT_WITH_1_ARGS(OnLostSight, subject);
	}

	virtual void OnSound(edict_t* source, const Vector& position, SoundType type, const int volume) override final
	{
		PROPAGATE_TASK_EVENT_WITH_4_ARGS(OnSound, source, position, type, volume);
	}
};

template<typename BotClass>
inline AITask<BotClass>::AITask() : m_pendingEventResult(PRIORITY_IGNORED, TASK_CONTINUE, nullptr, nullptr)
{
	m_manager = nullptr;
	m_headTask = nullptr;
	m_nextTask = nullptr;
	m_aboveTask = nullptr;
	m_belowTask = nullptr;
	m_bot = nullptr;
	m_hasStarted = false;
	m_isPaused = false;
	m_listener.reserve(2);
}

template<typename BotClass>
inline AITask<BotClass>::~AITask()
{
	// If there is a task before me
	if (m_headTask)
	{
		if (m_headTask->m_nextTask == this)
		{
			// Update the next task of the task before me to a task that is below me
			m_headTask->m_nextTask = m_belowTask;
		}
	}

	AITask<BotClass>* iter = nullptr, * next = nullptr;
	for (iter = m_headTask; iter != nullptr; iter = next)
	{
		next = iter->m_belowTask;
		delete iter;
	}

	if (m_belowTask)
	{
		// We're going away and is no longer above our below task
		m_belowTask->m_aboveTask = nullptr;
	}

	if (m_aboveTask)
	{
		// Any task above me is also going away
		delete m_aboveTask;
		m_aboveTask = nullptr;
	}

	m_pendingEventResult.DiscardResult();
	m_listener.clear();
}

template<typename BotClass>
inline const char* AITask<BotClass>::DebugString() const
{
	static char szdebug[256];

	szdebug[0] = '\0';

	auto root = this;
	while (root->m_headTask)
	{
		root = root->m_headTask;
	}

	return BuildDebugString(szdebug, root);
}

template<typename BotClass>
inline char* AITask<BotClass>::BuildDebugString(char* name, const AITask<BotClass>* task) const
{
	constexpr auto size = 256;

	Q_strcat(name, task->GetName(), size);

	auto next = task->GetNextTask();
	if (next)
	{
		Q_strcat(name, "(", size);
		BuildDebugString(name, next);
		Q_strcat(name, ")", size);
	}

	auto below = task->GetTaskBelowMe();
	if (below)
	{
		Q_strcat(name, "<<", size);
		BuildDebugString(name, below);
	}

	return name;
}

template<typename BotClass>
inline TaskResult<BotClass> AITask<BotClass>::Continue() const
{
	return TaskResult<BotClass>(TASK_CONTINUE, nullptr, nullptr);
}

template<typename BotClass>
inline TaskResult<BotClass> AITask<BotClass>::SwitchTo(AITask<BotClass>* newTask, const char* reason) const
{
	return TaskResult<BotClass>(TASK_SWITCH, newTask, reason);
}

template<typename BotClass>
inline TaskResult<BotClass> AITask<BotClass>::PauseFor(AITask<BotClass>* newTask, const char* reason) const
{
	// Clear any pending events when pausing
	ClearPendingEventResult();
	return TaskResult<BotClass>(TASK_PAUSE, newTask, reason);
}

template<typename BotClass>
inline TaskResult<BotClass> AITask<BotClass>::Done(const char* reason) const
{
	return TaskResult<BotClass>(TASK_DONE, nullptr, reason);
}

template<typename BotClass>
inline TaskEventResponseResult<BotClass> AITask<BotClass>::TryContinue(EventResultPriorityType priority) const
{
	return TaskEventResponseResult<BotClass>(priority, TASK_CONTINUE, nullptr, nullptr);
}

template<typename BotClass>
inline TaskEventResponseResult<BotClass> AITask<BotClass>::TrySwitchTo(AITask<BotClass>* task, EventResultPriorityType priority, const char* reason) const
{
	return TaskEventResponseResult<BotClass>(priority, TASK_SWITCH, task, reason);
}

template<typename BotClass>
inline TaskEventResponseResult<BotClass> AITask<BotClass>::TryPauseFor(AITask<BotClass>* task, EventResultPriorityType priority, const char* reason) const
{
	return TaskEventResponseResult<BotClass>(priority, TASK_PAUSE, task, reason);
}

template<typename BotClass>
inline TaskEventResponseResult<BotClass> AITask<BotClass>::TryDone(EventResultPriorityType priority, const char* reason) const
{
	return TaskEventResponseResult<BotClass>(priority, TASK_DONE, nullptr, reason);
}

template<typename BotClass>
inline AITask<BotClass>* AITask<BotClass>::RunTask(BotClass* bot, AITaskManager<BotClass>* manager, TaskResult<BotClass> result)
{
	AITask<BotClass>* newTask = result.GetNextTask();

	switch (result.GetType())
	{
	case TASK_PAUSE:
	{
		AITask<BotClass>* topTask = this;
		while (topTask->m_aboveTask != nullptr)
		{
			topTask = topTask->m_aboveTask;
		}

		if (bot->IsDebugging(BOTDEBUG_TASKS))
		{
			bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, "%s ", bot->GetDebugIdentifier());
			bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, this->GetName());
			bot->DebugPrintToConsole(BOTDEBUG_TASKS, 150, 150, 200, " caused ");
			bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, topTask->GetName());
			bot->DebugPrintToConsole(BOTDEBUG_TASKS, 150, 150, 200, " to PAUSE for ");
			bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, newTask->GetName());

			if (result.GetReason())
			{
				bot->DebugPrintToConsole(BOTDEBUG_TASKS, 150, 255, 150, " (%s)\n", result.GetReason());
			}
			else
			{
				bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, "\n");
			}
		}

		// Pause the task above us
		topTask = topTask->ProcessTaskPause(bot, manager, newTask);

		// Start the new task
		TaskResult<BotClass> startresult = newTask->ProcessTaskStart(bot, manager, topTask, topTask);

		return newTask->RunTask(bot, manager, startresult);
	}
	case TASK_SWITCH:
	{
		if (newTask == nullptr)
		{
			throw std::runtime_error("TASK_SWITCH result with NULL new task!");
		}

		if (bot->IsDebugging(BOTDEBUG_TASKS))
		{
			bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, "%s ", bot->GetDebugIdentifier());

			if (this == newTask)
			{
				bot->DebugPrintToConsole(BOTDEBUG_TASKS, 254, 254, 51, "START TASK ");
				bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, this->GetName());
			}
			else
			{
				bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, this->GetName());
				bot->DebugPrintToConsole(BOTDEBUG_TASKS, 150, 150, 200, " SWITCH ");
				bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, newTask->GetName());
			}	

			if (result.GetReason())
			{
				bot->DebugPrintToConsole(BOTDEBUG_TASKS, 150, 255, 150, " (%s)\n", result.GetReason());
			}
			else
			{
				bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, "\n");
			}
		}

		// We're changing tasks, end the current one
		this->ProcessTaskEnd(bot, manager, newTask);
		// And start the new one
		TaskResult<BotClass> startresult = newTask->ProcessTaskStart(bot, manager, this, this->m_belowTask);

		if (this != newTask)
		{
			manager->NotifyTaskEnd(this);
		}

		return newTask->RunTask(bot, manager, startresult);
	}
	case TASK_DONE:
	{
		AITask<BotClass>* taskToResume = this->m_belowTask;
		this->ProcessTaskEnd(bot, manager, taskToResume);

		if (bot->IsDebugging(BOTDEBUG_TASKS))
		{
			bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, "%s ", bot->GetDebugIdentifier());

			if (taskToResume)
			{
				bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 0, 0, "TASK DONE ");
				bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, this->GetName());
				bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 165, 0, " RESUME ");
				bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, taskToResume->GetName());
			}
			else
			{
				bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 0, 0, "TASK DONE ");
				bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, this->GetName());
			}

			if (result.GetReason())
			{
				bot->DebugPrintToConsole(BOTDEBUG_TASKS, 150, 255, 150, " (%s)\n", result.GetReason());
			}
			else
			{
				bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, "\n");
			}
		}

		// No task to resume, all tasks ended
		if (taskToResume == nullptr)
		{
			manager->NotifyTaskEnd(this);
			return nullptr;
		}

		TaskResult<BotClass> resumeresult = taskToResume->ProcessTaskResume(bot, manager, this);

		// send me to the trash list
		manager->NotifyTaskEnd(this);

		return taskToResume->RunTask(bot, manager, resumeresult);
	}
	case TASK_CONTINUE:
	case TASK_MAINTAIN:
		return this;
	default:
		throw std::runtime_error("Unknown Task Result Type!");
		break;
	}

	return nullptr;
}

template<typename BotClass>
inline TaskResult<BotClass> AITask<BotClass>::ProcessTaskStart(BotClass* bot, AITaskManager<BotClass>* manager, AITask<BotClass>* pastTask, AITask<BotClass>* belowTask)
{
	m_hasStarted = true;
	m_bot = bot;
	m_manager = manager;

	if (bot->IsDebugging(BOTDEBUG_TASKS))
	{
		bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, "%s", bot->GetDebugIdentifier());
		bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 165, 0, " STARTING ");
		bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, GetName());
		bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, " \n");
	}

	// Maintain the task list
	if (pastTask)
	{
		m_headTask = pastTask->m_headTask;
	}

	if (m_headTask)
	{
		m_headTask->m_nextTask = this;
	}

	m_belowTask = belowTask;
	if (belowTask) // if there is a task below us, then place us on top of that task
	{
		m_belowTask->m_aboveTask = this;
	}

	m_aboveTask = nullptr;

	m_nextTask = InitialNextTask();
	if (m_nextTask)
	{
		// Build Task list
		m_nextTask->m_headTask = this;
	}

	// Start ourself
	TaskResult<BotClass> result = OnTaskStart(m_bot, pastTask);

	return result;
}

template<typename BotClass>
inline TaskResult<BotClass> AITask<BotClass>::ProcessTaskUpdate(BotClass* bot, AITaskManager<BotClass>* manager)
{
	if (IsObsolete())
	{
		return Done("Current task is obsolete");
	}

	if (m_hasStarted == false)
	{
		return SwitchTo(this, "Starting Task");
	}

	TaskResult<BotClass> eventResult = ProcessPendingEvent();
	// Answer to an event
	if (eventResult.IsContinue() == false)
	{
		return eventResult;
	}

	// If we have a next task, call update on it first
	if (m_nextTask)
	{
		m_nextTask = m_nextTask->RunTask(bot, manager, m_nextTask->ProcessTaskUpdate(bot, manager));
	}

	// Update me
	TaskResult<BotClass> myresult = OnTaskUpdate(bot);
	return myresult;
}

template<typename BotClass>
inline void AITask<BotClass>::ProcessTaskEnd(BotClass* bot, AITaskManager<BotClass>* manager, AITask<BotClass>* nextTask)
{
	if (m_hasStarted == false)
	{
		return; // we never started anyways
	}

	if (bot->IsDebugging(BOTDEBUG_TASKS))
	{
		bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, "%s", bot->GetDebugIdentifier());
		bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 0, 0, " ENDING ");
		bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, GetName());
		bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, " \n");
	}

	m_hasStarted = false;

	// notify the other tasks on the list that we're ending
	AITask<BotClass>* iter = nullptr, * next = nullptr;
	for (iter = m_headTask; iter != nullptr; iter = next)
	{
		next = iter->m_belowTask;
		iter->ProcessTaskEnd(bot, manager, nextTask);
	}

	for (AITask<BotClass>* next = m_nextTask; next != nullptr; next = m_nextTask->m_belowTask)
	{
		next->ProcessTaskEnd(bot, manager, nextTask);
	}

	// Call my OnEnd
	OnTaskEnd(bot, nextTask);

	// Also end any tasks above me
	if (m_aboveTask)
	{
		m_aboveTask->ProcessTaskEnd(bot, manager, nextTask);
	}
}

template<typename BotClass>
inline AITask<BotClass>* AITask<BotClass>::ProcessTaskPause(BotClass* bot, AITaskManager<BotClass>* manager, AITask<BotClass>* task)
{
	if (bot->IsDebugging(BOTDEBUG_TASKS))
	{
		bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, "%s", bot->GetDebugIdentifier());
		bot->DebugPrintToConsole(BOTDEBUG_TASKS, 150, 150, 200, " PAUSING ");
		bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, GetName());
		bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, " \n");
	}

	// Pause the next task on the list
	if (m_nextTask)
	{
		m_nextTask->ProcessTaskPause(bot, manager, task);
	}

	m_isPaused = true;

	bool shouldkeep = OnTaskPause(bot, task);

	// If OnTaskPause returns false, destroy this task
	if (shouldkeep == false)
	{
		// End me
		ProcessTaskEnd(bot, manager, nullptr);
		AITask<BotClass>* toreplace = m_belowTask;
		manager->NotifyTaskEnd(this);
		return toreplace;
	}

	// We're paused but still exists
	return this;
}

template<typename BotClass>
inline TaskResult<BotClass> AITask<BotClass>::ProcessTaskResume(BotClass* bot, AITaskManager<BotClass>* manager, AITask<BotClass>* task)
{
	if (m_isPaused == false)
	{
		return Continue(); // what?
	}

	if (m_pendingEventResult.IsRequestingChange())
	{
		// Don't resume since a pending event will make changes to us
		return Continue();
	}

	if (bot->IsDebugging(BOTDEBUG_TASKS))
	{
		bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, "%s", bot->GetDebugIdentifier());
		bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 165, 0, " RESUMING ");
		bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, GetName());
		bot->DebugPrintToConsole(BOTDEBUG_TASKS, 255, 255, 255, " \n");
	}

	m_isPaused = false;
	m_aboveTask = nullptr;

	if (m_headTask)
	{
		// Add ourselves back to the list
		m_headTask->m_nextTask = this;
	}

	// Tell the next tasks on my list to also resume
	if (m_nextTask)
	{
		m_nextTask->RunTask(bot, manager, m_nextTask->ProcessTaskResume(bot, manager, task));
	}

	// Actually resume me
	TaskResult<BotClass> result = OnTaskResume(bot, task);

	return result;
}

#endif // !SMNAV_BOT_TASK_SYSTEM_H_