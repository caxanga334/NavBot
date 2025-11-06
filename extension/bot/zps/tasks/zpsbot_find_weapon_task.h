#ifndef __NAVBOT_ZPSBOT_FIND_WEAPON_TASK_H_
#define __NAVBOT_ZPSBOT_FIND_WEAPON_TASK_H_

class CZPSBotFindWeaponTask : public AITask<CZPSBot>
{
public:
	static bool IsPossible(CZPSBot* bot, CBaseEntity** outweapon);

	CZPSBotFindWeaponTask(CBaseEntity* weapon);

	TaskResult<CZPSBot> OnTaskStart(CZPSBot* bot, AITask<CZPSBot>* pastTask) override;
	TaskResult<CZPSBot> OnTaskUpdate(CZPSBot* bot) override;

	const char* GetName() const override { return "FindWeapon"; }
private:
	CHandle<CBaseEntity> m_weapon;
	CMeshNavigator m_nav;
	CountdownTimer m_waitTimer;
	CountdownTimer m_timeout;
	bool m_needsDrop;
	bool m_dropped;

	static bool IsValidWeapon(CBaseEntity* weapon);
};


#endif // !__NAVBOT_ZPSBOT_FIND_WEAPON_TASK_H_
