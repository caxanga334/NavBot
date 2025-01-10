#ifndef NAVBOT_TF2_BOT_MVM_TASKS_H_
#define NAVBOT_TF2_BOT_MVM_TASKS_H_

#include <vector>
#include <sdkports/sdk_timers.h>
#include <sdkports/sdk_ehandle.h>
#include <bot/interfaces/path/meshnavigator.h>

class CTF2Bot;

class CTF2BotCollectMvMCurrencyTask : public AITask<CTF2Bot>
{
public:
	CTF2BotCollectMvMCurrencyTask(std::vector<CHandle<CBaseEntity>>& packs);

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	
	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override
	{
		if (m_allowattack)
		{
			return ANSWER_YES;
		}
		
		return ANSWER_NO;
	}

	const char* GetName() const override { return "CollectMvMCurrency"; }
private:
	CountdownTimer m_repathTimer;
	CMeshNavigator m_nav;
	std::vector<CHandle<CBaseEntity>> m_currencypacks;
	std::vector<CHandle<CBaseEntity>>::iterator m_it;
	bool m_allowattack;
};

class CTF2BotMvMCombatTask : public AITask<CTF2Bot>
{
public:
	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;


	const char* GetName() const override { return "MvMCombat"; }
private:
	CountdownTimer m_repathTimer;
	CMeshNavigator m_nav;
	Vector m_moveGoal;
	float m_idealCombatRange;
};

#endif // !NAVBOT_TF2_BOT_MVM_TASKS_H_
