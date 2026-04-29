#ifndef __NAVBOT_CSS_BOT_MAIN_TASK_H_
#define __NAVBOT_CSS_BOT_MAIN_TASK_H_

class CCSSBot;

#include <bot/interfaces/aim.h>

class CCSSBotMainTask : public AITask<CCSSBot>
{
public:
	AITask<CCSSBot>* InitialNextTask(CCSSBot* bot) override;

	TaskResult<CCSSBot> OnTaskUpdate(CCSSBot* bot) override;

	TaskEventResponseResult<CCSSBot> OnDebugMoveToCommand(CCSSBot* bot, const Vector& moveTo) override;
	TaskEventResponseResult<CCSSBot> OnKilled(CCSSBot* bot, const CTakeDamageInfo& info) override;

	Vector GetTargetAimPos(CBaseBot* me, CBaseEntity* entity, DesiredAimSpot desiredAim = AIMSPOT_NONE) override;
	const CKnownEntity* SelectTargetThreat(CBaseBot* me, const CKnownEntity* threat1, const CKnownEntity* threat2) override;

	const char* GetName() const override { return "MainTask"; }

private:
	IBotAimHelper<CCSSBot> m_aim;
};


#endif // !__NAVBOT_CSS_BOT_MAIN_TASK_H_
