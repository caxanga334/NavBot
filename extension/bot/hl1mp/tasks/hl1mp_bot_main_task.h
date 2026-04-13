#ifndef __NAVBOT_HL1MP_BOT_MAIN_TASK_H_
#define __NAVBOT_HL1MP_BOT_MAIN_TASK_H_

class CHL1MPBot;

#include <bot/interfaces/aim.h>

class CHL1MPBotMainTask : public AITask<CHL1MPBot>
{
public:

	AITask<CHL1MPBot>* InitialNextTask(CHL1MPBot* bot) override;

	TaskResult<CHL1MPBot> OnTaskUpdate(CHL1MPBot* bot) override;

	TaskEventResponseResult<CHL1MPBot> OnDebugMoveToCommand(CHL1MPBot* bot, const Vector& moveTo) override;
	TaskEventResponseResult<CHL1MPBot> OnKilled(CHL1MPBot* bot, const CTakeDamageInfo& info) override;

	Vector GetTargetAimPos(CBaseBot* me, CBaseEntity* entity, DesiredAimSpot desiredAim = AIMSPOT_NONE) override;
	const CKnownEntity* SelectTargetThreat(CBaseBot* me, const CKnownEntity* threat1, const CKnownEntity* threat2) override;

	const char* GetName() const override { return "MainTask"; }
private:
	IBotAimHelper<CHL1MPBot> m_aimhelper;
};


#endif // !__NAVBOT_HL1MP_BOT_MAIN_TASK_H_
