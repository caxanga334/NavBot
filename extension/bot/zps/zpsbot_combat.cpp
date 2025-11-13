#include NAVBOT_PCH_FILE
#include <mods/zps/zps_mod.h>
#include "zpsbot.h"
#include "zpsbot_combat.h"

CZPSBotCombat::CZPSBotCombat(CZPSBot* bot) :
	ICombat(bot)
{
	m_shouldwalk = true;
}

void CZPSBotCombat::OnDifficultyProfileChanged()
{
	ICombat::OnDifficultyProfileChanged();

	m_shouldwalk = GetBot<CZPSBot>()->GetDifficultyProfile()->GetGameAwareness() >= 20;
}

void CZPSBotCombat::Update()
{
	ICombat::Update();

	CZPSBot* bot = GetBot<CZPSBot>();
	CZPSBotMovement* movement = bot->GetMovementInterface();

	if (GetBot<CZPSBot>()->GetMyZPSTeam() == zps::ZPSTeam::ZPS_TEAM_ZOMBIES)
	{
		if (!movement->IsRunning())
		{
			movement->RequestMovementTypeChange(IMovement::MovementType::MOVE_RUNNING, IMovement::MovementRequestPriority::MOVEREQUEST_PRIO_LOW, 1.0f, "Start running (Zombie)");
		}

		return;
	}

	if (GetTimeSinceLOSWasLost() >= 10.0f)
	{
		if (!movement->IsWalking())
		{
			// walk to save and regenerate stamina
			movement->RequestMovementTypeChange(IMovement::MovementType::MOVE_WALKING, IMovement::MovementRequestPriority::MOVEREQUEST_PRIO_LOW, 1.0f, "Start walking (Survivor)");
		}
	}
	else
	{
		if (!movement->IsRunning())
		{
			movement->RequestMovementTypeChange(IMovement::MovementType::MOVE_RUNNING, IMovement::MovementRequestPriority::MOVEREQUEST_PRIO_HIGH, 1.0f, "Start running (Survivor Combat)");
		}
	}
}

void CZPSBotCombat::Reset()
{
	ICombat::Reset();

	m_shouldwalk = GetBot<CZPSBot>()->GetDifficultyProfile()->GetGameAwareness() >= 20;
}

bool CZPSBotCombat::IsAbleToDodgeEnemies(const CKnownEntity* threat, const CBotWeapon* activeWeapon)
{
	CZPSBot* bot = GetBot<CZPSBot>();

	if (bot->GetMyZPSTeam() == zps::ZPSTeam::ZPS_TEAM_ZOMBIES)
	{
		return ICombat::IsAbleToDodgeEnemies(threat, activeWeapon);
	}

	// Survivors only dodges if zombies are close
	if (bot->GetRangeTo(threat->GetLastKnownPosition()) <= 400.0f)
	{
		return true;
	}

	return false;
}

void CZPSBotCombat::DodgeEnemies(const CKnownEntity* threat, const CBotWeapon* activeWeapon)
{
	CZPSBot* bot = GetBot<CZPSBot>();
	CZPSBotMovement* mover = bot->GetMovementInterface();

	if (bot->GetMyZPSTeam() == zps::ZPSTeam::ZPS_TEAM_ZOMBIES)
	{
		// zombies just uses base
		return ICombat::DodgeEnemies(threat, activeWeapon);
	}

	constexpr float BACKUP_DIST = 72.0f;

	Vector origin = bot->GetAbsOrigin();
	Vector to = UtilHelpers::math::BuildDirectionVector(threat->GetLastKnownPosition(), origin);
	Vector backUp = origin + (to * BACKUP_DIST);

	if (!mover->HasPotentialGap(origin, backUp))
	{
		mover->MoveTowards(backUp, IMovement::MOVEWEIGHT_DODGE);
	}
}
