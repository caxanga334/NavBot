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

CZPSBotSquad* CZPSBotSquad::GetFirstCarrierSquadInterface(CZPSBot* bot)
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
		else
		{
			// human carrier
			ISquad* squad = ISquad::GetSquadInterfaceOfHumanLeader(carrier->GetEntity());

			if (!squad)
			{
				squad = bot->GetSquadInterface();

				if (squad->IsInASquad())
				{
					return nullptr;
				}

				if (squad->CreateSquad(carrier->GetEntity()))
				{
					return static_cast<CZPSBotSquad*>(squad);
				}

				return nullptr;
			}

			return static_cast<CZPSBotSquad*>(squad);
		}
	}

	return nullptr;
}
