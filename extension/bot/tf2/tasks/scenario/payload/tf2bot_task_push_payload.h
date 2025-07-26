#ifndef NAVBOT_TF2BOT_TASK_PUSH_PAYLOAD_H_
#define NAVBOT_TF2BOT_TASK_PUSH_PAYLOAD_H_

#include <bot/interfaces/path/meshnavigator.h>
#include <sdkports/sdk_ehandle.h>

class CTF2Bot;

class CTF2BotPushPayloadTask : public AITask<CTF2Bot>
{
public:
	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	TaskResult<CTF2Bot> OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;

	TaskEventResponseResult<CTF2Bot> OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason) override;
	TaskEventResponseResult<CTF2Bot> OnMoveToSuccess(CTF2Bot* bot, CPath* path) override;

	const char* GetName() const override { return "PushPayload"; }
private:
	CMeshNavigator m_nav;
	Vector m_goal;
	CountdownTimer m_updatePayloadTimer;
	CHandle<CBaseEntity> m_payload;

	CBaseEntity* GetPayload(CTF2Bot* bot);
};

#endif // !NAVBOT_TF2BOT_TASK_PUSH_PAYLOAD_H_
