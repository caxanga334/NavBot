#ifndef __NAVBOT_CSS_BOT_GUARD_RESCUE_ZONE_TASK_H_
#define __NAVBOT_CSS_BOT_GUARD_RESCUE_ZONE_TASK_H_

class CCSSBotGuardRescueZoneTask : public AITask<CCSSBot>
{
public:
	AITask<CCSSBot>* InitialNextTask(CCSSBot* bot) override;
	TaskResult<CCSSBot> OnTaskUpdate(CCSSBot* bot) override;

	const char* GetName() const override { return "GuardRescueZone"; }
};

#endif // !__NAVBOT_CSS_BOT_GUARD_RESCUE_ZONE_TASK_H_
