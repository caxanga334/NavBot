#include NAVBOT_PCH_FILE
#include "hl1mp_bot.h"
#include "hl1mp_bot_movement.h"

CHL1MPBotMovement::CHL1MPBotMovement(CHL1MPBot* bot) :
	IMovement(bot)
{
}

float CHL1MPBotMovement::GetMaxGapJumpDistance() const
{
	if (GetBot<CHL1MPBot>()->HasLongJumpModule())
	{
		return 500.0f;
	}

	return 200.0f;
}
