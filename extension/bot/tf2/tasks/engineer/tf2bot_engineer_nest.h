#ifndef NAVBOT_TF2BOT_TASKS_ENGINEER_NEST_H_
#define NAVBOT_TF2BOT_TASKS_ENGINEER_NEST_H_

#include <sdkports/sdk_timers.h>
#include <bot/interfaces/path/meshnavigator.h>

class CTF2Bot;
class CTFWaypoint;

/**
 * @brief Task responsible for taking care of the engineer's own buildings
 */
class CTF2BotEngineerNestTask : public AITask<CTF2Bot>
{
public:
	CTF2BotEngineerNestTask();

	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	bool OnTaskPause(CTF2Bot* bot, AITask<CTF2Bot>* nextTask) override;
	TaskResult<CTF2Bot> OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;


	TaskEventResponseResult<CTF2Bot> OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason) override;
	TaskEventResponseResult<CTF2Bot> OnMoveToSuccess(CTF2Bot* bot, CPath* path) override;
	TaskEventResponseResult<CTF2Bot> OnRoundStateChanged(CTF2Bot* bot) override;
	TaskEventResponseResult<CTF2Bot> OnVoiceCommand(CTF2Bot* bot, CBaseEntity* subject, int command) override;
	TaskEventResponseResult<CTF2Bot> OnControlPointCaptured(CTF2Bot* bot, CBaseEntity* point) override;
	TaskEventResponseResult<CTF2Bot> OnControlPointLost(CTF2Bot* bot, CBaseEntity* point) override;

	// Engineers don't retreat for health and ammo
	QueryAnswerType ShouldRetreat(CBaseBot* me) override { return ANSWER_NO; }
	// for mvm
	QueryAnswerType IsReady(CBaseBot* me) override;

	const char* GetName() const override { return "EngineerNest"; }

	static constexpr auto max_travel_distance() { return 2048.0f; }
	static constexpr auto behind_sentry_distance() { return 96.0f; }
	static constexpr auto max_dispenser_to_sentry_range() { return 750.0f; }
	static constexpr auto random_exit_spot_travel_limit() { return 900.0f; }
	static constexpr auto mvm_sentry_to_bomb_range_limit() { return 1500.0f; }

private:
	CMeshNavigator m_nav;
	Vector m_goal;
	CTFWaypoint* m_sentryWaypoint; // last waypoint used for building sentry guns
	CountdownTimer m_noThreatTimer;
	CountdownTimer m_sentryEnemyScanTimer;
	CountdownTimer m_moveBuildingCheckTimer;
	CountdownTimer m_roundStateTimer;
	CountdownTimer m_respondToVCTimer;
	bool m_justMovedSentry; // recently moved the sentry

	bool ShouldMoveSentryGun();
	AITask<CTF2Bot>* NestTask(CTF2Bot* me);
	AITask<CTF2Bot>* MoveBuildingsIfNeeded(CTF2Bot* bot);

	inline void ForceMoveBuildings()
	{
		m_noThreatTimer.Start(1.0f);
		m_moveBuildingCheckTimer.Start(3.0f);
	}
};

#endif // !NAVBOT_TF2BOT_TASKS_ENGINEER_NEST_H_
