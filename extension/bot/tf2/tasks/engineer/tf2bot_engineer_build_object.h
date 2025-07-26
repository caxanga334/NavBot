#ifndef NAVBOT_TF2BOT_TASKS_ENGINEER_BUILD_OBJECT_H_
#define NAVBOT_TF2BOT_TASKS_ENGINEER_BUILD_OBJECT_H_

#include <string>

#include <sdkports/sdk_timers.h>
#include <bot/interfaces/path/meshnavigator.h>

class CTFWaypoint;

class CTF2BotEngineerBuildObjectTask : public AITask<CTF2Bot>
{
public:
	enum eObjectType
	{
		OBJECT_SENTRYGUN = 0,
		OBJECT_DISPENSER,
		OBJECT_TELEPORTER_ENTRANCE,
		OBJECT_TELEPORTER_EXIT,

		MAX_OBJECT_TYPES
	};

	CTF2BotEngineerBuildObjectTask(eObjectType type, const CTFWaypoint* waypoint);
	CTF2BotEngineerBuildObjectTask(eObjectType type, const Vector& goal);

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	void OnTaskEnd(CTF2Bot* bot, AITask<CTF2Bot>* nextTask) override;

	TaskEventResponseResult<CTF2Bot> OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason) override;
	TaskEventResponseResult<CTF2Bot> OnMoveToSuccess(CTF2Bot* bot, CPath* path) override;

	QueryAnswerType ShouldSeekAndDestroy(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_NO; }
	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override
	{
		if (m_reachedGoal)
		{
			return ANSWER_NO;
		}

		return ANSWER_UNDEFINED;
	}

	const char* GetName() const override { return m_taskname.c_str(); }

private:
	const CTFWaypoint* m_waypoint;
	Vector m_goal;
	QAngle m_lookangles;
	CountdownTimer m_giveupTimer;
	CountdownTimer m_strafeTimer;
	CMeshNavigator m_nav;
	eObjectType m_type;
	std::string m_taskname;
	bool m_reachedGoal;
	bool m_hasangles;
	int m_trydir;
};


#endif // !NAVBOT_TF2BOT_TASKS_ENGINEER_BUILD_OBJECT_H_
