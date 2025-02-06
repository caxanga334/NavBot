#ifndef NAVBOT_BLACK_MESA_BOT_MAIN_TASK_H_
#define NAVBOT_BLACK_MESA_BOT_MAIN_TASK_H_

class CBlackMesaBot;

class CBlackMesaBotMainTask : public AITask<CBlackMesaBot>
{
public:
	CBlackMesaBotMainTask();

	AITask<CBlackMesaBot>* InitialNextTask(CBlackMesaBot* bot) override;
	TaskResult<CBlackMesaBot> OnTaskUpdate(CBlackMesaBot* bot) override;

	TaskEventResponseResult<CBlackMesaBot> OnTestEventPropagation(CBlackMesaBot* bot) override;

	Vector GetTargetAimPos(CBaseBot* me, CBaseEntity* entity, CBaseExtPlayer* player = nullptr, DesiredAimSpot desiredAim = AIMSPOT_NONE) override;
	std::shared_ptr<const CKnownEntity> SelectTargetThreat(CBaseBot* me, std::shared_ptr<const CKnownEntity> threat1, std::shared_ptr<const CKnownEntity> threat2) override;

	const char* GetName() const override { return "MainTask"; }
private:

	bool AimAtEnemyPlayer(CBaseExtPlayer* them, CBlackMesaBot* me, Vector& out, CBotWeapon* weapon, DesiredAimSpot desiredAim, bool isSecondaryAttack);
};

#endif // !NAVBOT_BLACK_MESA_BOT_MAIN_TASK_H_
