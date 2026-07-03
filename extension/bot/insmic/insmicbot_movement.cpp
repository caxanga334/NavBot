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

