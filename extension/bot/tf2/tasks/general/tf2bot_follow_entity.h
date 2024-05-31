#ifndef NAVBOT_TF2BOT_FOLLOW_ENTITY_TASK_H_
#define NAVBOT_TF2BOT_FOLLOW_ENTITY_TASK_H_

#include <functional>
#include <string>
#include <sdkports/sdk_timers.h>
#include <sdkports/sdk_ehandle.h>
#include <bot/interfaces/path/meshnavigator.h>

class CTF2Bot;

class CTF2BotFollowEntityTask : public AITask<CTF2Bot>
{
public:
	CTF2BotFollowEntityTask(CBaseEntity* entity, const float updateRate = 0.5f, const float followRange = 256.0f, const bool failOnPause = false);

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	bool OnTaskPause(CTF2Bot* bot, AITask<CTF2Bot>* nextTask) override;
	TaskResult<CTF2Bot> OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;

	TaskEventResponseResult<CTF2Bot> OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason) override;
	TaskEventResponseResult<CTF2Bot> OnMoveToSuccess(CTF2Bot* bot, CPath* path) override;

	const char* GetName() const override { return m_name.c_str(); }

	// Sets a custom validator function, this should return false to 'fail' the task.
	void SetValidatorFunction(std::function<bool(CTF2Bot*, CBaseEntity*)> F) { m_validationFunc = F; }
	// Sets a custom get position function, returns a Vector to where the bot will path to.
	void SetPositionFunction(std::function<Vector(CTF2Bot*, CBaseEntity*)> F) { m_positionFunc = F; }
	// Sets a custom goal reached function, this should return true to 'complete' the task.
	void SetGoalFunction(std::function<bool(CTF2Bot*, CBaseEntity*)> F) { m_goalFunc = F; }

private:
	CHandle<CBaseEntity> m_followtarget; // Entity the bot will follow
	Vector m_followgoal; // Vector of the bot move goal
	std::string m_name;
	CountdownTimer m_updateEntityPositionTimer; // Timer to update the entity's position
	std::function<bool(CTF2Bot*, CBaseEntity*)> m_validationFunc; // function to call if the entity is valid
	std::function<Vector(CTF2Bot*, CBaseEntity*)> m_positionFunc; // function to call to get the object position
	std::function<bool(CTF2Bot*, CBaseEntity*)> m_goalFunc; // function to call to determine if the goal of the task was reached
	CMeshNavigator m_nav;
	const float m_updaterate; // frequency of position and pathing updates
	const float m_followrange; // Distance to stop moving towards the entity
	bool m_isplayer; // is the entity being follow a player?
	bool m_failOnPause; // 'fail' the task on pause?
	uint8_t m_pathfailures;

	bool ValidateFollowEntity(CTF2Bot* me);
	void GetEntityPosition(CTF2Bot*me, Vector& out);
};

#endif // !NAVBOT_TF2BOT_FOLLOW_ENTITY_TASK_H_
