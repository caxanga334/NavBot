#ifndef NAVBOT_TF2BOT_DEFEND_CONTROLPOINT_TASK_H_
#define NAVBOT_TF2BOT_DEFEND_CONTROLPOINT_TASK_H_

#include <vector>
#include <bot/interfaces/path/meshnavigator.h>

// Defend a control point from cappers
class CTF2BotDefendControlPointTask : public AITask<CTF2Bot>
{
public:
	CTF2BotDefendControlPointTask(CBaseEntity* controlpoint);

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	TaskResult<CTF2Bot> OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;

	const char* GetName() const override { return "DefendControlPoint"; }

private:
	CHandle<CBaseEntity> m_controlpoint; // control point entity
	CMeshNavigatorAutoRepath m_nav;
	Vector m_capturePos; // position the bot should move to to capture the point
	CountdownTimer m_refreshPosTimer;

	void FindCaptureTrigger(CBaseEntity* controlpoint);
};

// Patrol around the control point
class CTF2BotGuardControlPointTask : public AITask<CTF2Bot>
{
public:
	CTF2BotGuardControlPointTask(CBaseEntity* controlpoint);

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	TaskResult<CTF2Bot> OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;

	const char* GetName() const override { return "GuardControlPoint"; }

private:
	CHandle<CBaseEntity> m_controlpoint; // control point entity
	CMeshNavigatorAutoRepath m_nav;
	Vector m_lookTarget;
	std::vector<Vector> m_guardPos;
	size_t m_currentSpot;
	CountdownTimer m_moveTimer; // don't stand still on the same place
	CountdownTimer m_endTimer; // don't defend the same control point foverer

	void BuildGuardSpots(CTF2Bot* bot);
};


#endif // !NAVBOT_TF2BOT_DEFEND_CONTROLPOINT_TASK_H_
