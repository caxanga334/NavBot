#ifndef NAVBOT_BLACK_MESA_BOT_DEPLOY_TRIPMINES_TASK_H_
#define NAVBOT_BLACK_MESA_BOT_DEPLOY_TRIPMINES_TASK_H_

class CBlackMesaBot;

class CBlackMesaBotDeployTripminesTask : public AITask<CBlackMesaBot>
{
public:
	CBlackMesaBotDeployTripminesTask(const Vector& pos);

	static bool IsPossible(CBlackMesaBot* bot, Vector& wallPos);

	TaskResult<CBlackMesaBot> OnTaskStart(CBlackMesaBot* bot, AITask<CBlackMesaBot>* pastTask) override;
	TaskResult<CBlackMesaBot> OnTaskUpdate(CBlackMesaBot* bot) override;

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_NO; }
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override { return ANSWER_NO; }

	const char* GetName() const override { return "DeployTripmines"; }
private:
	Vector m_wallPosition;
	float m_timeout;
};


#endif // !NAVBOT_BLACK_MESA_BOT_DEPLOY_TRIPMINES_TASK_H_
