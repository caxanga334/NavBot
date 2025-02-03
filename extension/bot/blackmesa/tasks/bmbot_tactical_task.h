#ifndef NAVBOT_BLACK_MESA_BOT_TACTICAL_TASK_H_
#define NAVBOT_BLACK_MESA_BOT_TACTICAL_TASK_H_

#include <sdkports/sdk_timers.h>

class CBlackMesaBot;

class CBlackMesaBotTacticalTask : public AITask<CBlackMesaBot>
{
public:
	AITask<CBlackMesaBot>* InitialNextTask(CBlackMesaBot* bot) override;

	TaskResult<CBlackMesaBot> OnTaskStart(CBlackMesaBot* bot, AITask<CBlackMesaBot>* pastTask) override;
	TaskResult<CBlackMesaBot> OnTaskUpdate(CBlackMesaBot* bot) override;
	TaskResult<CBlackMesaBot> OnTaskResume(CBlackMesaBot* bot, AITask<CBlackMesaBot>* pastTask) override;

	QueryAnswerType ShouldSeekAndDestroy(CBaseBot* me, const CKnownEntity* them) override;

	const char* GetName() const override { return "TacticalMonitor"; }
private:
	CountdownTimer m_healthScanTimer;
	CountdownTimer m_deployTripminesTimer;
};

#endif // !NAVBOT_BLACK_MESA_BOT_TACTICAL_TASK_H_
