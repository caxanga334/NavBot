#ifndef __NAVBOT_HL1MPBOT_TACTICAL_TASK_H_
#define __NAVBOT_HL1MPBOT_TACTICAL_TASK_H_

class CHL1MPBotTacticalTask : public AITask<CHL1MPBot>
{
public:
	CHL1MPBotTacticalTask();

	AITask<CHL1MPBot>* InitialNextTask(CHL1MPBot* bot) override;
	TaskResult<CHL1MPBot> OnTaskUpdate(CHL1MPBot* bot) override;
	TaskResult<CHL1MPBot> OnTaskResume(CHL1MPBot* bot, AITask<CHL1MPBot>* pastTask) override;

	QueryAnswerType ShouldRetreat(CBaseBot* me) override;

	const char* GetName() const override { return "Tactical"; }

private:
	CountdownTimer m_healthscantimer;
	CountdownTimer m_armorscantimer;
};

#endif // !__NAVBOT_HL1MPBOT_TACTICAL_TASK_H_
