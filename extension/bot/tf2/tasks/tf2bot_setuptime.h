#ifndef NAVBOT_TF2BOT_SETUP_TIME_TASK_H_
#define NAVBOT_TF2BOT_SETUP_TIME_TASK_H_

#include <bot/interfaces/path/chasenavigator.h>
#include <sdkports/sdk_ehandle.h>
#include <sdkports/sdk_timers.h>

class CTF2BotSetupTimeTask : public AITask<CTF2Bot>
{
public:
	CTF2BotSetupTimeTask()
	{
		m_action = IDLE;
	}

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	void OnTaskEnd(CTF2Bot* bot, AITask<CTF2Bot>* nextTask) override;

	const char* GetName() const override { return "SetupTime"; }

private:

	enum SetupActions
	{
		IDLE = 0,
		TAUNT,
		LOOK,
		STARE,
		MELEE,
		KILLBIND,

		MAX_ACTIONS
	};

	void SelectRandomTarget(CTF2Bot* me);

	SetupActions m_action;
	CountdownTimer m_nextActionTimer;
	CHandle<CBaseEntity> m_target;
	CChaseNavigator m_nav;
};

#endif // !NAVBOT_TF2BOT_SETUP_TIME_TASK_H_
