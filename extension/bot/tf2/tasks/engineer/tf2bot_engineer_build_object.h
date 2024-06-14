#ifndef NAVBOT_TF2BOT_TASKS_ENGINEER_BUILD_OBJECT_H_
#define NAVBOT_TF2BOT_TASKS_ENGINEER_BUILD_OBJECT_H_

#include <string>

#include <sdkports/sdk_timers.h>
#include <bot/interfaces/path/meshnavigator.h>

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

	CTF2BotEngineerBuildObjectTask(eObjectType type, const Vector& location);

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	TaskEventResponseResult<CTF2Bot> OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason) override;
	TaskEventResponseResult<CTF2Bot> OnMoveToSuccess(CTF2Bot* bot, CPath* path) override;

	QueryAnswerType ShouldSeekAndDestroy(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_NO; }

	const char* GetName() const override { return m_taskname.c_str(); }

private:
	Vector m_goal;
	CountdownTimer m_repathTimer;
	CountdownTimer m_giveupTimer;
	CountdownTimer m_strafeTimer;
	CMeshNavigator m_nav;
	eObjectType m_type;
	std::string m_taskname;
	bool m_reachedGoal;
	int m_trydir;
};


#endif // !NAVBOT_TF2BOT_TASKS_ENGINEER_BUILD_OBJECT_H_
