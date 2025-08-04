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

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
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

	bool operator()(int index, edict_t* edict, CBaseEntity* entity);

	const char* GetName() const override { return "EngineerNest"; }

	static constexpr auto max_travel_distance() { return 2048.0f; }
	static constexpr auto behind_sentry_distance() { return 96.0f; }
	static constexpr auto max_dispenser_to_sentry_range() { return 750.0f; }
	static constexpr auto random_exit_spot_travel_limit() { return 900.0f; }
	static constexpr auto mvm_sentry_to_bomb_range_limit() { return 1500.0f; }

private:
	CMeshNavigator m_nav;
	Vector m_goal;
	Vector m_sentryBuildPos;
	CTFWaypoint* m_sentryWaypoint; // waypoint used to build the nest
	CountdownTimer m_moveSentryGunTimer; // main move building timer
	CountdownTimer m_checkOthersTimer; // secondary timer for moving non sentry gun buildings
	CountdownTimer m_roundStateTimer;
	CountdownTimer m_respondToVCTimer;
	CountdownTimer m_scanForEnemiesTimer;
	// cache for member functions
	CBaseEntity* m_mysentry;
	CBaseEntity* m_mydispenser;
	CBaseEntity* m_myentrance;
	CBaseEntity* m_myexit;
	int m_sentryKills;
	int m_teleUses;
	bool m_findNewNestPos;
	bool m_sentryWasBuilt;
	bool m_checkEntrance;
	bool m_checkExit;
	bool m_checkDispenser; // recently moved the sentry

	static constexpr int ALLY_ACTION_NONE = 0;
	static constexpr int ALLY_ACTION_REPAIR = 1;
	static constexpr int ALLY_ACTION_UPGRADE = 2;
	static constexpr int ALLY_ACTION_SAPPED = 3;

	int m_myteam;
	std::vector<std::pair<int, CBaseEntity*>> m_allyObjects;
	Vector m_myOrigin;
	float m_maxHelpAllyRange;
	static constexpr float MOVE_BUILDING_TIMER_EXTRA_TIME = 40.0f; // additional time to add to the move timer when moving buildings (gives time for bots to reach their destination)

	void ForceMoveBuildings(const float delay)
	{
		m_moveSentryGunTimer.Start(delay);
	}

	AITask<CTF2Bot>* GetBuildTask(CTF2Bot* me); // returns a build related task or NULL if none
	AITask<CTF2Bot>* GetMoveBuildingsTask(CTF2Bot* me);
	AITask<CTF2Bot>* GetHelpFriendlyEngineerTask(CTF2Bot* me);
};

#endif // !NAVBOT_TF2BOT_TASKS_ENGINEER_NEST_H_
