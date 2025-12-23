#ifndef __NAVBOT_DODSBOT_DEFUSE_BOMB_TASK_H_
#define __NAVBOT_DODSBOT_DEFUSE_BOMB_TASK_H_

class CDoDSBotDefuseBombTask : public AITask<CDoDSBot>
{
public:
	CDoDSBotDefuseBombTask(CBaseEntity* target);

	TaskResult<CDoDSBot> OnTaskStart(CDoDSBot* bot, AITask<CDoDSBot>* pastTask) override;
	TaskResult<CDoDSBot> OnTaskUpdate(CDoDSBot* bot) override;

	TaskEventResponseResult<CDoDSBot> OnBombDefused(CDoDSBot* bot, const Vector& position, const int teamIndex, CBaseEntity* player, CBaseEntity* ent) override;

	const char* GetName() const override { return "DefuseBomb"; }
private:
	CHandle<CBaseEntity> m_target;
	CMeshNavigator m_nav;
};


#endif // !__NAVBOT_DODSBOT_DEFUSE_BOMB_TASK_H_
