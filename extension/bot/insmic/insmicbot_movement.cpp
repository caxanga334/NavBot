#include NAVBOT_PCH_FILE
#include "insmicbot.h"
#include "insmicbot_movement.h"

CInsMICBotMovement::CInsMICBotMovement(CInsMICBot* bot) :
	IMovement(bot)
{
}

float CInsMICBotMovement::GetHullWidth() const
{
	return s_playerhull.width;
}

float CInsMICBotMovement::GetStandingHullHeight() const
{
	return s_playerhull.stand_height;
}

float CInsMICBotMovement::GetCrouchedHullHeight() const
{
	return s_playerhull.crouch_height;
}

float CInsMICBotMovement::GetProneHullHeight() const
{
	return s_playerhull.prone_height;
}

bool CInsMICBotMovement::IsCompletelyCrouched() const
{
	return GetBot<CInsMICBot>()->GetStance() == insmic::Stance_t::STANCE_CROUCH;
}

void CInsMICBotMovement::AdjustSpeedForPath(CMeshNavigator* path)
{
	// insurgency is a bit buggy when moving slow, always move at max speed
	SetDesiredSpeed(GetRunSpeed());
}

void CInsMICBotMovement::DetermineIdealPostureForPath(const CMeshNavigator* path)
{
	IMovement::DetermineIdealPostureForPath(path);

	CInsMICBot* bot = GetBot<CInsMICBot>();

	// we don't have prone only areas implemented right now so always go back to standing.
	if (bot->GetStance() == insmic::Stance_t::STANCE_PRONE)
	{
		bot->GetControlInterface()->PressAlt1Button();
	}
}

