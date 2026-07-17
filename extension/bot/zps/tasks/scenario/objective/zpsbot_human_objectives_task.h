#ifndef __NAVBOT_ZPSBOT_TASKS_SCENARIO_HUMAN_OBJECTIVES_H_
#define __NAVBOT_ZPSBOT_TASKS_SCENARIO_HUMAN_OBJECTIVES_H_

class CZPSBotObjectiveFindItemTask : public AITask<CZPSBot>
{
public:
	AITask<CZPSBot>* InitialNextTask(CZPSBot* bot) override;

	TaskResult<CZPSBot> OnTaskStart(CZPSBot* bot, AITask<CZPSBot>* pastTask) override;
	TaskResult<CZPSBot> OnTaskUpdate(CZPSBot* bot) override;

	QueryAnswerType ShouldPickup(CBaseBot* me, CBaseEntity* item) override;

	const char* GetName() const override { return "ObjectiveFindItem"; }

private:
	CountdownTimer m_nextScanTimer;
	
	bool HasSpaceInInventory(CZPSBot* bot) const;
};

class CZPSBotObjectiveUseItemTask : public AITask<CZPSBot>
{
public:
	static bool IsPossible(CZPSBot* bot);

	TaskResult<CZPSBot> OnTaskStart(CZPSBot* bot, AITask<CZPSBot>* pastTask) override;
	TaskResult<CZPSBot> OnTaskUpdate(CZPSBot* bot) override;

	QueryAnswerType ShouldPickup(CBaseBot* me, CBaseEntity* item) override;
	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override { return m_usingItem ? ANSWER_UNDEFINED : ANSWER_NO; }
	QueryAnswerType ShouldHurry(CBaseBot* me) override { return m_usingItem ? ANSWER_UNDEFINED : ANSWER_YES; }
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override { return m_usingItem ? ANSWER_UNDEFINED : ANSWER_NO; }

	const char* GetName() const override { return "ObjectiveUseItem"; }

private:
	CMeshNavigator m_nav;
	bool m_usingItem;
	CountdownTimer m_switchDelay;

	bool IsItemEquipped(CZPSBot* bot) const;
	void EquipRequiredItem(CZPSBot* bot);
};


#endif // !__NAVBOT_ZPSBOT_TASKS_SCENARIO_HUMAN_OBJECTIVES_H_
