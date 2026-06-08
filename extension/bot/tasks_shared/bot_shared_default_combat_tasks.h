#ifndef __NAVBOT_BOT_SHARED_DEFAULT_COMBAT_TASKS_H_
#define __NAVBOT_BOT_SHARED_DEFAULT_COMBAT_TASKS_H_

#include <bot/bot_shared_utils.h>
#include <bot/interfaces/path/chasenavigator.h>
#include "bot_shared_retreat_from_threat.h"

// Moves to a threat last known position and
template <typename BT, typename CT>
class CBotSharedDefaultCombatSearchLKPTask : public AITask<BT>
{
public:
	CBotSharedDefaultCombatSearchLKPTask(CBaseEntity* threat) :
		m_threat(threat), m_goal(0.0f, 0.0f, 0.0f), m_usingSixthSense(false), m_pathFails(0)
	{
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override
	{
		CBaseEntity* threat = m_threat.Get();

		if (!threat)
		{
			return AITask<BT>::Done("NULL threat!");
		}

		const CKnownEntity* known = bot->GetSensorInterface()->GetKnown(threat);

		if (!known)
		{
			return AITask<BT>::Done("Threat is no longer in memory!");
		}

		m_tac.Enable(bot->GetCombatInterface());

		return AITask<BT>::Continue();
	}

	TaskResult<BT> OnTaskUpdate(BT* bot) override
	{
		CBaseEntity* threat = m_threat.Get();

		if (!threat)
		{
			return AITask<BT>::Done("NULL threat!");
		}

		if (bot->GetSensorInterface()->IsIgnored(threat))
		{
			return AITask<BT>::Done("Threat is ignored!");
		}

		if (!bot->GetSensorInterface()->IsEnemy(threat))
		{
			return AITask<BT>::Done("Threat is no longer considered an enemy!");
		}

		const CKnownEntity* known = bot->GetSensorInterface()->GetKnown(threat);

		if (!known)
		{
			return AITask<BT>::Done("Threat is no longer in memory!");
		}

		if (!m_usingSixthSense)
		{
			m_goal = known->GetLastKnownPosition();
		}
		
		if (m_nav.NeedsRepath())
		{
			CT cost(bot);
			cost.SetRouteType(RouteType::FASTEST_ROUTE);
			
			if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
			{
				if (++m_pathFails >= 10)
				{
					return AITask<BT>::Done("Can't path to last known position!");
				}
			}

			m_nav.StartRepathTimer();
		}

		m_nav.Update(bot);
		return AITask<BT>::Continue();
	}

	TaskEventResponseResult<BT> OnNavAreaChanged(BT* bot, CNavArea* oldArea, CNavArea* newArea) override
	{
		// as we search for an enemy, mark areas we walk into as cleared for the patrol behavior
		if (newArea)
		{
			newArea->MarkAsCleared(bot->GetCurrentTeamIndex());
		}

		return AITask<BT>::TryContinue();
	}

	TaskEventResponseResult<BT> OnSight(BT* bot, CBaseEntity* subject) override
	{
		if (bot->GetSensorInterface()->IsEnemy(subject))
		{
			return AITask<BT>::TryDone(PRIORITY_HIGH, "Enemy on my sights!");
		}

		return AITask<BT>::TryToMaintain(PRIORITY_MEDIUM);
	}

	TaskEventResponseResult<BT> OnOtherKilled(BT* bot, CBaseEntity* pVictim, const CTakeDamageInfo& info) override
	{
		CBaseEntity* threat = m_threat.Get();

		if (threat && pVictim && threat == pVictim)
		{
			return AITask<BT>::TryDone(PRIORITY_HIGH, "Enemy killed!");
		}

		return AITask<BT>::TryToMaintain(PRIORITY_MEDIUM);
	}

	TaskEventResponseResult<BT> OnStuck(BT* bot) override
	{
		if (++m_pathFails >= 10)
		{
			return AITask<BT>::TryDone(PRIORITY_HIGH, "Got stuck!");
		}

		m_nav.Invalidate(); // force a repath
		return AITask<BT>::TryToMaintain(PRIORITY_LOW);
	}

	TaskEventResponseResult<BT> OnMoveToFailure(BT* bot, CPath* path, IEventListener::MovementFailureType reason) override
	{
		if (++m_pathFails >= 10)
		{
			return AITask<BT>::TryDone(PRIORITY_HIGH, "Move failed!");
		}

		m_nav.Invalidate(); // force a repath
		return AITask<BT>::TryToMaintain(PRIORITY_LOW);
	}

	TaskEventResponseResult<BT> OnMoveToSuccess(BT* bot, CPath* path) override
	{
		// mark areas we can see as cleared for the patrol behavior
		botsharedutils::search::MarkVisibleAreasAsCleared functor(bot);
		functor.Execute();

		if (m_usingSixthSense)
		{
			return AITask<BT>::TryDone(PRIORITY_HIGH, "Last known position reached! (Sixth Sense)");
		}

		CBaseEntity* threat = m_threat.Get();

		if (threat)
		{
			// game awareness controls the chance for entering the sixth sense mode
			const int chance = std::min(bot->GetDifficultyProfile()->GetGameAwareness(), 90);

			if (CBaseBot::s_botrng.GetRandomChance(chance))
			{
				m_usingSixthSense = true;
				m_goal = UtilHelpers::getEntityOrigin(threat);
				m_nav.Invalidate();
				return AITask<BT>::TryToMaintain(PRIORITY_MEDIUM);
			}
		}

		return AITask<BT>::TryDone(PRIORITY_HIGH, "Last known position reached!");
	}

	TaskEventResponseResult<BT> OnSound(BT* bot, CBaseEntity* source, const Vector& position, IEventListener::SoundType type, const float maxRadius) override
	{
		// low skill bot, ignore all sounds
		if (bot->GetDifficultyProfile()->GetGameAwareness() < 35)
		{
			return AITask<BT>::TryToMaintain(PRIORITY_MEDIUM);
		}

		const float rangeTo = bot->GetRangeTo(position);

		// can't hear it
		if (rangeTo > maxRadius || rangeTo > bot->GetSensorInterface()->GetMaxHearingRange())
		{
			return AITask<BT>::TryToMaintain(PRIORITY_MEDIUM);
		}

		CBaseEntity* threat = m_threat.Get();

		if (threat && source && threat == source)
		{
			CKnownEntity* known = bot->GetSensorInterface()->AddKnownEntity(source);
			known->UpdateHeard();

			if (m_usingSixthSense)
			{
				m_usingSixthSense = false;
				m_goal = position;
			}
		}

		return AITask<BT>::TryToMaintain(PRIORITY_MEDIUM);
	}

	const char* GetName() const override { return "SearchLastKnownPosition"; }
private:
	CMeshNavigator m_nav;
	CHandle<CBaseEntity> m_threat;
	Vector m_goal;
	bool m_usingSixthSense; // this is true if the bot is moving to the threat's current location, ignoring LOS
	int m_pathFails;
	combatutils::ToggleAreaClearing m_tac;
};

// Attacks the primary threat, moves if necessary.
template <typename BT, typename CT>
class CBotSharedDefaultCombatAttackMoveTask : public AITask<BT>
{
public:
	CBotSharedDefaultCombatAttackMoveTask() :
		m_nav(CChaseNavigator::SubjectLeadType::DONT_LEAD_SUBJECT)
	{
	}

	TaskResult<BT> OnTaskUpdate(BT* bot) override
	{
		const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(ISensor::ANY_THREATS);
		CBaseBot* __bot = bot;

		if (!threat)
		{
			return AITask<BT>::Done("No threat!");
		}

		if (!threat->IsVisibleNow())
		{
			// mark areas we can see as cleared for the patrol behavior
			botsharedutils::search::MarkVisibleAreasAsCleared functor(bot);
			functor.Execute();

			// too far to chase
			if (bot->GetRangeTo(threat->GetLastKnownPosition()) >= bot->GetDifficultyProfile()->GetEnemyFarRange())
			{
				return AITask<BT>::Done("Enemy eluded me and is too far to chase!");
			}

			// if healthy and a random chance based on aggression level, search for the enemy
			if (__bot->GetHealthState() == CBaseBot::HealthState::HEALTH_OK && 
				CBaseBot::s_botrng.GetRandomChance(bot->GetDifficultyProfile()->GetAggressiveness()))
			{
				return AITask<BT>::SwitchTo(new CBotSharedDefaultCombatSearchLKPTask<BT, CT>(threat->GetEntity()), "Enemy is occluded!");
			}

			return AITask<BT>::Done("Enemy eluded me, giving up chase!");
		}

		const ICombat::CombatData& cd = bot->GetCombatInterface()->GetCachedCombatData();

		if (m_retreatCheckTimer.IsElapsed())
		{
			m_retreatCheckTimer.Start(0.75f);

			// behavior allows retreating?
			if (bot->GetBehaviorInterface()->ShouldRetreat(bot) != ANSWER_NO)
			{
				if (bot->GetHealthState() == CBaseBot::HealthState::HEALTH_CRITICAL)
				{
					// critically low on health, retreat
					return AITask<BT>::SwitchTo(new CBotSharedRetreatFromThreatTask<BT, CT>(bot, botsharedutils::SelectRetreatArea::RetreatAreaPreference::NEAREST), "Critically low on health, retreating!");
				}
			}
		}

		// if the enemy is currently outside our firing range OR the line of fire is obstructed
		if (cd.ShouldPathToEnemy())
		{
			CT cost(bot);
			cost.SetRouteType(RouteType::FASTEST_ROUTE);
			m_nav.Update(bot, threat->GetEntity(), cost);
		}

		return AITask<BT>::Continue();
	}

	TaskEventResponseResult<BT> OnSight(BT* bot, CBaseEntity* subject) override
	{
		// block OnSight events
		return AITask<BT>::TryToMaintain(PRIORITY_MEDIUM);
	}

	QueryAnswerType ShouldRetreat(CBaseBot* me) override
	{
		const ICombat::CombatData& cd = me->GetCombatInterface()->GetCachedCombatData();

		// too close to retreat, fight until death
		if (cd.is_visible)
		{
			if (cd.enemy_range <= me->GetDifficultyProfile()->GetEnemyCloseRange())
			{
				return ANSWER_NO;
			}

			// always allow retreating if too far
			if (cd.enemy_range > me->GetDifficultyProfile()->GetEnemyFarRange())
			{
				return ANSWER_YES;
			}
		}

		return ANSWER_UNDEFINED;
	}

	const char* GetName() const override { return "AttackMove"; }
private:
	CChaseNavigator m_nav;
	CountdownTimer m_retreatCheckTimer;
};

/**
 * @brief Default combat handling task.
 * @tparam BT Bot class.
 * @tparam CT Bot path cost class.
 */
template <typename BT, typename CT>
class CBotSharedDefaultCombatBehaviorTask : public AITask<BT>
{
public:
	static bool IsPossible(BT* bot)
	{
		const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(ISensor::ONLY_VISIBLE_THREATS);

		if (!threat)
		{
			return false;
		}

		if (bot->GetBehaviorInterface()->ShouldSeekAndDestroy(bot, threat) == QueryAnswerType::ANSWER_NO)
		{
			return false;
		}

		return true;
	}

	AITask<BT>* InitialNextTask(BT* bot) override
	{
		return new CBotSharedDefaultCombatAttackMoveTask<BT, CT>;
	}

	TaskResult<BT> OnTaskUpdate(BT* bot) override
	{
		const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(ISensor::ANY_THREATS);

		if (!threat)
		{
			return AITask<BT>::Done("No threat!");
		}

		if (AITask<BT>::GetNextTask() == nullptr)
		{
			if (threat->IsVisibleNow())
			{
				AITask<BT>::StartNewNextTask(bot, new CBotSharedDefaultCombatAttackMoveTask<BT, CT>, "Threat visible, starting attack move behavior!");
			}

			return AITask<BT>::Done("Secondary behavior ended!");
		}

		return AITask<BT>::Continue();
	}

	// always attack enemies in this behavior
	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_YES; }
	// No, we are already in combat.
	QueryAnswerType ShouldSeekAndDestroy(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_NO; }
	// Always allow weapon selection in this behavior
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override { return ANSWER_YES; }

	const char* GetName() const override { return "Combat"; }
};



#endif // !__NAVBOT_BOT_SHARED_DEFAULT_COMBAT_TASKS_H_
