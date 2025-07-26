#ifndef NAVBOT_BLACK_MESA_BOT_ROAM_TASK_H_
#define NAVBOT_BLACK_MESA_BOT_ROAM_TASK_H_

#include <sdkports/sdk_timers.h>
#include <bot/interfaces/path/meshnavigator.h>

class CBlackMesaBot;

class CBlackMesaBotRoamTask : public AITask<CBlackMesaBot>
{
public:

	TaskResult<CBlackMesaBot> OnTaskStart(CBlackMesaBot* bot, AITask<CBlackMesaBot>* pastTask) override;
	TaskResult<CBlackMesaBot> OnTaskUpdate(CBlackMesaBot* bot) override;

	TaskEventResponseResult<CBlackMesaBot> OnMoveToFailure(CBlackMesaBot* bot, CPath* path, IEventListener::MovementFailureType reason) override;
	TaskEventResponseResult<CBlackMesaBot> OnMoveToSuccess(CBlackMesaBot* bot, CPath* path) override;

	const char* GetName() const override { return "Roam"; }
private:
	CMeshNavigator m_nav;
	Vector m_goal;
	int m_failCount;

	bool SelectRandomGoal(const Vector& start);
};


#endif // !NAVBOT_BLACK_MESA_BOT_ROAM_TASK_H_
