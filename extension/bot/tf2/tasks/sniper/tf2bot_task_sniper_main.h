#ifndef __NAVBOT_TF2BOT_SNIPER_MAIN_TASK_H_
#define __NAVBOT_TF2BOT_SNIPER_MAIN_TASK_H_

class CTF2BotSniperMainTask : public AITask<CTF2Bot>
{
public:
	CTF2BotSniperMainTask();

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	// Don't chase enemies
	QueryAnswerType ShouldSeekAndDestroy(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_NO; }

	const char* GetName() const override { return "SniperMain"; }
private:

};


#endif // !__NAVBOT_TF2BOT_SNIPER_MAIN_TASK_H_
