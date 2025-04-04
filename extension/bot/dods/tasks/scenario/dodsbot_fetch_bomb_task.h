#ifndef __NAVBOT_DODSBOT_FETCH_BOMB_TASK_H_
#define __NAVBOT_DODSBOT_FETCH_BOMB_TASK_H_

class CDoDSBotFetchBombTask : public AITask<CDoDSBot>
{
public:
	CDoDSBotFetchBombTask(AITask<CDoDSBot>* exitTask = nullptr);
	~CDoDSBotFetchBombTask() override;

	TaskResult<CDoDSBot> OnTaskStart(CDoDSBot* bot, AITask<CDoDSBot>* pastTask) override;
	TaskResult<CDoDSBot> OnTaskUpdate(CDoDSBot* bot) override;

	TaskEventResponseResult<CDoDSBot> OnMoveToSuccess(CDoDSBot* bot, CPath* path) override;

	const char* GetName() const override { return "FetchBomb"; }
private:
	Vector m_goal;
	CMeshNavigator m_nav;
	CountdownTimer m_repathtimer;
	AITask<CDoDSBot>* m_exittask;

	bool SelectNearestBombDispenser(CDoDSBot* bot);
};

#endif // !__NAVBOT_DODSBOT_FETCH_BOMB_TASK_H_
