#ifndef __NAVBOT_TF2BOT_PD_MOVE_TO_CAPTURE_ZONE_TASK_H_
#define __NAVBOT_TF2BOT_PD_MOVE_TO_CAPTURE_ZONE_TASK_H_

class CTF2BotPDMoveToCaptureZoneTask : public AITask<CTF2Bot>
{
public:
	CTF2BotPDMoveToCaptureZoneTask(CBaseEntity* captureZone);

	static bool IsPossible(CTF2Bot* bot, CBaseEntity** captureZone);

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	TaskEventResponseResult<CTF2Bot> OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason) override;

	QueryAnswerType ShouldSeekAndDestroy(CBaseBot* me, const CKnownEntity* them) override;
	QueryAnswerType ShouldHurry(CBaseBot* me) override;
	QueryAnswerType ShouldRetreat(CBaseBot* me) override;
	QueryAnswerType ShouldAssistTeammate(CBaseBot* me, CBaseEntity* teammate) override;

	const char* GetName() const override { return "PDMoveToCap"; }
private:
	CHandle<CBaseEntity> m_capzone;
	CountdownTimer m_timeout;
	CMeshNavigator m_nav;
	int m_pathfailures;
};

#endif // !__NAVBOT_TF2BOT_PD_MOVE_TO_CAPTURE_ZONE_TASK_H_
