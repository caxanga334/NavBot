#ifndef NAVBOT_TF2_TASK_SNIPER_MOVE_TO_SPOT_H_
#define NAVBOT_TF2_TASK_SNIPER_MOVE_TO_SPOT_H_

#include <bot/interfaces/path/meshnavigator.h>

class CTF2Bot;
class CTFWaypoint;

class CTF2BotSniperMoveToSnipingSpotTask : public AITask<CTF2Bot>
{
public:
	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	TaskResult<CTF2Bot> OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;

	const char* GetName() const override { return "SniperGoToSpot"; }

private:
	Vector m_goal;
	CMeshNavigator m_nav;
	CTFWaypoint* m_waypoint;
	bool m_sniping;

	void FindSniperSpot(CTF2Bot* bot);
	void GetRandomSnipingSpot(CTF2Bot* bot);
	void FilterWaypoints(CTF2Bot* bot, std::vector<CTFWaypoint*>& waypoints);
	Vector GetRandomSnipingPosition(CTF2Bot* bot);
	Vector GetSnipingSearchStartPosition(CTF2Bot* bot);
};

#endif // !NAVBOT_TF2_TASK_SNIPER_MOVE_TO_SPOT_H_
