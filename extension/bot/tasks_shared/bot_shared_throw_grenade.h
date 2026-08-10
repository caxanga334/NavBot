#ifndef __NAVBOT_BOT_SHARED_THROW_GRENADE_TASK_H_
#define __NAVBOT_BOT_SHARED_THROW_GRENADE_TASK_H_

#include <bot/interfaces/path/chasenavigator.h>
#include <navmesh/nav_mesh.h>

template <typename BotClass, typename PathCostClass>
class CBotSharedThrowGrenadeTask : public AITask<BotClass>
{
public:
	static bool IsPossible(BotClass* bot_, const CKnownEntity* threat, CBaseEntity** grenade)
	{
		CBaseBot* bot = bot_;

		// low skill bots don't throw grenades
		if (bot->GetDifficultyProfile()->GetGameAwareness() < 25)
		{
			return false;
		}

		// random chance to not throw grenades
		if (CBaseBot::s_botrng.GetRandomChance(33))
		{
			return false;
		}

		const CBotWeapon* weapon = bot->GetInventoryInterface()->FindCombatGrenade(threat->GetLastKnownPosition());

		if (weapon)
		{
			*grenade = weapon->GetEntity();
			return true;
		}

		return false;
	}

	CBotSharedThrowGrenadeTask(CBaseEntity* weapon, const Vector& target) :
		m_weapon(weapon), m_throwTarget(target)
	{
		m_moveTarget = trace::getground(target);
	}

	TaskResult<BotClass> OnTaskStart(BotClass* bot, AITask<BotClass>* pastTask) override
	{
		m_timeout.StartRandom(10.0f, 15.0f);
		return AITask<BotClass>::Continue();
	}

	TaskResult<BotClass> OnTaskUpdate(BotClass* bot) override
	{
		const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(ISensor::ONLY_VISIBLE_THREATS);

		if (threat)
		{
			return AITask<BotClass>::Done("Visible enemy, aborting!");
		}

		if (!m_delay.IsElapsed())
		{
			return AITask<BotClass>::Continue();
		}

		if (m_timeout.IsElapsed())
		{
			return AITask<BotClass>::Done("Timed out!");
		}

		CBaseEntity* entity = m_weapon.Get();

		if (!entity)
		{
			return AITask<BotClass>::Done("NULL weapon!");
		}

		IInventory* inv = bot->GetInventoryInterface();
		const CBotWeapon* weapon = inv->GetWeaponOfEntity(entity);

		if (!weapon)
		{
			return AITask<BotClass>::Done("No longer has a grenade!");
		}

		const WeaponInfo* info = weapon->GetWeaponInfo();
		Vector eyePos = bot->GetEyeOrigin();
		const float range = (m_throwTarget - eyePos).Length();
		const float maxrange = info->GetAttackInfo(botweapons::AttackType::PRIMARY).GetMaxRange();
		const float minrange = info->GetAttackInfo(botweapons::AttackType::PRIMARY).GetMinRange();

		if (range <= minrange)
		{
			return AITask<BotClass>::Done("Too close!");
		}

		if (!bot->GetCombatInterface()->IsBlastTargetClearOfAllies(m_throwTarget, info->GetExplosionBlastRadius()))
		{
			return AITask<BotClass>::Done("Aborting throw to avoid friendly fire!");
		}

		if (range <= maxrange && bot->IsLineOfFireClear(m_throwTarget))
		{
			// in range and clear line of fire
			const CBotWeapon* current = inv->GetActiveBotWeapon();
			
			if (current != weapon)
			{
				m_delay.Start(2.0f);
				inv->EquipWeapon(weapon);
				return AITask<BotClass>::Continue();
			}

			IPlayerController* input = bot->GetControlInterface();
			Vector aimAt = bot->GetBehaviorInterface()->GetAimPosition(bot, m_throwTarget, botweapons::AttackType::PRIMARY);

			if (!bot->IsLineOfFireClear(aimAt))
			{
				return AITask<BotClass>::Done("Aborting throw: no clear line of fire to predicted position!");
			}

			input->AimAt(aimAt, IPlayerController::LOOK_CRITICAL, 1.0f, "Looking at grenade throw target!");

			if (input->IsAimOnTarget())
			{
				if (!m_release.HasStarted())
				{
					// hold the attack button
					m_release.Start(0.5f);
					input->PressAttackButton(0.4f);
				}
				else if (m_release.IsElapsed())
				{
					CNavArea* area = TheNavMesh->GetNearestNavArea(m_moveTarget, 400.0f);

					if (area && area->HasPlaceName())
					{
						const std::string* name = TheNavMesh->GetPlaceName(area->GetPlace());

						if (name)
						{
							std::array<char, 128> buffer;
							ke::SafeSprintf(buffer.data(), buffer.size(), "Throwing grenade at %s!", name->c_str());
							bot->SendTeamChatMessage(buffer.data());
						}
					}

					return AITask<BotClass>::Done("Grenade thrown!");
				}
			}
		}
		else
		{
			if (m_nav.NeedsRepath())
			{
				PathCostClass cost(bot);
				cost.SetRouteType(RouteType::FASTEST_ROUTE);
				m_nav.ComputePathToPosition(bot, m_moveTarget, cost);
				m_nav.StartRepathTimer();
			}

			m_nav.Update(bot);
		}

		return AITask<BotClass>::Continue();
	}

	const char* GetName() const override { return "ThrowGrenade"; }
private:
	CMeshNavigator m_nav;
	CountdownTimer m_timeout;
	CountdownTimer m_delay;
	CountdownTimer m_release;
	CHandle<CBaseEntity> m_weapon; // the weapon to use
	Vector m_throwTarget; // position the grenade should land
	Vector m_moveTarget;
};




#endif // !__NAVBOT_BOT_SHARED_THROW_GRENADE_TASK_H_
