#ifndef __NAVBOT_BOT_SHARED_ROGUE_BEHAVIOR_TASK_H_
#define __NAVBOT_BOT_SHARED_ROGUE_BEHAVIOR_TASK_H_

#include <extension.h>
#include <manager.h>
#include <mods/basemod.h>
#include <bot/basebot.h>
#include <bot/basebot_pathcost.h>
#include "bot_shared_attack_enemy.h"
#include "bot_shared_search_area.h"
#include "bot_shared_roam.h"
#include "bot_shared_escort_entity.h"

/**
 * @brief Shared behavior task for 'rogue' bots.
 * 
 * Rogue bots ignore map objectives. (Unless the objective is deathmatch/team deathmatch)
 * @tparam BT Bot class.
 * @tparam CT Bot path cost class.
 */
template <typename BT, typename CT = CBaseBotPathCost>
class CBotSharedRogueBehaviorTask : public AITask<BT>
{
public:
	/**
	 * @brief Constructor
	 * @param maxTime Maximum time override, negative for random time based on the mod settings values.
	 */
	CBotSharedRogueBehaviorTask(const float maxTime = -1.0f) :
		m_maxTime(maxTime)
	{
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override;
	TaskResult<BT> OnTaskUpdate(BT* bot) override;

	QueryAnswerType ShouldAssistTeammate(CBaseBot* me, CBaseEntity* teammate) override { return ANSWER_NO; }

	const char* GetName() const override { return "RogueBehavior"; }
private:
	float m_maxTime;
	CountdownTimer m_endTimer;
};

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedRogueBehaviorTask<BT, CT>::OnTaskStart(BT* bot, AITask<BT>* pastTask)
{

	if (m_maxTime > 0.0f)
	{
		m_endTimer.Start(m_maxTime);
	}
	else
	{
		const CModSettings* settings = extmanager->GetMod()->GetModSettings();
		const float duration = CBaseBot::s_botrng.GetRandomReal<float>(settings->GetRogueBehaviorMinTime(), settings->GetRogueBehaviorMaxTime());
		m_endTimer.Start(duration);
	}

	return AITask<BT>::Continue();
}

template<typename BT, typename CT>
inline TaskResult<BT> CBotSharedRogueBehaviorTask<BT, CT>::OnTaskUpdate(BT* bot)
{
	if (m_endTimer.IsElapsed())
	{
		return AITask<BT>::Done("Ending rogue behavior! (Timer)");
	}

	const CKnownEntity* known = bot->GetSensorInterface()->GetPrimaryKnownThreat();

	if (known)
	{
		if (known->IsVisibleNow())
		{
			return AITask<BT>::PauseFor(new CBotSharedAttackEnemyTask<BT, CT>(bot), "Attacking visible enemy!");
		}
		else
		{
			const Vector& pos = known->GetLastKnownPosition();
			float minDist = (bot->GetAbsOrigin() - pos).Length();
			minDist = std::min(minDist, 750.0f);
			return AITask<BT>::PauseFor(new CBotSharedSearchAreaTask<BT, CT>(bot, pos, minDist), "Patrolling last known enemy position!");
		}
	}

	const int followchance = std::min(bot->GetDifficultyProfile()->GetTeamwork(), 50);

	if (CBaseBot::s_botrng.GetRandomChance(followchance))
	{
		// get a random teammate (including other bots)
		CBaseEntity* teammate = UtilHelpers::players::GetRandomTeammate(bot->GetEntity(), true, false);

		if (teammate)
		{
			return AITask<BT>::PauseFor(new CBotSharedEscortEntityTask<BT, CT>(bot, teammate, 30.0f), "Following random teammate!");
		}
	}

	const int attackchance = std::min(bot->GetDifficultyProfile()->GetAggressiveness(), 75);
	bool attackEnemies = CBaseBot::s_botrng.GetRandomChance(attackchance);
	return AITask<BT>::PauseFor(new CBotSharedRoamTask<BT, CT>(bot, 8192.0f, attackEnemies, -1.0f, false), "Roaming!");
}

#endif