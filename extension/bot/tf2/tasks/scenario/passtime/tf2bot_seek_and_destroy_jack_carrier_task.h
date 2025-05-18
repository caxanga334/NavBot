#ifndef __NAVBOT_TF2BOT_SEEK_AND_DESTROY_JACK_CARRIER_H_
#define __NAVBOT_TF2BOT_SEEK_AND_DESTROY_JACK_CARRIER_H_

class CTF2BotSeekAndDestroyJackCarrierTask : public AITask<CTF2Bot>
{
public:
	CTF2BotSeekAndDestroyJackCarrierTask();

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	const char* GetName() const override { return "SeekAndDestroyJackCarrier"; }

private:
	CChaseNavigator m_nav;
	CHandle<CBaseEntity> m_jack;
};

#endif // !__NAVBOT_TF2BOT_SEEK_AND_DESTROY_JACK_CARRIER_H_
