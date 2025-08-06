#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/entprops.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <entities/tf2/tf_entities.h>
#include <bot/tf2/tf2bot.h>
#include <bot/tasks_shared/bot_shared_attack_enemy.h>
#include "tf2bot_attack_controlpoint.h"

CTF2BotAttackControlPointTask::CTF2BotAttackControlPointTask(CBaseEntity* controlpoint) :
	m_capturePos(0.0f, 0.0f, 0.0f)
{
	m_controlpoint = controlpoint;
	m_routeType = DEFAULT_ROUTE;
}

TaskResult<CTF2Bot> CTF2BotAttackControlPointTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	CBaseEntity* pEntity = m_controlpoint.Get();

	if (pEntity == nullptr)
	{
		return Done("Given control point is NULL!");
	}

	tfentities::HTeamControlPoint cp(pEntity);

	if (cp.IsLocked())
	{
		return Done("Control point is locked!");
	}

	FindCaptureTrigger(pEntity);
	m_refreshPosTimer.Start(5.0f);

	if (bot->GetTimeLeftToCapture() <= 90.0f)
	{
		m_routeType = FASTEST_ROUTE;
	}
	else
	{
		int aggression = bot->GetDifficultyProfile()->GetAggressiveness() - 40;

		if (aggression <= 0)
		{
			m_routeType = SAFEST_ROUTE;
		}
		else if(CBaseBot::s_botrng.GetRandomInt<int>(1, 100) <= aggression)
		{
			m_routeType = FASTEST_ROUTE;
		}
		else
		{
			m_routeType = SAFEST_ROUTE;
		}
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotAttackControlPointTask::OnTaskUpdate(CTF2Bot* bot)
{
	CBaseEntity* pEntity = m_controlpoint.Get();

	if (pEntity == nullptr)
	{
		return Done("Given control point is NULL!");
	}

	if (m_refreshPosTimer.IsElapsed())
	{
		m_refreshPosTimer.Start(5.0f);
		FindCaptureTrigger(pEntity);
	}

	auto threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

	if (threat && bot->GetBehaviorInterface()->ShouldHurry(bot) != ANSWER_YES)
	{
		// only attack if i'm not close to the control point
		if (bot->GetRangeTo(UtilHelpers::getWorldSpaceCenter(threat->GetEntity())) + 512.0f < bot->GetRangeTo(m_capturePos))
		{
			return PauseFor(new CBotSharedAttackEnemyTask<CTF2Bot, CTF2BotPathCost>(bot), "Attacking threat!");
		}
	}

	tfentities::HTeamControlPoint cp(pEntity);
	Vector pos = bot->WorldSpaceCenter();

	if (!trace::pointwithin(pEntity, pos))
	{
		CTF2BotPathCost cost(bot, m_routeType);
		m_nav.Update(bot, m_capturePos, cost);
	}

	std::vector<CBaseEntity*> Vecpoints;
	CTeamFortress2Mod::GetTF2Mod()->CollectControlPointsToAttack(bot->GetMyTFTeam(), Vecpoints);

	for (auto point : Vecpoints)
	{
		if (point == pEntity)
		{
			return Continue(); // current target point still is a valid attack target
		}
	}

	// current target is not listed on the control point attack list but the list is also not empty

	if (!Vecpoints.empty())
	{
		CBaseEntity* newPoint = librandom::utils::GetRandomElementFromVector<CBaseEntity*>(Vecpoints);
		return SwitchTo(new CTF2BotAttackControlPointTask(newPoint), "Attacking another control point!");
	}
	else
	{
		// list of control points to attack is empty
		return Done("No more control points to attack.");
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotAttackControlPointTask::OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	if (m_controlpoint.Get() == nullptr)
	{
		return Done("NULL control point!");
	}

	m_nav.Invalidate();
	m_nav.ForceRepath();

	return Continue();
}

QueryAnswerType CTF2BotAttackControlPointTask::ShouldHurry(CBaseBot* me)
{
	if (static_cast<CTF2Bot*>(me)->GetTimeLeftToCapture() <= CRITICAL_ROUND_TIME)
	{
		return ANSWER_YES;
	}

	return ANSWER_UNDEFINED;
}

QueryAnswerType CTF2BotAttackControlPointTask::ShouldRetreat(CBaseBot* me)
{
	if (static_cast<CTF2Bot*>(me)->GetTimeLeftToCapture() <= CRITICAL_ROUND_TIME)
	{
		return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}

QueryAnswerType CTF2BotAttackControlPointTask::ShouldSeekAndDestroy(CBaseBot* me, const CKnownEntity* them)
{
	if (static_cast<CTF2Bot*>(me)->GetTimeLeftToCapture() <= CRITICAL_ROUND_TIME)
	{
		return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}

void CTF2BotAttackControlPointTask::FindCaptureTrigger(CBaseEntity* controlpoint)
{
	tfentities::HTeamControlPoint entity(controlpoint);

	std::unique_ptr<char[]> targetname = std::make_unique<char[]>(128);
	entity.GetTargetName(targetname.get(), 128);
	CBaseEntity* trigger = nullptr;
	auto functor = [&targetname, &controlpoint, &trigger](int index, edict_t* edict, CBaseEntity* entity) {

		if (entity != nullptr)
		{
			std::unique_ptr<char[]> cpname = std::make_unique<char[]>(128);
			size_t length = 0;
			entprops->GetEntPropString(index, Prop_Data, "m_iszCapPointName", cpname.get(), 128, length);

			if (length > 0)
			{
				int cpindex = UtilHelpers::FindEntityByTargetname(INVALID_EHANDLE_INDEX, cpname.get());

				if (cpindex == gamehelpers->EntityToBCompatRef(controlpoint))
				{
					trigger = entity;
					return false;
				}
			}

		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("trigger_capture_area", functor);

	if (trigger)
	{
		Vector pos = UtilHelpers::getWorldSpaceCenter(trigger);

		if (!pos.IsZero(0.5f))
		{
			pos = trace::getground(pos);
			m_capturePos = pos;
		}
		else
		{
			m_capturePos = entity.GetAbsOrigin();
		}
	}
	else
	{
		m_capturePos = entity.GetAbsOrigin();
	}
}
