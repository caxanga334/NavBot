#ifndef NAVBOT_BLACK_MESA_BOT_MAIN_TASK_H_
#define NAVBOT_BLACK_MESA_BOT_MAIN_TASK_H_

#ifdef EXT_DEBUG
#include <bot/interfaces/path/meshnavigator.h>
#endif

class CBlackMesaBot;

class CBlackMesaBotMainTask : public AITask<CBlackMesaBot>
{
public:
	CBlackMesaBotMainTask();

	AITask<CBlackMesaBot>* InitialNextTask(CBlackMesaBot* bot) override;
	TaskResult<CBlackMesaBot> OnTaskUpdate(CBlackMesaBot* bot) override;

#ifdef EXT_DEBUG
	TaskEventResponseResult<CBlackMesaBot> OnTestEventPropagation(CBlackMesaBot* bot) override;
#endif

	Vector GetTargetAimPos(CBaseBot* me, CBaseEntity* entity, CBaseExtPlayer* player = nullptr, DesiredAimSpot desiredAim = AIMSPOT_NONE) override;
	std::shared_ptr<const CKnownEntity> SelectTargetThreat(CBaseBot* me, std::shared_ptr<const CKnownEntity> threat1, std::shared_ptr<const CKnownEntity> threat2) override;

	const char* GetName() const override { return "MainTask"; }
private:

	bool AimAtEnemyPlayer(CBaseExtPlayer* them, CBlackMesaBot* me, Vector& out, CBotWeapon* weapon, DesiredAimSpot desiredAim, bool isSecondaryAttack);
};

#ifdef EXT_DEBUG
	
class CBlackMesaBotDebugMoveToOriginTask : public AITask<CBlackMesaBot>
{
public:
	CBlackMesaBotDebugMoveToOriginTask(const Vector& destination);

	TaskResult<CBlackMesaBot> OnTaskUpdate(CBlackMesaBot* bot) override;
	TaskEventResponseResult<CBlackMesaBot> OnMoveToFailure(CBlackMesaBot* bot, CPath* path, IEventListener::MovementFailureType reason) override;
	TaskEventResponseResult<CBlackMesaBot> OnMoveToSuccess(CBlackMesaBot* bot, CPath* path) override;

	const char* GetName() const override { return "DebugGoToOrigin"; }
private:
	CMeshNavigatorAutoRepath m_nav;
	Vector m_goal;
	int m_failures;
};


#endif // EXT_DEBUG

#endif // !NAVBOT_BLACK_MESA_BOT_MAIN_TASK_H_
