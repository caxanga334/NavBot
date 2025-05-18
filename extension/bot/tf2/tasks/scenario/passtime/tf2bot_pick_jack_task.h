#ifndef __NAVBOT_TF2BOT_PICK_JACK_TASK_H_
#define __NAVBOT_TF2BOT_PICK_JACK_TASK_H_

class CTF2BotPickJackTask : public AITask<CTF2Bot>
{
public:

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	const char* GetName() const override { return "PickJack"; }

private:
	CMeshNavigatorAutoRepath m_nav;
	CHandle<CBaseEntity> m_jack;
};

#endif // !__NAVBOT_TF2BOT_PICK_JACK_TASK_H_
