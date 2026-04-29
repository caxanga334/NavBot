#ifndef __NAVBOT_CSS_BOT_PLANT_BOMB_TASK_H_
#define __NAVBOT_CSS_BOT_PLANT_BOMB_TASK_H_

class CCSSBotPlantBombTask : public AITask<CCSSBot>
{
public:
	/**
	 * @brief Checks if the plant bomb task is possible.
	 * @param[in] bot Bot that will plant the bomb.
	 * @param[out] plantspot Position to plant the bomb at.
	 * @return True if the task is possible, false otherwise.
	 */
	static bool IsPossible(CCSSBot* bot, Vector& plantspot);

	CCSSBotPlantBombTask(const Vector& plantspot);

	// TaskResult<CCSSBot> OnTaskStart(CCSSBot* bot, AITask<CCSSBot>* pastTask) override;
	TaskResult<CCSSBot> OnTaskUpdate(CCSSBot* bot) override;
	void OnTaskEnd(CCSSBot* bot, AITask<CCSSBot>* nextTask) override;

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override;
	QueryAnswerType ShouldSeekAndDestroy(CBaseBot* me, const CKnownEntity* them) override;
	QueryAnswerType ShouldRetreat(CBaseBot* me) override;
	QueryAnswerType ShouldAssistTeammate(CBaseBot* me, CBaseEntity* teammate) override;
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override;

	const char* GetName() const override { return "PlantC4"; }
private:
	Vector m_moveGoal;
	CMeshNavigator m_nav;
	bool m_isplanting;
	CountdownTimer m_timeout;
};

#endif // !__NAVBOT_CSS_BOT_PLANT_BOMB_TASK_H_
