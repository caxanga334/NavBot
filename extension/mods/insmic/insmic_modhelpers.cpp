#include NAVBOT_PCH_FILE
#include "insmic_modhelpers.h"

int CInsMICModHelpers::GetEntityTeamNumber(CBaseEntity* entity)
{
	/*
	* Insurgency doesn't have m_iTeamNum.
	* Only players and the team manager entities have team numbers.
	* For players it's CINSPlayer::m_iTeamID, however it's not a netprop or datamap.
	* Player info GetTeamIndex returns it.
	*/

	if (IsPlayer(entity))
	{
		SourceMod::IGamePlayer* gp = playerhelpers->GetGamePlayer(UtilHelpers::IndexOfEntity(entity));

		if (gp && gp->GetPlayerInfo() != nullptr)
		{
			return gp->GetPlayerInfo()->GetTeamIndex();
		}
	}

	return 0;
}

bool CInsMICModHelpers::IsCombatCharacter(CBaseEntity* entity)
{
	// CBaseCombatCharacter doesn't exists in insurgency, check for the networked property instead of the datamap
	return entprops->HasEntProp(entity, Prop_Send, "m_hActiveWeapon");
}
