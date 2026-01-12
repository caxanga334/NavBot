#ifndef __NAVBOT_DODSBOT_FIRE_RIFLE_GRENADE_TASK_H_
#define __NAVBOT_DODSBOT_FIRE_RIFLE_GRENADE_TASK_H_

class CDoDSBotFireRifleGrenadeTask : public AITask<CDoDSBot>
{
public:
	CDoDSBotFireRifleGrenadeTask(CDoDSWaypoint* waypoint);
	/**
	 * @brief Determines if it's possible for a given bot to fire a rifle grenade.
	 * @param bot Bot that will be using a rifle grenade.
	 * @param waypoint The waypoint to fire the grenade from will be stored here.
	 * @param index Optional control point index filter.
	 * @return True if it's possible and a waypoint has been found. False otherwise.
	 */
	static bool IsPossible(CDoDSBot* bot, CDoDSWaypoint** waypoint, int index);
	TaskResult<CDoDSBot> OnTaskStart(CDoDSBot* bot, AITask<CDoDSBot>* pastTask) override;
	TaskResult<CDoDSBot> OnTaskUpdate(CDoDSBot* bot) override;
	void OnTaskEnd(CDoDSBot* bot, AITask<CDoDSBot>* nextTask) override;

	TaskEventResponseResult<CDoDSBot> OnMoveToSuccess(CDoDSBot* bot, CPath* path) override;

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override;
	QueryAnswerType ShouldAssistTeammate(CBaseBot* me, CBaseEntity* teammate) override;
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override;

	const char* GetName() const override { return "FireRifleGrenade"; }
private:
	CMeshNavigator m_nav;
	CDoDSWaypoint* m_waypoint;
	Vector m_pos;
	QAngle m_aimAngle;
	CountdownTimer m_fireDelay;
	bool m_atPos;
};


#endif // __NAVBOT_DODSBOT_FIRE_RIFLE_GRENADE_TASK_H_