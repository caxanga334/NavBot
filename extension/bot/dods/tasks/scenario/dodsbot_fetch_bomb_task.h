#ifndef __NAVBOT_DODSBOT_FETCH_BOMB_TASK_H_
#define __NAVBOT_DODSBOT_FETCH_BOMB_TASK_H_

class CDoDSBotFetchBombTask : public AITask<CDoDSBot>
{
public:
	CDoDSBotFetchBombTask(AITask<CDoDSBot>* exitTask = nullptr, CBaseEntity* dispenser = nullptr);
	~CDoDSBotFetchBombTask() override;

	static bool IsPossible(CDoDSBot* bot, CBaseEntity** dispenser);

	TaskResult<CDoDSBot> OnTaskStart(CDoDSBot* bot, AITask<CDoDSBot>* pastTask) override;
	TaskResult<CDoDSBot> OnTaskUpdate(CDoDSBot* bot) override;

	TaskEventResponseResult<CDoDSBot> OnMoveToSuccess(CDoDSBot* bot, CPath* path) override;

	const char* GetName() const override { return "FetchBomb"; }
private:
	Vector m_goal;
	CMeshNavigator m_nav;
	AITask<CDoDSBot>* m_exittask;
	CHandle<CBaseEntity> m_dispenser;
};

#endif // !__NAVBOT_DODSBOT_FETCH_BOMB_TASK_H_
