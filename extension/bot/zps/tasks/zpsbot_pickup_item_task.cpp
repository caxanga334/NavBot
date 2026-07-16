#include NAVBOT_PCH_FILE
#include <mods/zps/zps_lib.h>
#include <bot/zps/zpsbot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include "zpsbot_pickup_item_task.h"

CZPSBotPickupItemTask::CZPSBotPickupItemTask(CBaseEntity* item) :
	m_item(item)
{
	m_inventoryCheckDone = false;
}

TaskResult<CZPSBot> CZPSBotPickupItemTask::OnTaskUpdate(CZPSBot* bot)
{
	return Continue();
}
