#ifndef NAVBOT_TF2_BOT_ENGINEER_MOVE_OBJECT_TASK_H_
#define NAVBOT_TF2_BOT_ENGINEER_MOVE_OBJECT_TASK_H_

#include <bot/interfaces/path/meshnavigator.h>
#include <sdkports/sdk_ehandle.h>

class CTF2Bot;
class CTFWaypoint;

class CTF2BotEngineerMoveObjectTask : public AITask<CTF2Bot>
{
public:
	CTF2BotEngineerMoveObjectTask(CBaseEntity* object, const Vector& goal, const bool allowDestroying = false);
	CTF2BotEngineerMoveObjectTask(CBaseEntity* object, CTFWaypoint* goal, const bool allowDestroying = false);

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	void OnTaskEnd(CTF2Bot* bot, AITask<CTF2Bot>* nextTask) override;

	// Don't attack or switch weapons
	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_NO; }
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override { return ANSWER_NO; }

	const char* GetName() const override { return "MoveObject"; }
private:
	CHandle<CBaseEntity> m_building;
	Vector m_goal;
	CTFWaypoint* m_waypoint;
	QAngle m_angle;
	CMeshNavigatorAutoRepath m_nav;
	CountdownTimer m_changeAngleTimer;
	bool m_hasBuilding;
	bool m_canDestroy;
	bool m_useRescueRanger;
};


#endif // !NAVBOT_TF2_BOT_ENGINEER_MOVE_OBJECT_TASK_H_

