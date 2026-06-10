#include NAVBOT_PCH_FILE
#include <extension.h>
#include "helpers.h"
#include "pawnutils.h"

bool pawnutils::IsValidClientIndex(IPluginContext* context, int index)
{
	if (index <= 0 || index > gpGlobals->maxClients)
	{
		context->ReportError("Client index %i is out of bounds!", index);
		return false;
	}

    return true;
}

bool pawnutils::IsClientInGame(IPluginContext* context, int index)
{
	SourceMod::IGamePlayer* player = playerhelpers->GetGamePlayer(index);

	if (!player)
	{
		context->ReportError("Invalid client of index %i!", index);
		return false;
	}

	if (!player->IsInGame())
	{
		context->ReportError("Client %i is not in game!", index);
		return false;
	}

	return true;
}
