#include NAVBOT_PCH_FILE
#include <mods/zps/zps_mod.h>
#include "zpsbot.h"
#include "zpsbot_squads.h"

CZPSBotSquad::CZPSBotSquad(CZPSBot* bot) :
	ISquad(bot)
{
}

bool CZPSBotSquad::CanLeadSquads() const
{
	return true;
}

CZPSBotSquad* CZPSBotSquad::GetFirstCarrierSquadInterface(CZPSBot* bot, CBaseEntity** humancarrier)
{
	CBaseExtPlayer* carrier = nullptr;

	auto func = [&carrier](CBaseExtPlayer* player) {

		if (carrier) { return; }

		if (player->IsAlive() && zpslib::IsPlayerCarrier(player->GetEntity()))
		{
			carrier = player;
		}
	};

	extmanager->ForEachClient(func);

	if (carrier)
	{
		if (carrier->IsExtensionBot())
		{
			return static_cast<CZPSBotSquad*>(carrier->MyBotPointer()->GetSquadInterface());
		}

		// human carrier
		ISquad* squad = ISquad::GetHumanPlayerSquad(carrier->GetEntity(), true);

		if (!squad)
		{
			squad = bot->GetSquadInterface();

			if (squad->IsInASquad())
			{
				return nullptr;
			}

			*humancarrier = carrier->GetEntity();
			return static_cast<CZPSBotSquad*>(squad);
		}

		return static_cast<CZPSBotSquad*>(squad);
	}

	return nullptr;
}
