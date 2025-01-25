#include <extension.h>
#include <bot/blackmesa/bmbot.h>
#include "bmbot_main_task.h"

CBlackMesaBotMainTask::CBlackMesaBotMainTask()
{
}

TaskResult<CBlackMesaBot> CBlackMesaBotMainTask::OnTaskUpdate(CBlackMesaBot* bot)
{
	return Continue();
}


