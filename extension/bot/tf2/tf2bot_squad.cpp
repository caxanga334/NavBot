#include NAVBOT_PCH_FILE
#include <mods/tf2/tf2lib.h>
#include <mods/tf2/teamfortress2mod.h>
#include "tf2bot.h"
#include "tf2bot_squad.h"

CTF2BotSquad::CTF2BotSquad(CTF2Bot* bot) :
	ISquad(bot)
{
}

bool CTF2BotSquad::CanJoinSquads() const
{
	CTF2Bot* bot = GetBot<CTF2Bot>();

	switch (bot->GetMyClassType())
	{
	case TeamFortress2::TFClassType::TFClass_Engineer:
		[[fallthrough]];
	case TeamFortress2::TFClassType::TFClass_Spy:
		return false;
	default:
		break;
	}

	return true;
}

bool CTF2BotSquad::CanLeadSquads() const
{
	CTF2Bot* bot = GetBot<CTF2Bot>();

	switch (bot->GetMyClassType())
	{
	case TeamFortress2::TFClassType::TFClass_Medic:
		[[fallthrough]];
	case TeamFortress2::TFClassType::TFClass_Engineer:
		[[fallthrough]];
	case TeamFortress2::TFClassType::TFClass_Spy:
		return false;
	default:
		break;
	}

	return true;
}

bool CTF2BotSquad::UsesSquadBehavior() const
{
	CTF2Bot* bot = GetBot<CTF2Bot>();

	switch (bot->GetMyClassType())
	{
	case TeamFortress2::TFClassType::TFClass_Medic:
		[[fallthrough]];
	case TeamFortress2::TFClassType::TFClass_Engineer:
		[[fallthrough]];
	case TeamFortress2::TFClassType::TFClass_Spy:
		return false;
	default:
		break;
	}

	return true;
}
