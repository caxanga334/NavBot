#ifndef __NAVBOT_BOT_SHARED_INVESTIGATE_HEARD_ENTITY_H_
#define __NAVBOT_BOT_SHARED_INVESTIGATE_HEARD_ENTITY_H_

#include <extension.h>
#include <util/helpers.h>
#include <sdkports/sdk_ehandle.h>
#include <bot/basebot.h>
#include <bot/basebot_pathcost.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <sdkports/sdk_timers.h>

template <typename BT, typename CT = CBaseBotPathCost>
class CBotSharedInvestigateHeardEntityTask : public AITask<BT>
{
public:
	CBotSharedInvestigateHeardEntityTask(BT* bot, const CKnownEntity* known) :
		m_pathcost(bot), m_entity(known->GetEntity())
	{
		m_lastPosition = known->GetLastKnownPosition();
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override
	{
		CBaseEntity* entity = m_entity.Get();

		if (!entity)
		{
			return AITask<BT>::Done("NULL entity!");
		}

		m_pathcost.SetRouteType(RouteType::FASTEST_ROUTE);
		CKnownEntity* known = bot->GetSensorInterface()->AddKnownEntity(entity);

		if (!m_nav.ComputePathToPosition(bot, known->GetLastKnownPosition(), m_pathcost))
		{
			return AITask<BT>::Done("No path to heard entity last known position!");
		}

		return AITask<BT>::Continue();
	}

	TaskResult<BT> OnTaskUpdate(BT* bot) override
	{
		CBaseEntity* entity = m_entity.Get();

		if (!entity)
		{
			return AITask<BT>::Done("NULL entity!");
		}

		if (bot->GetSensorInterface()->IsIgnored(entity) || !bot->GetSensorInterface()->IsEnemy(entity))
		{
			return AITask<BT>::Done("Invalid entity!");
		}

		if (!UtilHelpers::IsEntityAlive(entity))
		{
			return AITask<BT>::Done("Entity is dead!");
		}

		const CKnownEntity* known = bot->GetSensorInterface()->GetKnown(entity);

		if (known)
		{
			if (known->IsVisibleNow())
			{
				return AITask<BT>::Done("Entity is visible to me!");
			}

			m_lastPosition = known->GetLastKnownPosition();
		}

		if (m_nav.NeedsRepath())
		{
			m_nav.ComputePathToPosition(bot, m_lastPosition, m_pathcost);
			m_nav.StartRepathTimer(1.0f); // faster repaths since we may be chasing a moving entity
		}

		m_nav.Update(bot);

		return AITask<BT>::Continue();
	}

	TaskEventResponseResult<BT> OnSound(BT* bot, CBaseEntity* source, const Vector& position, IEventListener::SoundType type, const float maxRadius) override
	{
		if (!source) { return AITask<BT>::TryContinue(PRIORITY_LOW); }
		if (source == bot->GetEntity()) { return AITask<BT>::TryContinue(PRIORITY_LOW); }

		Vector pos = bot->GetAbsOrigin();
		const float range = (pos - position).Length();

		if (range > maxRadius) { return AITask<BT>::TryContinue(PRIORITY_LOW); }
		if (range > bot->GetSensorInterface()->GetMaxHearingRange()) { return AITask<BT>::TryContinue(PRIORITY_LOW); }
		
		CKnownEntity* known = bot->GetSensorInterface()->AddKnownEntity(source);
		known->UpdateHeard();
		
		if (source == m_entity.Get())
		{
			m_lastPosition = known->GetLastKnownPosition();
		}

		return AITask<BT>::TryContinue(PRIORITY_LOW);
	}

	TaskEventResponseResult<BT> OnMoveToSuccess(BT* bot, CPath* path) override
	{
		CBaseEntity* entity = m_entity.Get();

		if (!entity)
		{
			return AITask<BT>::TryDone(PRIORITY_HIGH, "NULL entity!");
		}

		Vector pos = UtilHelpers::getWorldSpaceCenter(entity);

		if (bot->GetSensorInterface()->IsLineOfSightClear(pos))
		{
			bot->GetControlInterface()->AimAt(pos, IPlayerController::LookPriority::LOOK_SEARCH, 1.5f, "Looking at entity position!");
		}

		return AITask<BT>::TryDone(PRIORITY_HIGH, "Entity position reached!");
	}

	const char* GetName() const override { return "InvestigateHeardEntity"; }
private:
	CT m_pathcost;
	CHandle<CBaseEntity> m_entity;
	Vector m_lastPosition;
	CMeshNavigator m_nav;
};



#endif // !__NAVBOT_BOT_SHARED_INVESTIGATE_HEARD_ENTITY_H_
