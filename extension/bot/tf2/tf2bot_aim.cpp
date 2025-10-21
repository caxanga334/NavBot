#include NAVBOT_PCH_FILE
#include "tf2bot.h"
#include "tf2bot_weaponinfo.h"
#include "tf2bot_aim.h"

Vector CTF2BotAimHelper::SelectAimPosition(CTF2Bot* bot, CBaseEntity* entity, IDecisionQuery::DesiredAimSpot aimspot)
{
	Initialize(bot, entity, aimspot);
	const CTF2BotWeapon* weapon = GetModWeapon<const CTF2BotWeapon*>();

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
			// Use TF2 exclusive speed and gravity functions
			data.speed = weapon->GetProjectileSpeed(type);
			data.gravity = weapon->GetProjectileGravity(type);
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
