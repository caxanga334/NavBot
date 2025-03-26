#ifndef __NAVBOT_TF2BOT_SD_WAIT_FOR_FLAG_TASK_H_
#define __NAVBOT_TF2BOT_SD_WAIT_FOR_FLAG_TASK_H_

class CTF2BotSDWaitForFlagTask : public AITask<CTF2Bot>
{
public:
	CTF2BotSDWaitForFlagTask() :
		m_flag(nullptr)
	{
	}

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	const char* GetName() const override { return "WaitForAustralium"; }

private:
	CHandle<CBaseEntity> m_flag;
	CMeshNavigator m_nav;
	CountdownTimer m_repathTimer;
};

#endif // !__NAVBOT_TF2BOT_SD_WAIT_FOR_FLAG_TASK_H_
