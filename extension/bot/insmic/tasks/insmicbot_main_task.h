#ifndef __NAVBOT_INSMICBOT_MAIN_TASK_H_
#define __NAVBOT_INSMICBOT_MAIN_TASK_H_

class CInsMICBot;

#include <bot/interfaces/aim.h>

class CInsMICBotMainTask : public AITask<CInsMICBot>
{
public:
	CInsMICBotMainTask() {}

	AITask<CInsMICBot>* InitialNextTask(CInsMICBot* bot) override;
	TaskResult<CInsMICBot> OnTaskUpdate(CInsMICBot* bot) override;

	const CKnownEntity* SelectTargetThreat(CBaseBot* me, const CKnownEntity* threat1, const CKnownEntity* threat2) override;
	Vector GetTargetAimPos(CBaseBot* me, CBaseEntity* entity, DesiredAimSpot desiredAim = AIMSPOT_NONE) override;

	TaskEventResponseResult<CInsMICBot> OnDebugMoveToCommand(CInsMICBot* bot, const Vector& moveTo) override;
	TaskEventResponseResult<CInsMICBot> OnKilled(CInsMICBot* bot, const CTakeDamageInfo& info) override;

	const char* GetName() const override { return "Main"; }

private:
	IBotAimHelper<CInsMICBot> m_aimhelper;
};


#endif // !__NAVBOT_INSMICBOT_MAIN_TASK_H_
