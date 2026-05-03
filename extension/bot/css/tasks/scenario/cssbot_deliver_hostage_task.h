#ifndef __NAVBOT_CSS_BOT_DELIVER_HOSTAGE_TASK_H_
#define __NAVBOT_CSS_BOT_DELIVER_HOSTAGE_TASK_H_

class CCSSBotDeliverHostageTask : public AITask<CCSSBot>
{
public:
	TaskResult<CCSSBot> OnTaskStart(CCSSBot* bot, AITask<CCSSBot>* pastTask) override;
	TaskResult<CCSSBot> OnTaskUpdate(CCSSBot* bot) override;

	const char* GetName() const override { return "DeliverHostage"; }
private:
	Vector m_goal;
	bool m_didtouch;
	CMeshNavigator m_nav;
	CountdownTimer m_timeout;
};


#endif // !__NAVBOT_CSS_BOT_DELIVER_HOSTAGE_TASK_H_
