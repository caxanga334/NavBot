#ifndef __NAVBOT_BOT_SHARED_PICKUP_WEAPON_TASK_H_
#define __NAVBOT_BOT_SHARED_PICKUP_WEAPON_TASK_H_

#include "bot_shared_drop_weapon.h"
#include "bot_shared_use_entity.h"

template <typename BotClass, typename PathCostClass>
class CBotSharedPickupWeaponTask : public AITask<BotClass>
{
public:

	/**
	 * @brief Constructor
	 * @param weapon Weapon entity to pickup.
	 * @param needsUSE If true, bots will +USE to pickup the weapon, else just walk over it.
	 */
	CBotSharedPickupWeaponTask(CBaseEntity* weapon, const bool needsUSE) :
		m_entity(weapon), m_needsUse(needsUSE), m_minDistToDrop(0.0f), m_timesUsedObstacles(0)
	{
	}

	/**
	 * @brief Constructor with validation function
	 * @tparam F Validation function. bool (BotClass* bot, CBaseEntity* weapon).
	 * @param weapon Weapon entity to pickup.
	 * @param needsUSE If true, bots will +USE to pickup the weapon, else just walk over it.
	 * @param fun Function to call to validate this task.
	 */
	template <typename F>
	CBotSharedPickupWeaponTask(CBaseEntity* weapon, const bool needsUSE, F fun) :
		m_entity(weapon), m_needsUse(needsUSE), m_minDistToDrop(0.0f), m_timesUsedObstacles(0)
	{
		m_validFunc = fun;
	}

	TaskResult<BotClass> OnTaskStart(BotClass* bot, AITask<BotClass>* pastTask) override
	{
		CBaseEntity* weapon = m_entity.Get();

		if (weapon == nullptr)
		{
			return AITask<BotClass>::Done("Weapon is NULL!");
		}

		if (bot->IsDebugging(BOTDEBUG_TASKS))
		{
			bot->DebugPrintToConsole(255, 255, 0, "%s PICKUP WEAPON TASK GOAL ENTITY: %s \n", 
				bot->GetDebugIdentifier(), UtilHelpers::textformat::FormatEntity(weapon));
		}

		bot->GetSharedMemoryInterface()->NotifyItemPickup(weapon);
		m_minDistToDrop = bot->GetInventoryInterface()->GetWeaponDropDistance();
		return AITask<BotClass>::Continue();
	}

	TaskResult<BotClass> OnTaskUpdate(BotClass* bot) override
	{
		if (!m_waitTimer.IsElapsed())
		{
			return AITask<BotClass>::Continue();
		}

		if (bot->GetMovementInterface()->IsBreakingObstacle())
		{
			m_timeoutTimer.Invalidate();
			return AITask<BotClass>::Continue();
		}

		if (m_timeoutTimer.HasStarted() && m_timeoutTimer.IsElapsed())
		{
			return AITask<BotClass>::Done("Task timed out!");
		}

		CBaseEntity* weapon = m_entity.Get();

		if (weapon == nullptr)
		{
			return AITask<BotClass>::Done("Weapon is NULL!");
		}
		
		if (CBaseEntity* owner = nullptr; entprops->GetEntPropEnt(weapon, Prop_Send, "m_hOwner", nullptr, &owner))
		{
			if (owner != nullptr)
			{
				if (owner == bot->GetEntity())
				{
					return AITask<BotClass>::Done("I have the weapon!");
				}

				return AITask<BotClass>::Done("Someone eles got the weapon!");
			}
		}

		// In some games, the entity gets EF_NODRAW set while waiting for respawn.
		// Also makes bots a little more human by not being able to pick a weapon nobody can see.
		if (!modhelpers->IsEntityVisible(weapon))
		{
			return AITask<BotClass>::Done("Entity is invisible!");
		}

		bool move = true;
		bool dropWeapons = false;
		Vector eyePos = bot->GetEyeOrigin();
		Vector center = UtilHelpers::getWorldSpaceCenter(weapon);
		float range = (center - eyePos).Length();

		if (range <= m_minDistToDrop)
		{
			if (bot->GetInventoryInterface()->ShouldDropWeaponToPick(weapon))
			{
				const CBotWeapon* weaponToDrop = bot->GetInventoryInterface()->FindWeaponToDrop();

				if (weaponToDrop != nullptr)
				{
					return AITask<BotClass>::PauseFor(new CBotSharedDropWeaponTask<BotClass>(weaponToDrop),
						"I need to drop a weapon to make room in my inventory!");
				}
			}
		}

		if (m_needsUse)
		{
			if (range <= CBaseExtPlayer::PLAYER_USE_RADIUS)
			{
				move = false;

				if (CBaseEntity* obstacle = nullptr; modhelpers->IsUseObstructed(bot->GetEntity(), weapon, &obstacle) == true && m_timesUsedObstacles <= 3)
				{
					if (CBaseEntity* useTarget = nullptr; bot->GetMovementInterface()->IsUseableObstacle(obstacle, &useTarget) == true)
					{
						if (useTarget == nullptr)
						{
							useTarget = obstacle;
						}

						m_timesUsedObstacles++;
						return AITask<BotClass>::PauseFor(new CBotSharedUseEntityTask<BotClass, PathCostClass>(useTarget),
							"Usable obstacle blocking me from picking up my weapon!");
					}

					if (bot->IsAbleToBreak(obstacle))
					{
						m_waitTimer.Start(1.0f);
						bot->GetMovementInterface()->BreakObstacle(obstacle);
						return AITask<BotClass>::Continue();
					}

					if (!m_timeoutTimer.HasStarted())
					{
						m_timeoutTimer.Start(20.0f);
					}

					move = true; // keep moving, maybe we can figure something out
				}

				if (!move)
				{
					if (!m_timeoutTimer.HasStarted())
					{
						m_timeoutTimer.Start(10.0f);
					}

					IPlayerController* input = bot->GetControlInterface();
					float zDiff = std::abs(eyePos.z - center.z);

					if (center.z < eyePos.z && zDiff >= bot->GetMovementInterface()->GetCrouchedHullHeight())
					{
						input->PressCrouchButton(1.0f);
					}

					input->AimAt(center, IPlayerController::LOOK_CRITICAL, 1.0f, "Looking at weapon entity to pick it up.");

					if (input->IsAimOnTarget())
					{
						input->PressUseButton();
						m_waitTimer.Start(0.5f); // don't spam use
						return AITask<BotClass>::Continue();
					}
				}
			}
		}

		if (move)
		{
			if (m_nav.NeedsRepath())
			{
				PathCostClass cost(bot);
				if (!m_nav.ComputePathToPosition(bot, center, cost))
				{
					if (m_counter.Increase())
					{
						return AITask<BotClass>::Done("I can't reach this weapon!");
					}
				}

				m_nav.StartRepathTimer();
			}

			m_nav.Update(bot);
		}

		return AITask<BotClass>::Continue();
	}

	void OnTaskEnd(BotClass* bot, AITask<BotClass>* nextTask) override
	{
		bot->GetInventoryInterface()->RequestRefresh();
	}

	TaskEventResponseResult<BotClass> OnStuck(BotClass* bot) override
	{
		if (m_counter.Increase())
		{
			return AITask<BotClass>::TryDone(PRIORITY_HIGH, "Too many times stuck trying to reach the weapon!");
		}

		return AITask<BotClass>::TryToMaintain(PRIORITY_HIGH);
	}

	TaskEventResponseResult<BotClass> OnUnstuck(BotClass* bot) override
	{
		m_counter.Decrease();
		return AITask<BotClass>::TryToMaintain(PRIORITY_HIGH);
	}

	TaskEventResponseResult<BotClass> OnMoveToFailure(BotClass* bot, CPath* path, IEventListener::MovementFailureType reason) override
	{
		if (m_counter.Increase())
		{
			return AITask<BotClass>::TryDone(PRIORITY_HIGH, "Too many movement failures while trying to reach the weapon!");
		}

		return AITask<BotClass>::TryToMaintain(PRIORITY_HIGH);
	}

	TaskEventResponseResult<BotClass> OnMoveToSuccess(BotClass* bot, CPath* path) override
	{
		if (!m_needsUse)
		{
			return AITask<BotClass>::TryDone(PRIORITY_HIGH, "Weapon reached");
		}

		return AITask<BotClass>::TryToMaintain(PRIORITY_HIGH);
	}

	const char* GetName() const override { return "PickupWeapon"; }
private:
	CHandle<CBaseEntity> m_entity; // target weapon entity handle
	CMeshNavigator m_nav;
	CPathFailCounter m_counter;
	std::function<bool(BotClass*, CBaseEntity*)> m_validFunc; // optional callback to validate this task
	CountdownTimer m_waitTimer;
	CountdownTimer m_timeoutTimer;
	bool m_needsUse; // if true, pickup the weapon by using it, else just walk over
	float m_minDistToDrop;
	int m_timesUsedObstacles;
};


#endif // !__NAVBOT_BOT_SHARED_PICKUP_WEAPON_TASK_H_
