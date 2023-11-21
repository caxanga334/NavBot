#ifndef SMNAV_BOT_TASK_SYSTEM_H_
#define SMNAV_BOT_TASK_SYSTEM_H_
#pragma once

#include <vector>
#include <stdexcept>

#include "decisionquery.h"
#include "event_listener.h"

// Forward declaration
template <typename BotClass> class AITask;

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
	PRIORITY_DONT_CARE = 0,
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
			break;
		case TASK_PAUSE:
		case TASK_SWITCH:
		case TASK_DONE:
			return true;
			break;
		case TASK_MAINTAIN:
		case MAX_TASK_TYPES:
		default:
			return false;
			break;
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
			break;
		case TASK_PAUSE:
			return "PAUSE";
			break;
		case TASK_SWITCH:
			return "SWITCH";
			break;
		case TASK_DONE:
			return "DONE"
			break;
		case TASK_MAINTAIN:
			return "MAINTAIN";
			break;
		case MAX_TASK_TYPES:
		default:
			return "INVALID TASK TYPE";
			break;
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
		case PRIORITY_DONT_CARE:
			return "DONT CARE";
			break;
		case PRIORITY_LOW:
			return "LOW";
			break;
		case PRIORITY_MEDIUM:
			return "MEDIUM";
			break;
		case PRIORITY_HIGH:
			return "HIGH";
			break;
		case PRIORITY_CRITICAL:
			return "CRITICAL";
			break;
		case PRIORITY_MANDATORY:
			return "MANDATORY";
			break;
		case MAX_EVENT_RESULT_PRIORITY_TYPES:
		default:
			return "INVALID EVENT PRIORITY TYPE";
			break;
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

	bool IsRunningTasks() { return m_task != nullptr; }

	void Reset(AITask<BotClass>* task);
	void Update(BotClass* bot);

	// Notify that a task has ended, add it to the list of tasks for deallocation
	void NotifyTaskEnd(AITask<BotClass>* task) { m_taskbin.push_back(task); }

private:
	friend class AITask<BotClass>;

	AITask<BotClass>* m_task; // The first task in the list
	std::vector<AITask<BotClass>*> m_taskbin; // Trash bin for dead tasks
	BotClass* m_bot;

	void CleanUpDeadTasks();
};

template<typename BotClass>
inline AITaskManager<BotClass>::AITaskManager(AITask<BotClass>* initialTask)
{
	m_task = initialTask;
	m_taskbin.reserve(16);
	m_bot = nullptr;
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
}

template<typename BotClass>
inline void AITaskManager<BotClass>::Reset(AITask<BotClass>* task)
{
}

template<typename BotClass>
inline void AITaskManager<BotClass>::Update(BotClass* bot)
{
	if (IsRunningTasks() == false)
	{
		return;
	}

	if (m_bot == nullptr) { m_bot = bot; }

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

	// These functions are used to trigger a state change on a Task.

	TaskResult<BotClass> Continue() const;
	TaskResult<BotClass> SwitchTo(AITask<BotClass>* newTask, const char* reason) const;
	TaskResult<BotClass> PauseFor(AITask<BotClass>* newTask, const char* reason) const;
	TaskResult<BotClass> Done(const char* reason) const;

	virtual AITask<BotClass>* InitialParallelTask() { return nullptr; }

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
	bool m_hasStarted;
	bool m_isPaused;

	AITask<BotClass>* RunTask(BotClass* bot, AITaskManager<BotClass>* manager, TaskResult<BotClass> result);

	TaskResult<BotClass> ProcessTaskStart(BotClass* bot, AITaskManager<BotClass>* manager, AITask<BotClass>* pastTask, AITask<BotClass>* belowTask);
	TaskResult<BotClass> ProcessTaskUpdate(BotClass* bot, AITaskManager<BotClass>* manager);
	void ProcessTaskEnd(BotClass* bot, AITaskManager<BotClass>* manager, AITask<BotClass>* nextTask);
	AITask<BotClass>* ProcessTaskPause(BotClass* bot, AITaskManager<BotClass>* manager, AITask<BotClass>* task);
	TaskResult<BotClass> ProcessTaskResume(BotClass* bot, AITaskManager<BotClass>* manager, AITask<BotClass>* task);
};


template<typename BotClass>
inline AITask<BotClass>::AITask()
{
	m_manager = nullptr;
	m_headTask = nullptr;
	m_nextTask = nullptr;
	m_aboveTask = nullptr;
	m_belowTask = nullptr;
	m_bot = nullptr;
	m_hasStarted = false;
	m_isPaused = false;
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
	}
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
	return TaskResult<BotClass>(TASK_PAUSE, newTask, reason);
}

template<typename BotClass>
inline TaskResult<BotClass> AITask<BotClass>::Done(const char* reason) const
{
	return TaskResult<BotClass>(TASK_DONE, nullptr, reason);
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
		while (topTask != nullptr)
		{
			topTask = topTask->m_aboveTask;
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

	m_nextTask = InitialParallelTask();
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

	if (m_hasStarted == false)
	{
		return SwitchTo(this, "Starting Task");
	}

	// TO-DO: Handle events

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
	// Pause the next task on the list
	if (m_nextTask)
	{
		m_nextTask->ProcessTaskPause(bot, manager, task);
	}

	m_isPaused = true;

	bool shouldkeep = OnTaskPause(bot, task);

	// If OnTaskPause returns false, destroy this task
	if (shouldkeep == true)
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

	// TO-DO: Handle events

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