#include <extension.h>
#include <manager.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include "tf2bot.h"

#undef max
#undef min
#undef clamp

CTF2Bot::CTF2Bot(edict_t* edict) : CBaseBot(edict)
{
	m_tf2movement = std::make_unique<CTF2BotMovement>(this);
	m_tf2controller = std::make_unique<CTF2BotPlayerController>(this);
	m_tf2sensor = std::make_unique<CTF2BotSensor>(this);
	m_tf2behavior = std::make_unique<CTF2BotBehavior>(this);
}

CTF2Bot::~CTF2Bot()
{
}

void CTF2Bot::TryJoinGame()
{
	JoinTeam();
	auto tfclass = CTeamFortress2Mod::GetTF2Mod()->SelectAClassForBot(this);
	JoinClass(tfclass);
}

void CTF2Bot::Spawn()
{
	CBaseBot::Spawn();
}

void CTF2Bot::FirstSpawn()
{
	CBaseBot::FirstSpawn();
}

int CTF2Bot::GetMaxHealth() const
{
	return tf2lib::GetPlayerMaxHealth(GetIndex());
}

TeamFortress2::TFClassType CTF2Bot::GetMyClassType() const
{
	return tf2lib::GetPlayerClassType(GetIndex());
}

TeamFortress2::TFTeam CTF2Bot::GetMyTFTeam() const
{
	return tf2lib::GetEntityTFTeam(GetIndex());
}

void CTF2Bot::JoinClass(TeamFortress2::TFClassType tfclass) const
{
	auto szclass = tf2lib::GetClassNameFromType(tfclass);
	char command[128];
	ke::SafeSprintf(command, sizeof(command), "joinclass %s", szclass);
	FakeClientCommand(command);
}

void CTF2Bot::JoinTeam() const
{
	FakeClientCommand("jointeam auto");
}
