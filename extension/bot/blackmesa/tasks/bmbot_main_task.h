#ifndef NAVBOT_BLACK_MESA_BOT_MAIN_TASK_H_
#define NAVBOT_BLACK_MESA_BOT_MAIN_TASK_H_

class CBlackMesaBot;

#include <bot/interfaces/aim.h>

class CBlackMesaBotMainTask : public AITask<CBlackMesaBot>
{
public:
	CBlackMesaBotMainTask();

	AITask<CBlackMesaBot>* InitialNextTask(CBlackMesaBot* bot) override;
	TaskResult<CBlackMesaBot> OnTaskUpdate(CBlackMesaBot* bot) override;

	TaskEventResponseResult<CBlackMesaBot> OnDebugMoveToCommand(CBlackMesaBot* bot, const Vector& moveTo) override;
	TaskEventResponseResult<CBlackMesaBot> OnKilled(CBlackMesaBot* bot, const CTakeDamageInfo& info) override;

	Vector GetTargetAimPos(CBaseBot* me, CBaseEntity* entity, DesiredAimSpot desiredAim = AIMSPOT_NONE) override;
	const CKnownEntity* SelectTargetThreat(CBaseBot* me, const CKnownEntity* threat1, const CKnownEntity* threat2) override;

	const char* GetName() const override { return "MainTask"; }
private:
	IBotAimHelper<CBlackMesaBot> m_aimhelper;
};

#endif // !NAVBOT_BLACK_MESA_BOT_MAIN_TASK_H_
