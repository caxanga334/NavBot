#include NAVBOT_PCH_FILE
#include "insmicbot.h"

CInsMICBot::CInsMICBot(edict_t* edict) :
	CBaseBot(edict)
{
}

bool CInsMICBot::HasJoinedGame()
{
	insmic::InsMICTeam myteam = GetMyInsurgencyTeam();
	return myteam == insmic::InsMICTeam::INSMICTEAM_USMC || myteam == insmic::InsMICTeam::INSMICTEAM_INSURGENTS;
}

void CInsMICBot::TryJoinGame()
{
	// temporary to get at least 1 bot spawning
	DelayedFakeClientCommand("pfullsetup 0 5");
}
