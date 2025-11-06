#ifndef __NAVBOT_ZPSBOT_MAIN_TASK_H_
#define __NAVBOT_ZPSBOT_MAIN_TASK_H_

class CZPSBot;

#include <bot/interfaces/aim.h>

class CZPSBotMainTask : public AITask<CZPSBot>
{
public:
	CZPSBotMainTask() {}

	AITask<CZPSBot>* InitialNextTask(CZPSBot* bot) override;
	TaskResult<CZPSBot> OnTaskUpdate(CZPSBot* bot) override;

	const CKnownEntity* SelectTargetThreat(CBaseBot* me, const CKnownEntity* threat1, const CKnownEntity* threat2) override;
	Vector GetTargetAimPos(CBaseBot* me, CBaseEntity* entity, DesiredAimSpot desiredAim = AIMSPOT_NONE) override;
	QueryAnswerType ShouldPickup(CBaseBot* me, CBaseEntity* item) override;

	TaskEventResponseResult<CZPSBot> OnDebugMoveToCommand(CZPSBot* bot, const Vector& moveTo) override;
	TaskEventResponseResult<CZPSBot> OnKilled(CZPSBot* bot, const CTakeDamageInfo& info) override;

	const char* GetName() const override { return "MainTask"; }

private:
	IBotAimHelper<CZPSBot> m_aimhelper;
};


#endif // !__NAVBOT_ZPSBOT_MAIN_TASK_H_
