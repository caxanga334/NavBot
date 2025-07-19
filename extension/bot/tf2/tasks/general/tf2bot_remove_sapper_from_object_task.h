#ifndef __NAVBOT_TF2BOT_REMOVE_OBJECT_SAPPER_TASK_H_
#define __NAVBOT_TF2BOT_REMOVE_OBJECT_SAPPER_TASK_H_

/**
 * @brief Task for removing sappers from objects. Can be used by engineers and non-engineers.
 */
class CTF2BotRemoveObjectSapperTask : public AITask<CTF2Bot>
{
public:
	CTF2BotRemoveObjectSapperTask(CBaseEntity* object);

	static bool IsPossible(CTF2Bot* bot);

	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_NO; }
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override { return ANSWER_NO; }

	const char* GetName() const override { return "RemoveSapper"; }

private:
	CChaseNavigator m_nav;
	CHandle<CBaseEntity> m_object;

};


#endif // !__NAVBOT_TF2BOT_REMOVE_OBJECT_SAPPER_TASK_H_
