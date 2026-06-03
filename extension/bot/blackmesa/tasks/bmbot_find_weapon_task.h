#ifndef NAVBOT_BLACK_MESA_BOT_FIND_WEAPON_TASK_H_
#define NAVBOT_BLACK_MESA_BOT_FIND_WEAPON_TASK_H_

#include <sdkports/sdk_ehandle.h>
#include <bot/interfaces/path/meshnavigator.h>

class CBlackMesaBot;

class CBlackMesaBotFindWeaponTask : public AITask<CBlackMesaBot>
{
public:
	/**
	 * @brief Determines if the find weapon task can be done right now.
	 * @param[in] bot Bot that will use this task.
	 * @param[out] weapon Weapon the bot will pick up.
	 * @return True if the task can be done, false otherwise.
	 */
	static bool IsPossible(CBlackMesaBot* bot, CBaseEntity** weapon);

	CBlackMesaBotFindWeaponTask(CBaseEntity* weapon) :
		m_weapon(weapon), m_goal(0.0f, 0.0f, 0.0f)
	{
	}

	TaskResult<CBlackMesaBot> OnTaskStart(CBlackMesaBot* bot, AITask<CBlackMesaBot>* pastTask) override;
	TaskResult<CBlackMesaBot> OnTaskUpdate(CBlackMesaBot* bot) override;
	void OnTaskEnd(CBlackMesaBot* bot, AITask<CBlackMesaBot>* nextTask) override;

	TaskEventResponseResult<CBlackMesaBot> OnMoveToSuccess(CBlackMesaBot* bot, CPath* path) override;
	TaskEventResponseResult<CBlackMesaBot> OnWeaponEquip(CBlackMesaBot* bot, CBaseEntity* weapon) override;

	const char* GetName() const override { return "FindWeapon"; }
private:
	CHandle<CBaseEntity> m_weapon;
	CMeshNavigator m_nav;
	Vector m_goal;

	bool IsWeaponValid();
};

#endif // !NAVBOT_BLACK_MESA_BOT_FIND_WEAPON_TASK_H_
