#include <extension.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <navmesh/nav_pathfind.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <util/entprops.h>
#include <util/helpers.h>
#include <entities/tf2/tf_entities.h>
#include <sdkports/sdk_takedamageinfo.h>
#include "tf2bot_spy_tasks.h"

class TF2SpyLurkAreaCollector : public INavAreaCollector<CTFNavArea>
{
public:
	TF2SpyLurkAreaCollector(CTF2Bot* me, CTFNavArea* startArea) :
		INavAreaCollector<CTFNavArea>(startArea, 4096.0f)
	{
		m_me = me;
	}

	bool ShouldCollect(CTFNavArea* area) override;
private:
	CTF2Bot* m_me;
};

bool TF2SpyLurkAreaCollector::ShouldCollect(CTFNavArea* area)
{
	if (area->GetSizeX() < 16.0f || area->GetSizeY() < 16.0f)
	{
		// skip small areas
		return false;
	}

	if (area->HasTFPathAttributes(CTFNavArea::TFNavPathAttributes::TFNAV_PATH_DYNAMIC_SPAWNROOM))
	{
		// don't lurk inside spawn rooms
		return false;
	}

	if (area->IsBlocked(m_me->GetCurrentTeamIndex()))
	{
		// area is blocked to me
		return false;
	}

	if (area->GetHidingSpots()->Count() < 1)
	{
		// area doesn't have any hiding spots
		return false;
	}

	return true;
}

CTF2BotSpyInfiltrateTask::CTF2BotSpyInfiltrateTask()
{
	m_cloakMeter = nullptr;
}

TaskResult<CTF2Bot> CTF2BotSpyInfiltrateTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	m_cloakMeter = entprops->GetPointerToEntData<float>(bot->GetEntity(), Prop_Send, "m_flCloakMeter");

	FindLurkPosition(bot);

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotSpyInfiltrateTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (!bot->IsDisguised())
	{
		DisguiseMe(bot);
	}

	auto threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

	if (threat)
	{
		CBaseEntity* entity = threat->GetEntity();

		if (UtilHelpers::FClassnameIs(entity, "obj_*"))
		{
			// TO-DO: Sap objects
		}
		else
		{
			return PauseFor(new CTF2BotSpyAttackTask(entity), "Visible enemy, going for the stab.");
		}
	}

	if (bot->GetRangeTo(m_goal) > 32.0f)
	{
		// move to lurk position

		if (!m_nav.IsValid() || m_repathTimer.IsElapsed())
		{
			CTF2BotPathCost cost(bot, SAFEST_ROUTE);
			m_nav.ComputePathToPosition(bot, m_goal, cost);
			m_repathTimer.Start(0.6f);
		}

		m_nav.Update(bot);

		if (!bot->IsInsideSpawnRoom() && !tf2lib::IsPlayerInvisible(bot->GetEntity()))
		{
			if (GetCloakPercentage() > 20.0f && m_cloakTimer.IsElapsed())
			{
				m_cloakTimer.Start(0.5f);
				bot->GetControlInterface()->PressSecondaryAttackButton();
			}
		}
	}
	else
	{
		// arrived at lurk spot

		if (!m_lurkTimer.HasStarted())
		{
			m_lurkTimer.StartRandom(15.0f, 60.0f);

			if (tf2lib::IsPlayerInvisible(bot->GetEntity()))
			{
				bot->GetControlInterface()->PressSecondaryAttackButton();
			}
		}
		else
		{
			if (m_lurkTimer.IsElapsed())
			{
				FindLurkPosition(bot);
				m_lurkTimer.Invalidate();
			}
			else
			{
				if (m_lookAround.IsElapsed())
				{
					m_lookAround.StartRandom(2.0f, 6.0f);

					QAngle angle{ 0.0f, bot->GetRandomNumberGenerator().GetRandomReal<float>(0.0f, 360.0f), 0.0f };
					bot->GetControlInterface()->AimAt(angle, IPlayerController::LOOK_INTERESTING, 1.0f, "Looking around.");
				}
			}
		}
	}

	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotSpyInfiltrateTask::OnControlPointCaptured(CTF2Bot* bot, CBaseEntity* point)
{
	FindLurkPosition(bot);
	m_lurkTimer.Invalidate();

	return TryContinue(PRIORITY_LOW);
}

TaskEventResponseResult<CTF2Bot> CTF2BotSpyInfiltrateTask::OnControlPointLost(CTF2Bot* bot, CBaseEntity* point)
{
	FindLurkPosition(bot);
	m_lurkTimer.Invalidate();

	return TryContinue(PRIORITY_LOW);
}

void CTF2BotSpyInfiltrateTask::DisguiseMe(CTF2Bot* me)
{
	if (m_disguiseCooldown.IsElapsed())
	{
		me->DisguiseAs(static_cast<TeamFortress2::TFClassType>(randomgen->GetRandomInt<int>(1, 9)));
		m_disguiseCooldown.Start(2.0f);
	}
}

void CTF2BotSpyInfiltrateTask::FindLurkPosition(CTF2Bot* me)
{
	Vector pos;
	GetLurkSearchStartPos(me, pos);
	
	CTFNavArea* startArea = static_cast<CTFNavArea*>(TheNavMesh->GetNearestNavArea(pos, 1024.0f));

	if (startArea == nullptr)
	{
		m_goal = pos;
		return;
	}

	TF2SpyLurkAreaCollector collector(me, startArea);
	collector.Execute();

	if (collector.IsCollectedAreasEmpty())
	{
		m_goal = pos;
		return;
	}

	CTFNavArea* randomGoal = collector.GetRandomCollectedArea();
	m_goal = randomGoal->GetCenter();
}

void CTF2BotSpyInfiltrateTask::GetLurkSearchStartPos(CTF2Bot* me, Vector& out)
{
	auto mod = CTeamFortress2Mod::GetTF2Mod();

	auto gm = mod->GetCurrentGameMode();

	switch (gm)
	{
	case TeamFortress2::GameModeType::GM_TC:
		[[fallthrough]];
	case TeamFortress2::GameModeType::GM_KOTH:
		[[fallthrough]];
	case TeamFortress2::GameModeType::GM_ARENA:
		[[fallthrough]];
	case TeamFortress2::GameModeType::GM_CP:
		[[fallthrough]];
	case TeamFortress2::GameModeType::GM_ADCP:
	{
		std::vector<CBaseEntity*> points;
		points.reserve(MAX_CONTROL_POINTS);
		mod->CollectControlPointsToAttack(me->GetMyTFTeam(), points);
		mod->CollectControlPointsToDefend(me->GetMyTFTeam(), points);

		if (points.empty())
		{
			out = me->GetAbsOrigin();
			return;
		}

		CBaseEntity* pEntity = librandom::utils::GetRandomElementFromVector<CBaseEntity*>(points);

		out = UtilHelpers::getWorldSpaceCenter(pEntity);
		return;
	}
	case TeamFortress2::GameModeType::GM_CTF:
	{
		CBaseEntity* enemyZone = nullptr;
		TeamFortress2::TFTeam enemyTeam = tf2lib::GetEnemyTFTeam(me->GetMyTFTeam());

		UtilHelpers::ForEachEntityOfClassname("func_capturezone", [&enemyZone, &enemyTeam](int index, edict_t* edict, CBaseEntity* entity) {
			if (entity != nullptr)
			{
				tfentities::HCaptureZone capzone(entity);

				if (!capzone.IsDisabled() && capzone.GetTFTeam() == enemyTeam)
				{
					enemyZone = entity;
					return false; // end search
				}
			}

			return true;
		});

		if (enemyZone != nullptr)
		{
			out = UtilHelpers::getWorldSpaceCenter(enemyZone);
			out = trace::getground(out);
			return;
		}

		// no enemy capture zone found
		out = me->GetAbsOrigin();
		break;
	}
	default:
		out = me->GetAbsOrigin();
		break;
	}
}

CTF2BotSpyAttackTask::CTF2BotSpyAttackTask(CBaseEntity* victim) :
	m_nav(CChaseNavigator::DONT_LEAD_SUBJECT)
{
	m_victim = victim;
	m_coverBlown = false;
}

TaskResult<CTF2Bot> CTF2BotSpyAttackTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotSpyAttackTask::OnTaskUpdate(CTF2Bot* bot)
{
	auto threat = bot->GetSensorInterface()->GetKnown(m_victim.Get());
	auto primarythreat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

	if (!threat)
	{
		if (primarythreat && primarythreat->IsPlayer())
		{
			threat = primarythreat;
		}
	}
	else if (threat && primarythreat)
	{
		if (threat->GetEntity() != primarythreat->GetEntity() && primarythreat->IsPlayer())
		{
			float currentRange = bot->GetRangeTo(threat->GetLastKnownPosition());
			float newRange = bot->GetRangeTo(primarythreat->GetLastKnownPosition());

			if (currentRange - newRange > CHANGE_TARGET_DISTANCE)
			{
				threat = primarythreat;
				m_victim = primarythreat->GetEntity();
			}
		}
	}

	if (!threat || threat->IsObsolete())
	{
		return Done("Victim is obsolete!");
	}

	if (tf2lib::IsPlayerInvisible(threat->GetEntity()))
	{
		return Done("My target is invisible!");
	}

	bool moveTowardsVictim = true;

	if (!m_coverBlown)
	{
		CBaseExtPlayer them(threat->GetEdict());
		Vector victimForward;
		them.EyeVectors(&victimForward);
		Vector toThem = (them.GetAbsOrigin() - bot->GetAbsOrigin());
		float distance = toThem.NormalizeInPlace();

		constexpr auto behindTolerance = 0.1f;

		bool isBehindThem = DotProduct(victimForward, toThem) > behindTolerance;

		auto myweapon = bot->GetInventoryInterface()->GetActiveBotWeapon();

		// switch to knife
		if (myweapon && myweapon->GetWeaponInfo()->GetSlot() != static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Melee))
		{
			auto knife = bot->GetWeaponOfSlot(static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Melee));

			if (knife)
			{
				bot->SelectWeapon(knife); // switch to knife
			}
		}

		// LOOK_VERY_IMPORTANT overrides combat but still allows for movement aim
		bot->GetControlInterface()->AimAt(threat->GetEntity(), IPlayerController::LOOK_VERY_IMPORTANT, 1.0f, "Aiming at my stab victim!");

		if (!isBehindThem)
		{
			// at this range, start strafing to get to their backs
			if (distance < 250.0f)
			{
				Vector myForward;
				bot->EyeVectors(&myForward);
				Vector cross;
				CrossProduct(victimForward, myForward, cross);

				if (cross.z < 0.0f)
				{
					// strafe right
					bot->GetControlInterface()->PressMoveRightButton();
				}
				else
				{
					// strafe left
					bot->GetControlInterface()->PressMoveLeftButton();
				}

				if (distance < 100.0f)
				{
					// avoid bumping into them
					moveTowardsVictim = false;
				}
			}
		}
		else
		{
			if (distance < 70.0f)
			{
				bot->GetControlInterface()->PressAttackButton(); // stab stab stab
			}
		}
	}

	if (moveTowardsVictim)
	{
		CTF2BotPathCost cost(bot);
		m_nav.Update(bot, threat->GetEntity(), cost);
	}

	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotSpyAttackTask::OnContact(CTF2Bot* bot, CBaseEntity* pOther)
{
	if (!m_coverBlown && pOther != nullptr && pOther == m_victim.Get())
	{
		// sanity check, should always be a player
		if (UtilHelpers::IsPlayerIndex(m_victim.GetEntryIndex()))
		{
			CBaseExtPlayer them(m_victim.ToEdict());
			
			if (them.IsLookingTowards(bot->GetEntity()))
			{
				m_coverBlown = true;
			}
		}
	}

	return TryContinue(PRIORITY_LOW);
}

TaskEventResponseResult<CTF2Bot> CTF2BotSpyAttackTask::OnInjured(CTF2Bot* bot, const CTakeDamageInfo& info)
{
	CBaseEntity* attacker = info.GetAttacker();

	if (attacker != nullptr && attacker == m_victim.Get())
	{
		m_coverBlown = true;
	}

	return TryContinue(PRIORITY_LOW);
}

QueryAnswerType CTF2BotSpyAttackTask::IsBlocker(CBaseBot* me, edict_t* blocker, const bool any)
{
	if (!any)
	{
		if (blocker != nullptr && m_victim.ToEdict() == blocker)
		{
			return ANSWER_NO; // tell the navigator to not avoid my target
		}
	}

	return ANSWER_UNDEFINED;
}

QueryAnswerType CTF2BotSpyAttackTask::ShouldAttack(CBaseBot* me, const CKnownEntity* them)
{
	// don't attack with revolver unless my cover was blown
	if (!m_coverBlown)
	{
		return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}

QueryAnswerType CTF2BotSpyAttackTask::ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon)
{
	// keep knife equipped unless cover is blown
	if (!m_coverBlown)
	{
		return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}
