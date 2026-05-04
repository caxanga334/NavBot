#ifndef __NAVBOT_CSS_BOT_GUARD_HOSTAGE_TASK_H_
#define __NAVBOT_CSS_BOT_GUARD_HOSTAGE_TASK_H_

class CCSSBotGuardHostageTask : public AITask<CCSSBot>
{
public:
	CCSSBotGuardHostageTask(CBaseEntity* hostage);

	AITask<CCSSBot>* InitialNextTask(CCSSBot* bot) override;
	TaskResult<CCSSBot> OnTaskUpdate(CCSSBot* bot) override;

	const char* GetName() const override { return "GuardHostage"; }
private:
	CHandle<CBaseEntity> m_hostage;
};

#endif // !__NAVBOT_CSS_BOT_GUARD_HOSTAGE_TASK_H_
