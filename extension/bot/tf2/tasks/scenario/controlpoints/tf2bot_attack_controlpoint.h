#ifndef NAVBOT_TF2BOT_ATTACK_CONTROL_POINT_H_
#define NAVBOT_TF2BOT_ATTACK_CONTROL_POINT_H_

#include <sdkports/sdk_ehandle.h>
#include <bot/interfaces/path/meshnavigator.h>

class CTF2BotAttackControlPointTask : public AITask<CTF2Bot>
{
public:
	CTF2BotAttackControlPointTask(CBaseEntity* controlpoint);

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	TaskResult<CTF2Bot> OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;

	QueryAnswerType ShouldHurry(CBaseBot* me) override;
	QueryAnswerType ShouldRetreat(CBaseBot* me) override;
	QueryAnswerType ShouldSeekAndDestroy(CBaseBot* me, const CKnownEntity* them) override;

	const char* GetName() const override { return "AttackControlPoint"; }

private:
	CHandle<CBaseEntity> m_controlpoint; // control point entity
	CMeshNavigatorAutoRepath m_nav;
	Vector m_capturePos; // position the bot should move to to capture the point
	CountdownTimer m_refreshPosTimer;
	RouteType m_routeType;

	void FindCaptureTrigger(CBaseEntity* controlpoint);

	static constexpr auto CRITICAL_ROUND_TIME = 60.0f; // if the round time remaining is less than this, rush
};

#endif // !NAVBOT_TF2BOT_ATTACK_CONTROL_POINT_H_
