#include NAVBOT_PCH_FILE
#include "nav.h"
#include "nav_mesh.h"
#include "nav_ladder.h"

CON_COMMAND_F(sm_nav_ladder_set_blocked, "Sets a ladder blocked/unblocked status for a speficic team.", FCVAR_GAMEDLL | FCVAR_CHEAT)
{
	DECLARE_COMMAND_ARGS;

	if (args.ArgC() < 3)
	{
		META_CONPRINT("[SM] Usage: sm_nav_ladder_set_blocked <team index> <yes/no>\n");
		return;
	}

	if (!extension->IsCommandIssuedByServerAdmin())
	{
		return;
	}

	CNavLadder* ladder = TheNavMesh->GetMarkedLadder();

	if (ladder == nullptr)
	{
		META_CONPRINT("Mark a ladder with sm_nav_mark first! \n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	int teamID = atoi(args[1]);
	bool blocked = UtilHelpers::StringToBoolean(args[2]);
	ladder->SetBlockedForTeam(teamID, blocked);
	META_CONPRINTF("Ladder %u is now %s for team %i. \n", ladder->GetID(), blocked ? "BLOCKED" : "UNBLOCKED", teamID);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_SUCCESS);
	TheNavMesh->SetMarkedLadder(nullptr);
}

CON_COMMAND_F(sm_nav_ladder_query_blocked, "Queries a ladder blocked/unblocked status.", FCVAR_GAMEDLL | FCVAR_CHEAT)
{
	if (!extension->IsCommandIssuedByServerAdmin())
	{
		return;
	}

	CNavLadder* ladder = TheNavMesh->GetMarkedLadder();

	if (ladder == nullptr)
	{
		META_CONPRINT("Mark a ladder with sm_nav_mark first! \n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	ladder->PrintTeamBlockedStatus();
}

