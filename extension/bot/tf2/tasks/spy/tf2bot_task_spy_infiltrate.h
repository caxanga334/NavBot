#ifndef NAVBOT_TF2_TASK_SPY_INFILTRATE_H_
#define NAVBOT_TF2_TASK_SPY_INFILTRATE_H_

#include <bot/interfaces/path/meshnavigator.h>

class CTF2Bot;

class CTF2BotSpyInfiltrateTask : public AITask<CTF2Bot>
{
public:

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	const char* GetName() const override { return "SpyInfiltrate"; }

private:
	CMeshNavigator m_nav;
	Vector m_goal;
	CountdownTimer m_disguiseCooldown;

	void DisguiseMe(CTF2Bot* me);
};

#endif // !NAVBOT_TF2_TASK_SPY_INFILTRATE_H_



