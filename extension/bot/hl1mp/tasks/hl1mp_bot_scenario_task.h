#ifndef __NAVBOT_HL1MPBOT_SCENARIO_TASK_H_
#define __NAVBOT_HL1MPBOT_SCENARIO_TASK_H_

class CHL1MPBotScenarioTask : public AITask<CHL1MPBot>
{
public:

	AITask<CHL1MPBot>* InitialNextTask(CHL1MPBot* bot) override;
	TaskResult<CHL1MPBot> OnTaskStart(CHL1MPBot* bot, AITask<CHL1MPBot>* pastTask) override;
	TaskResult<CHL1MPBot> OnTaskUpdate(CHL1MPBot* bot) override;

	QueryAnswerType ShouldPickup(CBaseBot* me, CBaseEntity* item) override;

	const char* GetName() const override { return "Scenario"; }

private:
	CountdownTimer m_pickupWeaponsTimer;
	CountdownTimer m_pickupAmmoTimer;
};

#endif // !__NAVBOT_HL1MPBOT_SCENARIO_TASK_H_