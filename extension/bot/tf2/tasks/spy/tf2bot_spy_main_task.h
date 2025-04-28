#ifndef __NAVBOT_TF2BOT_SPY_MAIN_TASK_H_
#define __NAVBOT_TF2BOT_SPY_MAIN_TASK_H_

class CTF2BotSpyMainTask : public AITask<CTF2Bot>
{
public:
	AITask<CTF2Bot>* InitialNextTask(CTF2Bot* bot) override;

	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;


	const char* GetName() const override { return "SpyMain"; }
private:

};


#endif // !__NAVBOT_TF2BOT_SPY_MAIN_TASK_H_
