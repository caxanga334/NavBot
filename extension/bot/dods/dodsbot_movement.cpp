#include <extension.h>
#include <mods/dods/dods_shareddefs.h>
#include "dodsbot.h"
#include "dodsbot_movement.h"

CDoDSBotMovement::CDoDSBotMovement(CBaseBot* bot) :
	IMovement(bot)
{
}

CDoDSBotMovement::~CDoDSBotMovement()
{
}

float CDoDSBotMovement::GetCrouchedHullHeight()
{
	return dayofdefeatsource::DOD_PLAYER_CROUCHING_HEIGHT * GetBot<CDoDSBot>()->GetModelScale();
}

float CDoDSBotMovement::GetProneHullHeight()
{
	return dayofdefeatsource::DOD_PLAYER_PRONE_HEIGHT * GetBot<CDoDSBot>()->GetModelScale();
}
