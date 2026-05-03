#ifndef __NAVBOT_CSS_BOT_RESCUE_HOSTAGE_TASK_H_
#define __NAVBOT_CSS_BOT_RESCUE_HOSTAGE_TASK_H_

class CCSSBotRescueHostageTask : public AITask<CCSSBot>
{
public:
	CCSSBotRescueHostageTask(CBaseEntity* hostage);
	TaskResult<CCSSBot> OnTaskUpdate(CCSSBot* bot) override;

	const char* GetName() const override { return "RescueHostages"; }
private:
	CHandle<CBaseEntity> m_hostage;
	CMeshNavigator m_nav;
};

#endif // !__NAVBOT_CSS_BOT_RESCUE_HOSTAGE_TASK_H_
