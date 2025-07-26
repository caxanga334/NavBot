#ifndef __NAVBOT_TF2BOT_WAIT_FOR_JACK_TASK_H_
#define __NAVBOT_TF2BOT_WAIT_FOR_JACK_TASK_H_

class CTF2BotWaitForJackTask : public AITask<CTF2Bot>
{
public:

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	const char* GetName() const override { return "WaitForJack"; }

private:
	Vector m_goal;
	CMeshNavigator m_nav;
};

#endif // !__NAVBOT_TF2BOT_WAIT_FOR_JACK_TASK_H_
