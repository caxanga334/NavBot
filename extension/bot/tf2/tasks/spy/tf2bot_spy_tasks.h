#ifndef NAVBOT_TF2BOT_SPY_TASKS_H_
#define NAVBOT_TF2BOT_SPY_TASKS_H_

#include <sdkports/sdk_ehandle.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <bot/interfaces/path/chasenavigator.h>

class CTF2Bot;

class CTF2BotSpyInfiltrateTask : public AITask<CTF2Bot>
{
public:
	CTF2BotSpyInfiltrateTask();

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	TaskResult<CTF2Bot> OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_NO; }

	TaskEventResponseResult<CTF2Bot> OnFlagTaken(CTF2Bot* bot, CBaseEntity* player) override;
	TaskEventResponseResult<CTF2Bot> OnControlPointCaptured(CTF2Bot* bot, CBaseEntity* point) override;
	TaskEventResponseResult<CTF2Bot> OnControlPointLost(CTF2Bot* bot, CBaseEntity* point) override;

	const char* GetName() const override { return "SpyInfiltrate"; }

private:
	CMeshNavigator m_nav;
	Vector m_goal;
	CountdownTimer m_disguiseCooldown;
	CountdownTimer m_cloakTimer;
	CountdownTimer m_lurkTimer;
	CountdownTimer m_repathTimer;
	CountdownTimer m_lookAround;
	float* m_cloakMeter;
	std::vector<Vector> m_aimSpots;
	bool m_isMvM;

	void DisguiseMe(CTF2Bot* me);

	// Cloak meter percentage
	float GetCloakPercentage()
	{
		if (m_cloakMeter)
		{
			return *m_cloakMeter;
		}

		return -1.0f;
	}

	void FindLurkPosition(CTF2Bot* me);
	void GetLurkSearchStartPos(CTF2Bot* me, Vector& out);
	void UpdateAimSpots(CTF2Bot* me);
};

class CTF2BotSpyAttackTask : public AITask<CTF2Bot>
{
public:
	CTF2BotSpyAttackTask(CBaseEntity* victim);

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	TaskEventResponseResult<CTF2Bot> OnContact(CTF2Bot* bot, CBaseEntity* pOther) override;
	TaskEventResponseResult<CTF2Bot> OnInjured(CTF2Bot* bot, const CTakeDamageInfo& info) override;

	QueryAnswerType IsBlocker(CBaseBot* me, edict_t* blocker, const bool any = false) override;
	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override;
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override;

	const char* GetName() const override { return "SpyAttack"; }
private:
	CHandle<CBaseEntity> m_victim;
	CChaseNavigator m_nav;
	bool m_coverBlown;

	static constexpr auto CHANGE_TARGET_DISTANCE = 300.0f;
};

class CTF2BotSpySapObjectTask : public AITask<CTF2Bot>
{
public:
	CTF2BotSpySapObjectTask(CBaseEntity* object);

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_NO; }
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override { return ANSWER_NO; }

	const char* GetName() const override { return "SpySapObject"; }

private:
	CHandle<CBaseEntity> m_object;
	CChaseNavigator m_nav;
	bool m_isSentryGun;
};

#endif // !NAVBOT_TF2BOT_SPY_TASKS_H_



