#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <mods/tf2/teamfortress2mod.h>
#include "bot/tf2/tf2bot.h"
#include "tf2bot_tactical.h"
#include "tf2bot_maintask.h"


AITask<CTF2Bot>* CTF2BotMainTask::InitialNextTask()
{
	return new CTF2BotTacticalTask;
}

TaskResult<CTF2Bot> CTF2BotMainTask::OnTaskUpdate(CTF2Bot* bot)
{
	return Continue();
}

Vector CTF2BotMainTask::GetTargetAimPos(CBaseBot* me, edict_t* entity, CBaseExtPlayer* player)
{
	Vector aimat(0.0f, 0.0f, 0.0f);
	CTF2Bot* tf2bot = static_cast<CTF2Bot*>(me);

	if (player)
	{
		InternalAimAtEnemyPlayer(tf2bot, player, aimat);
	}
	else
	{
		aimat = UtilHelpers::getWorldSpaceCenter(entity);
	}

	return aimat;
}

void CTF2BotMainTask::InternalAimAtEnemyPlayer(CTF2Bot* me, CBaseExtPlayer* player, Vector& result)
{
	auto myweapon = me->GetActiveWeapon();

	if (!myweapon) // how?
	{
		result = player->WorldSpaceCenter();
		return;
	}

	int index = -1;
	auto classname = gamehelpers->GetEntityClassname(myweapon);
	auto mod = CTeamFortress2Mod::GetTF2Mod();
	entprops->GetEntProp(gamehelpers->IndexOfEdict(myweapon), Prop_Send, "m_iItemDefinitionIndex", index);

	auto weaponinfo = [mod, &index, classname]() {
		auto& wim = mod->GetWeaponInfoManager();
		auto info = wim.GetWeaponInfoByEconIndex(index);

		if (!info)
		{
			info = wim.GetWeaponInfoByClassname(classname);
		}

		return info;
	}();

	if (!weaponinfo) // no weapon info, just aim at the center
	{
		result = player->WorldSpaceCenter();
		return;
	}

	// TO-DO: Add per weapon handling
	result = player->WorldSpaceCenter();
}


