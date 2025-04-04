#ifndef __NAVBOT_DODSBOT_MAIN_TASK_H_
#define __NAVBOT_DODSBOT_MAIN_TASK_H_

class CDoDSBotMainTask : public AITask<CDoDSBot>
{
public:

	AITask<CDoDSBot>* InitialNextTask(CDoDSBot* bot) override;

	TaskResult<CDoDSBot> OnTaskUpdate(CDoDSBot* bot) override;

	TaskEventResponseResult<CDoDSBot> OnDebugMoveToHostCommand(CDoDSBot* bot);

	Vector GetTargetAimPos(CBaseBot* me, CBaseEntity* entity, DesiredAimSpot desiredAim = AIMSPOT_NONE) override;
	const CKnownEntity* SelectTargetThreat(CBaseBot* me, const CKnownEntity* threat1, const CKnownEntity* threat2) override;

	const char* GetName() const override { return "MainTask"; }
};


#endif // !__NAVBOT_DODSBOT_MAIN_TASK_H_
