#ifndef TF2BOT_MVM_GUARD_BOMB_TASK_H_
#define TF2BOT_MVM_GUARD_BOMB_TASK_H_

class CTF2BotMvMGuardDroppedBombTask : public AITask<CTF2Bot>
{
public:
	CTF2BotMvMGuardDroppedBombTask(CBaseEntity* bomb);

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	TaskEventResponseResult<CTF2Bot> OnMoveToSuccess(CTF2Bot* bot, CPath* path) override;

	const char* GetName() const override { return "GuardDroppedBomb"; }
private:
	CHandle<CBaseEntity> m_bomb;
	CMeshNavigator m_nav;
	CountdownTimer m_scanTimer;
	Vector m_goal;
	bool m_reached;

};

#endif // !TF2BOT_MVM_GUARD_BOMB_TASK_H_