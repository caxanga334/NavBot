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

CON_COMMAND_F(sm_nav_ladder_connect, "Connects a ladder to another ladder.", FCVAR_GAMEDLL | FCVAR_CHEAT)
{
	CNavLadder* markedLadder = TheNavMesh->GetMarkedLadder();

	if (markedLadder == nullptr)
	{
		META_CONPRINT("Mark a ladder with sm_nav_mark first! \n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	CNavLadder* other = TheNavMesh->GetSelectedLadder();

	if (other == nullptr)
	{
		META_CONPRINT("Aim at the second ladder to connect! \n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	if (markedLadder == other)
	{
		META_CONPRINT("Can't connect a ladder to itself! \n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	markedLadder->ConnectTo(other);
	META_CONPRINTF("Ladders #%u and #%u are now connected! \n", markedLadder->GetID(), other->GetID());
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_SUCCESS);
}

CON_COMMAND_F(sm_nav_ladder_disconnect, "Disconnects a ladder to another ladder.", FCVAR_GAMEDLL | FCVAR_CHEAT)
{
	CNavLadder* markedLadder = TheNavMesh->GetMarkedLadder();

	if (markedLadder == nullptr)
	{
		META_CONPRINT("Mark a ladder with sm_nav_mark first! \n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	CNavLadder* other = TheNavMesh->GetSelectedLadder();

	if (other == nullptr)
	{
		META_CONPRINT("Aim at the second ladder to connect! \n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	if (markedLadder == other)
	{
		return;
	}

	markedLadder->Disconnect(other);
	META_CONPRINTF("Ladders #%u and #%u have been disconencted! \n", markedLadder->GetID(), other->GetID());
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_SUCCESS);
}
