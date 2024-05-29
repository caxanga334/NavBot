#ifndef NAVBOT_TF2BOT_SCENARIO_TASK_H_
#define NAVBOT_TF2BOT_SCENARIO_TASK_H_
#pragma once

#include <functional>
#include <string>
#include <sdkports/sdk_timers.h>
#include <sdkports/sdk_ehandle.h>

class CTF2Bot;

/**
 * @brief Shared tasks for picking up entities that are stored on the player's m_hItem
 */
class CTF2BotCollectItemTask : public AITask<CTF2Bot>
{
public:
	CTF2BotCollectItemTask(CBaseEntity* item, const bool failOnPause = false, const bool ignoreExisting = false);

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	bool OnTaskPause(CTF2Bot* bot, AITask<CTF2Bot>* nextTask) override;
	TaskResult<CTF2Bot> OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;

	TaskEventResponseResult<CTF2Bot> OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason) override;
	TaskEventResponseResult<CTF2Bot> OnMoveToSuccess(CTF2Bot* bot, CPath* path) override;

	// Overrides this task name (used in debugging).
	void SetNameOverride(const char* newName) { m_name.assign(newName); }
	const char* GetName() const override { return m_name.c_str(); }

	// Sets a custom validator function, this should return false to 'fail' the task.
	void SetValidatorFunction(std::function<bool(CTF2Bot*, CBaseEntity*)> F) { m_validationFunc = F; }
	// Sets a custom get position function, returns a Vector to where the bot will path to.
	void SetPositionFunction(std::function<Vector(CTF2Bot*, CBaseEntity*)> F) { m_positionFunc = F; }
	// Sets a custom goal reached function, this should return true to 'complete' the task.
	void SetGoalFunction(std::function<bool(CTF2Bot*, CBaseEntity*)> F) { m_goalFunc = F; }

private:
	CHandle<CBaseEntity> m_item; // item the bot will collect
	const bool m_failOnPause; // end this task if it's paused
	const bool m_ignoreExisting; // don't end this task if the bot already has an item on it's inventory
	std::function<bool(CTF2Bot*, CBaseEntity*)> m_validationFunc; // function to call if the object is valid
	std::function<Vector(CTF2Bot*, CBaseEntity*)> m_positionFunc; // function to call to get the object position
	std::function<bool(CTF2Bot*, CBaseEntity*)> m_goalFunc; // function to call to determine if the goal of the task was reached
	CMeshNavigator m_nav;
	Vector m_moveGoal;
	CountdownTimer m_repathTimer;
	std::string m_name; // task name
	uint8_t m_moveFailures;

	bool DefaultValidator(CTF2Bot* bot);
};

#endif // !NAVBOT_TF2BOT_SCENARIO_TASK_H_
