#ifndef NAVBOT_TF2BOT_TASK_SNIPE_AREA_H_
#define NAVBOT_TF2BOT_TASK_SNIPE_AREA_H_

#include <vector>
#include <mods/tf2/nav/tfnav_waypoint.h>

class CTF2Bot;

class CTF2BotSniperSnipeAreaTask : public AITask<CTF2Bot>
{
public:
	CTF2BotSniperSnipeAreaTask(CTFWaypoint* waypoint)
	{
		m_waypoint = waypoint;
		m_lookPoints.reserve(4);
	}

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	void OnTaskEnd(CTF2Bot* bot, AITask<CTF2Bot>* nextTask) override;
	bool OnTaskPause(CTF2Bot* bot, AITask<CTF2Bot>* nextTask) override;

	const char* GetName() const override { return "SnipeArea"; }

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override;

private:
	CTFWaypoint* m_waypoint;
	CountdownTimer m_boredTimer;
	CountdownTimer m_changeAnglesTimer;
	std::vector<Vector> m_lookPoints;
	CountdownTimer m_fireWeaponDelay;

	void BuildLookPoints(CTF2Bot* me);
	void EquipAndScope(CTF2Bot* me);
};

#endif // !NAVBOT_TF2BOT_TASK_SNIPE_AREA_H_
