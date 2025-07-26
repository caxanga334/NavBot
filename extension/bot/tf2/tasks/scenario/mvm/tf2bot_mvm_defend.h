#ifndef NAVBOT_TF2_BOT_MVM_DEFEND_TASKS_H_
#define NAVBOT_TF2_BOT_MVM_DEFEND_TASKS_H_

#include <sdkports/sdk_timers.h>
#include <sdkports/sdk_ehandle.h>
#include <bot/interfaces/path/meshnavigator.h>

class CTF2Bot;

class CTF2BotMvMDefendTask : public AITask<CTF2Bot>
{
public:
	CTF2BotMvMDefendTask();

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	const char* GetName() const override { return "MvMDefend"; }
private:
	CHandle<CBaseEntity> m_target;
	CMeshNavigator m_nav;
	CountdownTimer m_updateTargetTimer;
	CountdownTimer m_lookAtTimer;
	Vector m_targetPos;

	void UpdateTarget();
};

#endif // !NAVBOT_TF2_BOT_MVM_DEFEND_TASKS_H_