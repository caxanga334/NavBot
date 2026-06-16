#include NAVBOT_PCH_FILE
#include "insmic_shareddefs.h"
#include "insmic_modhelpers.h"

int CInsMICModHelpers::GetEntityTeamNumber(CBaseEntity* entity) const
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

bool CInsMICModHelpers::IsCombatCharacter(CBaseEntity* entity) const
{
	// CBaseCombatCharacter doesn't exists in insurgency, check for the networked property instead of the datamap
	return entprops->HasEntProp(entity, Prop_Send, "m_hActiveWeapon");
}

bool CInsMICModHelpers::IsPlayableTeam(int teamNum) const
{
	switch (teamNum)
	{
	case static_cast<int>(insmic::InsMICTeam::INSMICTEAM_UNASSINGED):
		[[fallthrough]];
	case static_cast<int>(insmic::InsMICTeam::INSMICTEAM_SPECTATOR):
		return false;
	default:
		return true;
	}
}
