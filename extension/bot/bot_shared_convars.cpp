#include NAVBOT_PCH_FILE
#include <extension.h>
#include "bot_shared_convars.h"

ConVar sm_navbot_bot_low_health_ratio("sm_navbot_bot_low_health_ratio", "0.6", FCVAR_GAMEDLL, "The bot is considered to be low on health if their health percentage is lower than this.", true, -2.0f, true, 1.0f);

ConVar sm_navbot_bot_no_ammo_check("sm_navbot_bot_no_ammo_check", "0", FCVAR_GAMEDLL, "If enabled, bots will not check for ammo.");