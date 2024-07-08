#ifndef NAVBOT_TF2BOT_TASK_SNIPE_AREA_H_
#define NAVBOT_TF2BOT_TASK_SNIPE_AREA_H_

class CTF2Bot;

class CTF2BotSniperSnipeAreaTask : public AITask<CTF2Bot>
{
public:
	CTF2BotSniperSnipeAreaTask();
	CTF2BotSniperSnipeAreaTask(const QAngle& hintAngles);

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	void OnTaskEnd(CTF2Bot* bot, AITask<CTF2Bot>* nextTask) override;
	bool OnTaskPause(CTF2Bot* bot, AITask<CTF2Bot>* nextTask) override;

	const char* GetName() const override { return "SnipeArea"; }

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override;

private:
	QAngle m_lookAngles;
	Vector m_hintLookAt;
	CountdownTimer m_boredTimer;
	CountdownTimer m_changeAnglesTimer;
	std::vector<Vector> m_lookPoints;
	CountdownTimer m_fireWeaponDelay;


	bool m_hint;

	void BuildLookPoints(CTF2Bot* me);
	void EquipAndScope(CTF2Bot* me);
};

#endif // !NAVBOT_TF2BOT_TASK_SNIPE_AREA_H_
