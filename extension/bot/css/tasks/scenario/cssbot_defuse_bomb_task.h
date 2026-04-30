#ifndef __NAVBOT_CSS_BOT_DEFUSE_BOMB_TASK_H_
#define __NAVBOT_CSS_BOT_DEFUSE_BOMB_TASK_H_

class CCSSBotDefuseBombTask : public AITask<CCSSBot>
{
public:
	TaskResult<CCSSBot> OnTaskStart(CCSSBot* bot, AITask<CCSSBot>* pastTask) override;
	TaskResult<CCSSBot> OnTaskUpdate(CCSSBot* bot) override;

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override;
	QueryAnswerType ShouldSeekAndDestroy(CBaseBot* me, const CKnownEntity* them) override;
	QueryAnswerType ShouldRetreat(CBaseBot* me) override;
	QueryAnswerType ShouldAssistTeammate(CBaseBot* me, CBaseEntity* teammate) override;
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override;

	const char* GetName() const override { return "DefuseBomb"; }
private:
	bool m_hasdefuser;
	bool m_defusing;
	CMeshNavigator m_nav;
};

#endif // !__NAVBOT_CSS_BOT_DEFUSE_BOMB_TASK_H_
