#ifndef __NAVBOT_BOT_AIM_H_
#define __NAVBOT_BOT_AIM_H_

#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/prediction.h>
#include <util/gamedata_const.h>
#include "weapon.h"
#include "decisionquery.h"
#include <extplayer.h>

class CBaseEntity;

template<typename BotClass>
class IBotAimHelper
{
public:
	IBotAimHelper()
	{
		m_target = nullptr;
		m_weapon = nullptr;
		m_desiredSpot = IDecisionQuery::DesiredAimSpot::AIMSPOT_NONE;
		m_targetPlayer = nullptr;
	}

	virtual ~IBotAimHelper() = default;

	// Main function, call this to select the position the bot should aim
	virtual Vector SelectAimPosition(BotClass* bot, CBaseEntity* entity, IDecisionQuery::DesiredAimSpot aimspot)
	{
		Initialize(bot, entity, aimspot);
		const CBotWeapon* weapon = GetHeldWeapon();

		if (weapon == nullptr)
		{
			return UtilHelpers::getWorldSpaceCenter(entity);
		}
	
		Vector initialTargetPosition = GetInitialAimPosition(bot);
		const bool canpredict = bot->GetDifficultyProfile()->ShouldPredictProjectiles();

		WeaponInfo::AttackFunctionType type = WeaponInfo::AttackFunctionType::PRIMARY_ATTACK;

		if (bot->GetControlInterface()->GetLastUsedAttackType() == IPlayerInput::AttackType::ATTACK_SECONDARY)
		{
			type = WeaponInfo::AttackFunctionType::SECONDARY_ATTACK;
		}
		
		if (canpredict)
		{
			const int maxiterations = bot->GetDifficultyProfile()->GetMaxPredictionIterations();

			if (weapon->GetWeaponInfo()->GetAttackInfo(type).IsBallistic())
			{
				pred::ProjectileData_t data;
				data.FillFromAttackInfo(&weapon->GetWeaponInfo()->GetAttackInfo(type));
				Vector predicted = pred::IterativeBallisticLead(GetShooterPosition(), initialTargetPosition, GetTargetVelocity(), data, maxiterations);

				if (!bot->IsLineOfFireClear(predicted))
				{
					return initialTargetPosition;
				}

				return predicted;
			}

			if (weapon->GetWeaponInfo()->GetAttackInfo(type).IsProjectile())
			{
				const float projSpeed = weapon->GetWeaponInfo()->GetAttackInfo(type).GetProjectileSpeed();
				Vector predicted = pred::IterativeProjectileLead(GetShooterPosition(), initialTargetPosition, GetTargetVelocity(), projSpeed, maxiterations);

				if (!bot->IsLineOfFireClear(predicted))
				{
					return initialTargetPosition;
				}

				return predicted;
			}
		}

		// hitscan weapons
		return initialTargetPosition;
	}

protected:

	CBaseEntity* GetAimTarget() const { return m_target; }
	void SetAimTarget(CBaseEntity* entity) { m_target = entity; }
	const CBotWeapon* GetHeldWeapon() const { return m_weapon; }
	template <typename T>
	T GetModWeapon() const { return static_cast<T>(m_weapon); }
	void SetHeldWeapon(const CBotWeapon* weapon) { m_weapon = weapon; }
	IDecisionQuery::DesiredAimSpot GetDesiredAimSpot() const { return m_desiredSpot; }
	void SetDesiredAimSpot(IDecisionQuery::DesiredAimSpot spot) { m_desiredSpot = spot; }
	const CBaseExtPlayer* GetTargetPlayer() const { return m_targetPlayer; }
	void SetTargetPlayer(const CBaseExtPlayer* player) { m_targetPlayer = player; }
	bool IsTargetAPlayer() const { return m_targetPlayer != nullptr; }
	const Vector& GetShooterPosition() const { return m_shooterPosition; }
	void SetShooterPosition(const Vector& pos) { m_shooterPosition = pos; }
	const Vector& GetTargetVelocity() const { return m_targetVelocity; }
	void SetTargetVelocity(const Vector& vel) { m_targetVelocity = vel; }

	// Called to initialize values for a new aim position calculation
	virtual void Initialize(BotClass* bot, CBaseEntity* entity, IDecisionQuery::DesiredAimSpot aimspot)
	{
		CBaseEntity* old = GetAimTarget();

		if (old != entity)
		{
			OnNewAimTarget(entity);
		}

		SetDesiredAimSpot(aimspot);
		SetHeldWeapon(bot->GetInventoryInterface()->GetActiveBotWeapon());
		SetAimTarget(entity);
		SetTargetPlayer(extmanager->GetPlayerOfEntity(entity));
		RetrieveTargetVelocity(entity);
		SetShooterPosition(bot->GetEyeOrigin());
	}

	// Called to read the velocity of the target entity
	virtual void RetrieveTargetVelocity(CBaseEntity* entity)
	{
		Vector result;

		if (entprops->GetEntPropVector(entity, Prop_Data, "m_vecAbsVelocity", result))
		{
			SetTargetVelocity(result);
			return;
		}

		SetTargetVelocity(vec3_origin);
	}

	// Called when the target entity changes
	virtual void OnNewAimTarget(CBaseEntity* entity) {}

	// Returns the initial aim position without any movement prediction
	Vector GetInitialAimPosition(BotClass* bot)
	{
		Vector result;

		if (IsTargetAPlayer())
		{
			GetInitialAimPositionForPlayers(bot, result);
		}
		else
		{
			GetInitialAimPositionForEntities(bot, result);
		}

		return result;
	}

	virtual void GetInitialAimPositionForPlayers(BotClass* bot, Vector& result)
	{
		Vector wepOffset = GetHeldWeapon()->GetWeaponInfo()->GetHeadShotAimOffset();
		const CBaseExtPlayer* player = GetTargetPlayer();

		switch (GetDesiredAimSpot())
		{
		case IDecisionQuery::AIMSPOT_ABSORIGIN:
			result = player->GetAbsOrigin();
			break;
		case IDecisionQuery::AIMSPOT_CENTER:
			result = player->WorldSpaceCenter();
			break;
		case IDecisionQuery::AIMSPOT_HEAD:
		{
			player->GetHeadShotPosition(GamedataConstants::s_head_aim_bone.c_str(), result);
			result + wepOffset;
			break;
		}
		case IDecisionQuery::AIMSPOT_OFFSET:
			result = player->GetAbsOrigin() + bot->GetControlInterface()->GetCurrentDesiredAimOffset();
			break;
		case IDecisionQuery::AIMSPOT_BONE:
		{
			int bone = UtilHelpers::studio::LookupBone(player->GetEntity(), bot->GetControlInterface()->GetCurrentDesiredAimBone().c_str());

			if (bone != UtilHelpers::studio::INVALID_BONE_INDEX)
			{
				QAngle angle;
				if (UtilHelpers::studio::GetBonePosition(player->GetEntity(), bone, result, angle))
				{
					break;
				}
			}

			// bone look up failed
			result = player->WorldSpaceCenter();
			break;
		}
		default:
			result = player->WorldSpaceCenter();
			break;
		}
	}
	
	// Non-player entities
	virtual void GetInitialAimPositionForEntities(BotClass* bot, Vector& result)
	{
		Vector wepOffset = GetHeldWeapon()->GetWeaponInfo()->GetHeadShotAimOffset();
		CBaseEntity* target = GetAimTarget();

		switch (GetDesiredAimSpot())
		{
		case IDecisionQuery::AIMSPOT_ABSORIGIN:
			result = UtilHelpers::getEntityOrigin(target);
			break;
		case IDecisionQuery::AIMSPOT_CENTER:
			result = UtilHelpers::getWorldSpaceCenter(target);
			break;
		case IDecisionQuery::AIMSPOT_HEAD:
		{
			Vector* viewOffset = entprops->GetPointerToEntData<Vector>(target, Prop_Data, "m_vecViewOffset");

			if (viewOffset)
			{
				result = UtilHelpers::getEntityOrigin(target);
				result = result + (*viewOffset);
			}
			else
			{
				ICollideable* collider = reinterpret_cast<IServerEntity*>(target)->GetCollideable();
				const Vector& maxs = collider->OBBMaxs();
				const Vector& mins = collider->OBBMins();
				float z = abs(maxs.z - mins.z);
				z *= 0.90f;
				result = UtilHelpers::getEntityOrigin(target);
				result.z += z;
			}

			break;
		}
		case IDecisionQuery::AIMSPOT_OFFSET:
			result = UtilHelpers::getEntityOrigin(target) + bot->GetControlInterface()->GetCurrentDesiredAimOffset();
			break;
		case IDecisionQuery::AIMSPOT_BONE:
		{
			int bone = UtilHelpers::studio::LookupBone(target, bot->GetControlInterface()->GetCurrentDesiredAimBone().c_str());

			if (bone != UtilHelpers::studio::INVALID_BONE_INDEX)
			{
				QAngle angle;
				if (UtilHelpers::studio::GetBonePosition(target, bone, result, angle))
				{
					break;
				}
			}

			// bone look up failed
			result = UtilHelpers::getWorldSpaceCenter(target);
			break;
		}
		default:
			result = UtilHelpers::getWorldSpaceCenter(target);
			break;
		}
	}

private:
	Vector m_shooterPosition;
	Vector m_targetVelocity;
	CBaseEntity* m_target;
	const CBotWeapon* m_weapon;
	IDecisionQuery::DesiredAimSpot m_desiredSpot;
	const CBaseExtPlayer* m_targetPlayer;

};

#endif // !__NAVBOT_BOT_AIM_H_
