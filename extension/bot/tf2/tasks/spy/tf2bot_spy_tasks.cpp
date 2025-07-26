#include NAVBOT_PCH_FILE
#include <cstring>
#include <vector>
#include <extension.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/nav/tfnavmesh.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <navmesh/nav_pathfind.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <bot/bot_shared_utils.h>
#include <util/entprops.h>
#include <util/helpers.h>
#include <entities/tf2/tf_entities.h>
#include <sdkports/sdk_takedamageinfo.h>
#include <bot/tf2/tasks/scenario/tf2bot_map_ctf.h>
#include <bot/tasks_shared/bot_shared_pursue_and_destroy.h>
#include "tf2bot_spy_mvm_tasks.h"
#include "tf2bot_spy_tasks.h"

class TF2SpyLurkAreaCollector : public INavAreaCollector<CTFNavArea>
{
public:
	TF2SpyLurkAreaCollector(CTF2Bot* me, CTFNavArea* startArea) :
		INavAreaCollector<CTFNavArea>(startArea, 4096.0f)
	{
		m_me = me;
	}

	bool ShouldSearch(CTFNavArea* area) override
	{
		if (area->IsBlocked(static_cast<int>(m_me->GetMyTFTeam())))
		{
			return false;
		}

		return true;
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

CTF2BotSpyInfiltrateTask::CTF2BotSpyInfiltrateTask() :
	m_goal(0.0f, 0.0f, 0.0f)
{
	m_cloakMeter = nullptr;
	m_isMvM = CTeamFortress2Mod::GetTF2Mod()->GetCurrentGameMode() == TeamFortress2::GameModeType::GM_MVM;
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
			return PauseFor(new CTF2BotSpySapObjectTask(entity), "Sapping visible enemy buildings!");
		}
		else
		{
			if (UtilHelpers::IsPlayer(entity))
			{
				if (m_isMvM && CTF2BotSpySapMvMRobotTask::IsPossible(bot))
				{
					return PauseFor(new CTF2BotSpySapMvMRobotTask(entity), "Going to sap MvM Robot!");
				}

				return PauseFor(new CTF2BotSpyAttackTask(entity), "Visible enemy, going for the stab.");
			}
			else
			{
				return PauseFor(new CBotSharedPursueAndDestroyTask<CTF2Bot, CTF2BotPathCost>(bot, entity), "Visible enemy, attacking!");
			}
		}
	}

	if (bot->GetRangeTo(m_goal) > 32.0f)
	{
		// move to lurk position

		if (!m_nav.IsValid() || m_nav.NeedsRepath())
		{
			CTF2BotPathCost cost(bot, SAFEST_ROUTE);
			m_nav.ComputePathToPosition(bot, m_goal, cost);
			m_nav.StartRepathTimer();
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
			UpdateAimSpots(bot);

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
				if (m_lookAround.IsElapsed() && !m_aimSpots.empty())
				{
					m_lookAround.StartRandom(3.0f, 6.0f);
					const Vector& aimAt = m_aimSpots[randomgen->GetRandomInt<std::size_t>(0U, m_aimSpots.size() - 1U)];
					bot->GetControlInterface()->AimAt(aimAt, IPlayerController::LOOK_INTERESTING, 2.0f, "Aiming at potential enemy pos.");
				}
			}
		}
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotSpyInfiltrateTask::OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	FindLurkPosition(bot);
	m_lurkTimer.Invalidate();

	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotSpyInfiltrateTask::OnFlagTaken(CTF2Bot* bot, CBaseEntity* player)
{
	// need to change this when adding support for player destruction game mode
	if (bot->GetEntity() == player)
	{
		return TryPauseFor(new CTF2BotCTFDeliverFlagTask(), PRIORITY_HIGH, "I have the flag, going to deliver it");
	}

	return TryContinue(PRIORITY_LOW);
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
		me->Disguise(false);
		m_disguiseCooldown.Start(2.0f);
	}
}

void CTF2BotSpyInfiltrateTask::FindLurkPosition(CTF2Bot* me)
{
	Vector pos;
	GetLurkSearchStartPos(me, pos);
	m_aimSpots.clear();
	
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
		auto functor = [&enemyZone, &enemyTeam](int index, edict_t* edict, CBaseEntity* entity) {
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
		};

		UtilHelpers::ForEachEntityOfClassname("func_capturezone", functor);

		if (enemyZone != nullptr)
		{
			out = UtilHelpers::getWorldSpaceCenter(enemyZone);
			out = trace::getground(out);
			return;
		}

		// no enemy capture zone found

		edict_t* flag = me->GetFlagToFetch();

		if (!flag)
		{
			flag = me->GetFlagToDefend(false, true);
		}

		if (flag)
		{
			out = tf2lib::GetFlagPosition(UtilHelpers::EdictToBaseEntity(flag));
			break;
		}

		out = me->GetAbsOrigin();
		break;
	}
	case TeamFortress2::GameModeType::GM_PL:
	{
		CBaseEntity* cart = mod->GetBLUPayload();

		if (!cart)
		{
			cart = mod->GetREDPayload();
		}

		if (cart)
		{
			out = UtilHelpers::getWorldSpaceCenter(cart);
			break;
		}

		CTFNavArea* spawnroomExit = TheTFNavMesh()->GetRandomSpawnRoomExitArea(static_cast<int>(tf2lib::GetEnemyTFTeam(me->GetMyTFTeam())));

		if (spawnroomExit)
		{
			out = spawnroomExit->GetCenter();
			break;
		}

		out = me->GetAbsOrigin();
		break;
	}
	case TeamFortress2::GameModeType::GM_PL_RACE:
	{
		CBaseEntity* cart = nullptr;

		if (me->GetMyTFTeam() == TeamFortress2::TFTeam::TFTeam_Blue)
		{
			cart = mod->GetREDPayload();
		}
		else
		{
			cart = mod->GetBLUPayload();
		}

		if (cart)
		{
			out = UtilHelpers::getWorldSpaceCenter(cart);
			break;
		}

		CTFNavArea* spawnroomExit = TheTFNavMesh()->GetRandomSpawnRoomExitArea(static_cast<int>(tf2lib::GetEnemyTFTeam(me->GetMyTFTeam())));

		if (spawnroomExit)
		{
			out = spawnroomExit->GetCenter();
			break;
		}

		out = me->GetAbsOrigin();
		break;
	}
	case TeamFortress2::GameModeType::GM_MVM:
	{
		CBaseEntity* bomb = tf2lib::mvm::GetMostDangerousFlag();

		if (!bomb)
		{
			CTFNavArea* frontlineArea = TheTFNavMesh()->GetRandomFrontLineArea();

			if (frontlineArea)
			{
				out = frontlineArea->GetCenter();
				break;
			}
		}
		else
		{
			out = tf2lib::GetFlagPosition(bomb);
			break;
		}

		out = me->GetAbsOrigin();
		break;
	}
	default:
	{
		CTFNavArea* spawnroomExit = TheTFNavMesh()->GetRandomSpawnRoomExitArea(static_cast<int>(tf2lib::GetEnemyTFTeam(me->GetMyTFTeam())));

		if (spawnroomExit)
		{
			out = spawnroomExit->GetCenter();
			break;
		}

		out = me->GetAbsOrigin();
		break;
	}
	}
}

void CTF2BotSpyInfiltrateTask::UpdateAimSpots(CTF2Bot* me)
{
	botsharedutils::AimSpotCollector collector(me);
	collector.Execute();
	collector.ExtractAimSpots(m_aimSpots);
}

CTF2BotSpyAttackTask::CTF2BotSpyAttackTask(CBaseEntity* victim) :
	m_nav(CChaseNavigator::DONT_LEAD_SUBJECT)
{
	m_victim = victim;
	m_coverBlown = false;
}

TaskResult<CTF2Bot> CTF2BotSpyAttackTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	if (bot->IsCloaked())
	{
		bot->GetControlInterface()->PressSecondaryAttackButton();
	}

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

	if (!threat || threat->IsObsolete() || !UtilHelpers::IsPlayer(threat->GetEntity()) || bot->GetSensorInterface()->IsIgnored(threat->GetEntity()))
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

		// LOOK_PRIORITY overrides combat but still allows for movement aim
		bot->GetControlInterface()->AimAt(threat->GetEntity(), IPlayerController::LOOK_PRIORITY, 1.0f, "Aiming at my stab victim!");

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

CTF2BotSpySapObjectTask::CTF2BotSpySapObjectTask(CBaseEntity* object) :
	m_nav(CChaseNavigator::DONT_LEAD_SUBJECT)
{
	m_object = object;
	m_isSentryGun = false;
}

TaskResult<CTF2Bot> CTF2BotSpySapObjectTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	CBaseEntity* object = m_object.Get();

	if (object == nullptr)
	{
		return Done("Object is gone!");
	}

	const char* classname = gamehelpers->GetEntityClassname(object);

	if (strncasecmp(classname, "obj_", 4) != 0)
	{
		return Done("Object is gone!");
	}

	if (std::strcmp(classname, "obj_sentrygun") == 0)
	{
		m_isSentryGun = true;
	}

	// equip sapper
	bot->DelayedFakeClientCommand("build 3 0");

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotSpySapObjectTask::OnTaskUpdate(CTF2Bot* bot)
{
	CBaseEntity* object = m_object.Get();

	if (object == nullptr)
	{
		return Done("Object is gone!");
	}

	if (!bot->IsDisguised())
	{
		bot->DisguiseAs(TeamFortress2::TFClass_Engineer, false);
	}

	auto weapon = bot->GetInventoryInterface()->GetActiveBotWeapon();

	// the sapper uses slot 1
	if (weapon && weapon->GetWeaponInfo()->GetSlot() != static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Secondary))
	{
		// equip sapper
		bot->DelayedFakeClientCommand("build 3 0");
	}

	tfentities::HBaseObject bo(object);

	if (bo.IsSapped())
	{
		if (m_isSentryGun)
		{
			auto engineer = bo.GetBuilder();

			if (engineer)
			{
				auto known = bot->GetSensorInterface()->GetKnown(engineer);

				if (known && known->IsVisibleNow())
				{
					return SwitchTo(new CTF2BotSpyAttackTask(known->GetEntity()), "Sentry is sapped, attacking the engineer!");
				}
			}
		}

		// sap other nearby objects

		CBaseEntity* nextObject = nullptr;
		auto func = [&nextObject](const CKnownEntity* known) {
			if (nextObject == nullptr)
			{
				if (!known->IsObsolete() && known->IsVisibleNow())
				{
					CBaseEntity* ent = known->GetEntity();

					if (UtilHelpers::FClassnameIs(ent, "obj_*") && !tf2lib::IsBuildingSapped(ent))
					{
						nextObject = ent;
					}
				}
			}
		};

		bot->GetSensorInterface()->ForEveryKnownEntity(func);

		if (nextObject == nullptr)
		{
			return Done("Object is sapped!");
		}
		else
		{
			m_object = nextObject;
		}
	}

	bot->GetControlInterface()->AimAt(UtilHelpers::getWorldSpaceCenter(object), IPlayerController::LOOK_PRIORITY, 0.5f, "Looking at object to sap!");

	if (bot->GetRangeTo(object) < 90.0f)
	{
		bot->GetControlInterface()->PressAttackButton();
	}
	else
	{
		CTF2BotPathCost cost(bot);
		m_nav.Update(bot, object, cost);
	}

	return Continue();
}
