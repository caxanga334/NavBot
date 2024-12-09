#ifndef NAVBOT_TF2_ROAM_TASK_H_
#define NAVBOT_TF2_ROAM_TASK_H_

#include <bot/interfaces/path/meshnavigator.h>
#include <sdkports/sdk_timers.h>

class CTF2Bot;

class CTF2BotRoamTask : public AITask<CTF2Bot>
{
public:
	CTF2BotRoamTask() : m_hasgoal(false), m_failcount(0) {}
	CTF2BotRoamTask(const Vector& goal);

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	TaskResult<CTF2Bot> OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;

	TaskEventResponseResult<CTF2Bot> OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason) override;
	TaskEventResponseResult<CTF2Bot> OnMoveToSuccess(CTF2Bot* bot, CPath* path) override;

	const char* GetName() const override { return "Roaming"; }
private:
	CMeshNavigator m_nav;
	Vector m_goal;
	CountdownTimer m_repathtimer;
	bool m_hasgoal;
	uint8_t m_failcount;

	bool FindRandomGoalPosition(CTF2Bot* me);
};

#endif // !NAVBOT_TF2_ROAM_TASK_H_
