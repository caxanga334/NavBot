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
			movement->ChangeMovementType(IMovement::MovementType::MOVE_RUNNING);
		}

		return;
	}

	if (GetTimeSinceLOSWasLost() >= 10.0f)
	{
		if (!movement->IsWalking())
		{
			// walk to save and regenerate stamina
			movement->ChangeMovementType(IMovement::MovementType::MOVE_WALKING);
		}
	}
	else
	{
		if (!movement->IsRunning())
		{
			movement->ChangeMovementType(IMovement::MovementType::MOVE_RUNNING);
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
	if (GetBot<CZPSBot>()->GetMyZPSTeam() == zps::ZPSTeam::ZPS_TEAM_ZOMBIES)
	{
		return true;
	}

	// disabled for survivors
	return false;
}
