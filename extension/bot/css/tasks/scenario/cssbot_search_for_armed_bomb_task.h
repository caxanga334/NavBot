#ifndef __NAVBOT_CSS_BOT_SEARCH_FOR_ARMED_BOMB_TASK_H_
#define __NAVBOT_CSS_BOT_SEARCH_FOR_ARMED_BOMB_TASK_H_

class CCSSBotSearchForArmedBombTask : public AITask<CCSSBot>
{
public:
	TaskResult<CCSSBot> OnTaskStart(CCSSBot* bot, AITask<CCSSBot>* pastTask) override;
	TaskResult<CCSSBot> OnTaskUpdate(CCSSBot* bot) override;
	TaskEventResponseResult<CCSSBot> OnMoveToSuccess(CCSSBot* bot, CPath* path) override;

	const char* GetName() const override { return "SearchForC4"; }
private:
	Vector m_goal;
	CMeshNavigator m_nav;
};


#endif // !__NAVBOT_CSS_BOT_SEARCH_FOR_ARMED_BOMB_TASK_H_
