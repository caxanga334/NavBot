#include NAVBOT_PCH_FILE

#include <navmesh/nav_area.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_pathfind.h>
#include <navmesh/nav_trace.h>
#include <mods/basemod.h>
#include <entities/basecombatweapon.h>
#include <sdkports/debugoverlay_shared.h>
#include <sdkports/sdk_entityfactory.h>
#include <bot/basebot.h>
#include <util/prediction.h>

#if SOURCE_ENGINE <= SE_DARKMESSIAH
#include <util/commandargs_episode1.h>
#include <sdkports/sdk_convarref_ep1.h>
#endif // SOURCE_ENGINE <= SE_DARKMESSIAH

class CToolsPlayerPathCost : public NavAStarPathCost
{
public:
	CToolsPlayerPathCost(const char* preset);

	float operator()(CNavArea* area, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator) const;

	void SetTeamNum(int team) { m_teamnum = team; }

private:
	float m_stepheight;
	float m_maxjumpheight;
	float m_maxdoublejumpheight;
	float m_maxdropheight;
	float m_maxgapjumpdistance;
	bool m_candoublejump;
	bool m_canblastjump;
	int m_teamnum;
};

CToolsPlayerPathCost::CToolsPlayerPathCost(const char* preset)
{
	m_stepheight = 18.0f; // this is generally the same for every game.
	m_teamnum = NAV_TEAM_ANY;

	if (std::strcmp(preset, "tf2_default") == 0)
	{
		m_maxjumpheight = 72.0f;
		m_maxdoublejumpheight = 116.0f;
		m_maxdropheight = 350.0f;
		m_maxgapjumpdistance = 258.0f;
		m_candoublejump = false;
		m_canblastjump = false;
	}
	else if (std::strcmp(preset, "tf2_scout") == 0)
	{
		m_maxjumpheight = 72.0f;
		m_maxdoublejumpheight = 116.0f;
		m_maxdropheight = 350.0f;
		m_maxgapjumpdistance = 600.0f;
		m_candoublejump = true;
		m_canblastjump = false;
	}
	else if (std::strcmp(preset, "tf2_soldier") == 0)
	{
		m_maxjumpheight = 72.0f;
		m_maxdoublejumpheight = 116.0f;
		m_maxdropheight = 350.0f;
		m_maxgapjumpdistance = 216.0f;
		m_candoublejump = true;
		m_canblastjump = false;
	}
	else if (std::strcmp(preset, "dods") == 0)
	{
		m_maxjumpheight = 57.0f;
		m_maxdoublejumpheight = 116.0f;
		m_maxdropheight = 200.0f;
		m_maxgapjumpdistance = 160.0f;
		m_candoublejump = false;
		m_canblastjump = false;
	}
	else /* hl2 */
	{
		m_maxjumpheight = 56.0f;
		m_maxdoublejumpheight = 56.0f;
		m_maxdropheight = 350.0f;
		m_maxgapjumpdistance = 220.0f;
		m_candoublejump = false;
		m_canblastjump = false;
	}
}

float CToolsPlayerPathCost::operator()(CNavArea* area, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator) const
{
	if (fromArea == nullptr)
	{
		// first area in path, no cost
		return 0.0f;
	}

	if (m_teamnum >= TEAM_UNASSIGNED)
	{
		if (area->IsBlocked(m_teamnum))
		{
			return -1.0f; // area is blocked
		}
	}

	float dist = 0.0f;

	if (link != nullptr)
	{
		dist = link->GetConnectionLength();
	}
	else if (ladder != nullptr) // experimental, very few maps have 'true' ladders
	{
		dist = ladder->m_length;
	}
	else if (elevator != nullptr)
	{
		auto fromFloor = fromArea->GetMyElevatorFloor();

		if (!fromFloor->HasCallButton() && !fromFloor->is_here)
		{
			return -1.0f; // Unable to use this elevator, lacks a button to call it to this floor and is not on this floor
		}

		dist = elevator->GetLengthBetweenFloors(fromArea, area);
	}
	else
	{
		dist = (area->GetCenter() + fromArea->GetCenter()).Length();
	}


	// only check gap and height on common connections
	if (link == nullptr && elevator == nullptr && ladder == nullptr)
	{
		float deltaZ = fromArea->ComputeAdjacentConnectionHeightChange(area);

		if (deltaZ >= m_stepheight)
		{
			if (m_candoublejump)
			{
				if (deltaZ > m_maxdoublejumpheight)
				{
					// too high to reach with double jumps
					return -1.0f;
				}
			}
			else
			{
				if (deltaZ > m_maxjumpheight)
				{
					// too high to reach with regular jumps
					return -1.0f;
				}
			}

			// jump type is resolved by the navigator

			// add jump penalty
			constexpr auto jumpPenalty = 2.0f;
			dist *= jumpPenalty;
		}
		else if (deltaZ < -m_maxdropheight)
		{
			// too far to drop
			return -1.0f;
		}

		float gap = fromArea->ComputeAdjacentConnectionGapDistance(area);

		if (gap >= m_maxgapjumpdistance)
		{
			return -1.0f; // can't jump over this gap
		}
	}
	else if (link != nullptr)
	{
		// Don't use double jump links if we can't perform a double jump
		if (link->GetType() == OffMeshConnectionType::OFFMESH_DOUBLE_JUMP && !m_candoublejump)
		{
			return -1.0f;
		}
		else if (link->GetType() == OffMeshConnectionType::OFFMESH_BLAST_JUMP && !m_canblastjump)
		{
			return -1.0f;
		}
		else if (link->GetType() == OffMeshConnectionType::OFFMESH_JUMP_OVER_GAP)
		{
			if ((link->GetStart() - link->GetEnd()).Length() > m_maxgapjumpdistance)
			{
				return -1.0f;
			}
		}
	}

	return dist;
}

CON_COMMAND_F(sm_navbot_tool_build_path, "Builds a path from your current position to the marked nav area.", FCVAR_CHEAT)
{
	DECLARE_COMMAND_ARGS;

	if (engine->IsDedicatedServer())
	{
		META_CONPRINT("This command can only be used on a listen server! \n");
		return;
	}

	if (args.ArgC() < 2)
	{
		META_CONPRINT("[SM] Usage: sm_navbot_tool_build_path -preset <preset> -team <my or team number> \n");
		META_CONPRINT("Available Presets: tf2_default tf2_scout tf2_soldier dods default \n");
		return;
	}

	char preset[64];
	std::memset(preset, 0, sizeof(preset));
	
	const char* argPreset = args.FindArg("-preset");

	if (!argPreset)
	{
		META_CONPRINTF("Missing -preset!\n");
		return;
	}

	ke::SafeSprintf(preset, sizeof(preset), "%s", argPreset);

	edict_t* host = UtilHelpers::GetListenServerHost();
	const Vector& origin = host->GetCollideable()->GetCollisionOrigin();
	SourceMod::IGamePlayer* player = playerhelpers->GetGamePlayer(host);

	if (!player || !player->GetPlayerInfo())
	{
		META_CONPRINT("Error: NULL player! \n");
		return;
	}

	CNavArea* start = TheNavMesh->GetNearestNavArea(origin, 512.0f, true, true, player->GetPlayerInfo()->GetTeamIndex());
	CNavArea* end = TheNavMesh->GetMarkedArea();

	if (start == nullptr)
	{
		META_CONPRINT("No Nav Area found near you! \n");
		META_CONPRINT("The nearest area may be blocked for your current team. \n");
		return;
	}

	if (end == nullptr)
	{
		META_CONPRINT("No End Area. Mark the destination area with sm_nav_mark! \n");
		return;
	}

	CToolsPlayerPathCost cost{ preset };
	NavAStarHeuristicCost heuristic;
	INavAStarSearch<CNavArea> search;
	search.SetStart(start);
	search.SetGoalArea(end);

	const char* argTeam = args.FindArg("-team");

	if (argTeam)
	{
		if (std::strcmp(argTeam, "my") == 0)
		{
			int teamNum = player->GetPlayerInfo()->GetTeamIndex();
			cost.SetTeamNum(teamNum);
		}
		else
		{
			int teamNum = atoi(argTeam);
			cost.SetTeamNum(teamNum);
		}
	}

	auto tstart = std::chrono::high_resolution_clock::now();
	search.DoSearch(cost, heuristic);
	auto tend = std::chrono::high_resolution_clock::now();

	const std::chrono::duration<double, std::milli> millis = (tend - tstart);

	META_CONPRINTF("INavAStarSearch<CNavArea>::DoSearch() took %g ms.\n", millis.count());

	if (search.FoundPath())
	{
		META_CONPRINT("Found path! \n");
	}
	else
	{
		META_CONPRINT("No path found! \n");
	}

	for (auto area : search.GetPath())
	{
		area->DrawFilled(0, 128, 0, 64, 30.0f);
	}
}

CON_COMMAND_F(sm_navbot_tool_report_player_info, "Prints some information about the player entity.", FCVAR_CHEAT)
{
	edict_t* host = UtilHelpers::GetListenServerHost();
	IPlayerInfo* info = playerinfomanager->GetPlayerInfo(host);

	const Vector& mins = host->GetCollideable()->OBBMins();
	const Vector& maxs = host->GetCollideable()->OBBMaxs();

	META_CONPRINTF("OBB Mins: %3.2f %3.2f %3.2f\n", mins.x, mins.y, mins.z);
	META_CONPRINTF("OBB Maxs: %3.2f %3.2f %3.2f\n", maxs.x, maxs.y, maxs.z);

	Vector viewoffset;

	if (entprops->GetEntPropVector(host, Prop_Data, "m_vecViewOffset", viewoffset))
	{
		META_CONPRINTF("View Offset: %3.2f %3.2f %3.2f \n", viewoffset.x, viewoffset.y, viewoffset.z);
	}
	else
	{
		META_CONPRINT("Warning: Failed to access m_vecViewOffset! \n");
	}

	if (info)
	{
		const char* model = info->GetModelName();

		if (model && model[0] != '\0')
		{
			META_CONPRINTF("Model: %s \n", model);
		}

		const char* weapon = info->GetWeaponName();

		if (weapon && weapon[0] != '\0')
		{
			META_CONPRINTF("Weapon Name: %s \n", weapon);
		}

		META_CONPRINTF("Health %i Max Health %i Armor %i Team Index %i\n", info->GetHealth(), info->GetMaxHealth(), info->GetArmorValue(), info->GetTeamIndex());
		META_CONPRINTF("Frags %i Deaths %i \n", info->GetFragCount(), info->GetDeathCount());
		META_CONPRINTF("Angles: %s \n", UtilHelpers::textformat::FormatAngles(info->GetAbsAngles()));

		Vector ear;
		gameclients->ClientEarPosition(host, &ear);

		const Vector origin = info->GetAbsOrigin();
		const float origin_to_ear = (ear.z - origin.z);
		META_CONPRINTF("Difference between ear and abs origin height: %3.2f \n", origin_to_ear);
	}
}

CON_COMMAND_F(sm_navbot_tool_projectile_aim, "Tests projectile aim parameters", FCVAR_CHEAT)
{
	DECLARE_COMMAND_ARGS;

	if (engine->IsDedicatedServer())
	{
		Msg("This command can only be used on a Listen Server! \n");
		return;
	}

	if (args.ArgC() < 9)
	{
		Msg("[SM] Usage: sm_navbot_tool_projectile_aim <target x> <target y> <target z> <projectile speed> <projectile gravity> <ballistics start range> <ballistics end range> <ballistics min rate> <ballistics max rate>\n");
		return;
	}

	ConVar* sv_gravity = g_pCVar->FindVar("sv_gravity");

	if (!sv_gravity)
	{
		Warning("sm_navbot_tool_projectile_aim FindVar sv_gravity == NULL! \n");
	}

	CBaseExtPlayer host{ UtilHelpers::GetListenServerHost() };
	const float x = atof(args[1]);
	const float y = atof(args[2]);
	const float z = atof(args[3]);
	const Vector targetPos{ x,y,z };
	const float projSpeed = atof(args[4]);
	const float projGrav = atof(args[5]);
	const float startRange = atof(args[6]);
	const float endRange = atof(args[7]);
	const float startRate = atof(args[8]);
	const float endRate = atof(args[9]);
	const Vector eyePos = host.GetEyeOrigin();
	const Vector enemyVel{ 0.0f, 0.0f, 0.0f }; // always zero in this case
	float rangeTo = (targetPos - eyePos).Length();
	Vector aimPos = pred::SimpleProjectileLead(targetPos, enemyVel, projSpeed, rangeTo);
	rangeTo = (eyePos - aimPos).Length(); // update range for gravity compensation
	const float elevation_rate = RemapValClamped(rangeTo, startRange, endRange, startRate, endRate);
	const float elevation_Z = pred::GravityComp(rangeTo, projGrav, elevation_rate);
	aimPos.z += elevation_Z;

	Vector to = (aimPos - eyePos);
	to.NormalizeInPlace();
	QAngle viewAngles;
	VectorAngles(to, viewAngles);
	host.SnapEyeAngles(viewAngles);

	debugoverlay->AddLineOverlay(eyePos, targetPos, 255, 0, 0, true, 20.0f); // eye to target pos
	debugoverlay->AddLineOverlay(eyePos, aimPos, 0, 128, 0, true, 20.0f); // eye to calculated pos
	debugoverlay->AddLineOverlay(aimPos, targetPos, 0, 128, 255, true, 20.0f); // aim pos to target
}

CON_COMMAND_F(sm_navbot_tool_projectile_aim_entity, "Tests projectile aim parameters", FCVAR_CHEAT)
{
	DECLARE_COMMAND_ARGS;

	if (engine->IsDedicatedServer())
	{
		Msg("This command can only be used on a Listen Server! \n");
		return;
	}

	if (args.ArgC() < 7)
	{
		Msg("[SM] Usage: sm_navbot_tool_projectile_aim <ent index> <projectile speed> <projectile gravity> <ballistics start range> <ballistics end range> <ballistics min rate> <ballistics max rate>\n");
		return;
	}

	ConVar* sv_gravity = g_pCVar->FindVar("sv_gravity");

	if (!sv_gravity)
	{
		Warning("sm_navbot_tool_projectile_aim FindVar sv_gravity == NULL! \n");
	}

	CBaseExtPlayer host{ UtilHelpers::GetListenServerHost() };
	const int entindex = atof(args[1]);
	CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(entindex);

	if (!pEntity)
	{
		Warning("No entity of index %i \n", entindex);
		return;
	}

	const Vector targetPos = UtilHelpers::getWorldSpaceCenter(pEntity);
	const float projSpeed = atof(args[2]);
	const float projGrav = atof(args[3]);
	const float startRange = atof(args[4]);
	const float endRange = atof(args[5]);
	const float startRate = atof(args[6]);
	const float endRate = atof(args[7]);
	const Vector eyePos = host.GetEyeOrigin();
	Vector enemyVel{ 0.0f, 0.0f, 0.0f };
	entityprops::GetEntityAbsVelocity(pEntity, enemyVel);
	float rangeTo = (targetPos - eyePos).Length();
	Vector aimPos = pred::SimpleProjectileLead(targetPos, enemyVel, projSpeed, rangeTo);
	rangeTo = (eyePos - aimPos).Length(); // update range for gravity compensation
	const float elevation_rate = RemapValClamped(rangeTo, startRange, endRange, startRate, endRate);
	const float elevation_Z = pred::GravityComp(rangeTo, projGrav, elevation_rate);
	aimPos.z += elevation_Z;

	Vector to = (aimPos - eyePos);
	to.NormalizeInPlace();
	QAngle viewAngles;
	VectorAngles(to, viewAngles);
	host.SnapEyeAngles(viewAngles);

	debugoverlay->AddLineOverlay(eyePos, targetPos, 255, 0, 0, true, 20.0f); // eye to target pos
	debugoverlay->AddLineOverlay(eyePos, aimPos, 0, 128, 0, true, 20.0f); // eye to calculated pos
	debugoverlay->AddLineOverlay(aimPos, targetPos, 0, 128, 255, true, 20.0f); // aim pos to target
}

CON_COMMAND_F(sm_navbot_tool_give_infammo, "Gives infinite reserve ammo to every player", FCVAR_CHEAT)
{
	if (engine->IsDedicatedServer())
	{
		return;
	}

	auto func = [](int client, edict_t* entity, SourceMod::IGamePlayer* player) {
		if (player->IsInGame())
		{
			for (int i = 0; i < MAX_AMMO_TYPES; i++)
			{
				entprops->SetEntProp(client, Prop_Send, "m_iAmmo", 9999, 4, i);
			}
		}
	};

	UtilHelpers::ForEachPlayer(func);
}

CON_COMMAND_F(sm_navbot_tool_bots_go_to, "Bots will move to your current position.", FCVAR_CHEAT)
{
	DECLARE_COMMAND_ARGS;

	if (engine->IsDedicatedServer())
	{
		Msg("This command can only be used on a Listen Server! \n");
		return;
	}
	
	edict_t* host = gamehelpers->EdictOfIndex(1);
	Vector goal = host->GetCollideable()->GetCollisionOrigin();

	const char* inputPos = args.FindArg("-pos");

	if (inputPos && inputPos[0] != '\0')
	{
		Vector v;
		int count = sscanf(inputPos, "%f %f %f", &v.x, &v.y, &v.z);

		if (count == 3)
		{
			goal = v;
		}
	}

	int areaID = args.FindArgInt("-area", -1);

	CNavArea* area = nullptr;

	if (areaID > 0)
	{
		area = TheNavMesh->GetNavAreaByID(static_cast<unsigned int>(areaID));
	}

	if (area)
	{
		goal = area->GetCenter();
	}

	auto func = [&goal](CBaseBot* bot) {
		bot->OnDebugMoveToCommand(goal);
	};

	extmanager->ForEachBot(func);
}

CON_COMMAND_F(sm_navbot_tool_reset_bots, "Reset the bot interfaces.", FCVAR_CHEAT)
{
	if (engine->IsDedicatedServer())
	{
		Msg("This command can only be used on a Listen Server! \n");
		return;
	}

	auto func = [](CBaseBot* bot) {
		bot->Reset();
	};

	extmanager->ForEachBot(func);
}

CON_COMMAND_F(sm_navbot_tool_toggle_non_solid, "Makes your non solid to triggers.", FCVAR_CHEAT)
{
	edict_t* host = gamehelpers->EdictOfIndex(1);

	static int oldsolidtype = -1;

	if (oldsolidtype < 0)
	{
		entprops->GetEntProp(1, Prop_Data, "m_nSolidType", oldsolidtype);
		entprops->SetEntProp(1, Prop_Data, "m_nSolidType", static_cast<int>(SolidType_t::SOLID_NONE));
		META_CONPRINTF("Setting to SOLID_NONE! Old = %i \n", oldsolidtype);
	}
	else
	{
		entprops->SetEntProp(1, Prop_Data, "m_nSolidType", oldsolidtype);
		META_CONPRINTF("Restoring to old solid type %i \n", oldsolidtype);
		oldsolidtype = -1;
	}	
}

CON_COMMAND_F(sm_navbot_tool_dump_weapons, "Lists all CBaseCombatCharacter weapons.", FCVAR_CHEAT)
{
	DECLARE_COMMAND_ARGS;

	if (args.ArgC() < 2)
	{
		META_CONPRINT("[SM] Usage: sm_navbot_tool_dump_weapons <client index> \n    Use 'self' to dump your weapons. \n");
		return;
	}

	int iEnt;

	if (std::strcmp(args[1], "self") == 0)
	{
		iEnt = 1; // listen server host
	}
	else
	{
		iEnt = atoi(args[1]);
	}

	std::size_t numWeapons = entprops->GetEntPropArraySize(iEnt, Prop_Send, "m_hMyWeapons");

	if (numWeapons == 0)
	{
		META_CONPRINT("Error: m_hMyWeapons size == 0! \n");
		return;
	}

	if (numWeapons != static_cast<std::size_t>(MAX_WEAPONS))
	{
		META_CONPRINTF("Warning: m_hMyWeapons size doesn't match SDK's MAX_WEAPONS definiton! %zu != %i \n", numWeapons, MAX_WEAPONS);
	}

	CBaseEntity* pEntity = UtilHelpers::EdictToBaseEntity(gamehelpers->EdictOfIndex(iEnt));

	CHandle<CBaseEntity>* pMyWeapons = entprops->GetPointerToEntData<CHandle<CBaseEntity>>(pEntity, Prop_Send, "m_hMyWeapons");

	if (!pMyWeapons)
	{
		META_CONPRINT("Failed to get address to CBaseCombatCharacter::m_hMyWeapons! \n");
		return;
	}

	for (std::size_t i = 0; i < numWeapons; i++)
	{
		CBaseEntity* pWeapon = pMyWeapons[i].Get();

		if (!pWeapon)
		{
			continue;
		}

		META_CONPRINTF("[%zu] Weapon %s #%i <%p> \n", i, gamehelpers->GetEntityClassname(pWeapon), UtilHelpers::IndexOfEntity(pWeapon), pWeapon);
	}

	CBaseBot* bot = extmanager->GetBotByIndex(iEnt);

	if (bot)
	{
		META_CONPRINTF("Dumping stored Weapon Information data of %s \n", bot->GetClientName());

		auto dumpweapons = [](const CBotWeapon* weapon) {
			META_CONPRINTF("Weapon \"%s\" #%i using config entry: %s \n", 
				weapon->GetClassname().c_str(), weapon->GetWeaponEconIndex(), weapon->GetWeaponInfo()->GetConfigEntryName());
		};

		bot->GetInventoryInterface()->ForEveryWeapon(dumpweapons);
	}
}

CON_COMMAND_F(sm_navbot_tool_weapon_report, "Reports some information about your currently held weapon.", FCVAR_CHEAT)
{
	CBaseExtPlayer player{ gamehelpers->EdictOfIndex(1) };

	CBaseEntity* weapon = player.GetActiveWeapon();

	if (!weapon)
	{
		META_CONPRINT("NULL Active Weapon! \n");
		return;
	}

	int entindex = UtilHelpers::IndexOfEntity(weapon);
	const char* classname = gamehelpers->GetEntityClassname(weapon);
	int itemindex = modhelpers->GetItemEconomyIndex(weapon);

	META_CONPRINTF("Weapon: \n  Item Item: %i\n  Entity Classname: %s\n  Entity Index: %i\n", itemindex, classname, entindex);

	entities::HBaseCombatWeapon bcw{ weapon };

	int clip1 = 0;
	int clip2 = 0;
	entprops->GetEntProp(entindex, Prop_Data, "m_iClip1", clip1);
	entprops->GetEntProp(entindex, Prop_Data, "m_iClip2", clip2);
	int primaryammotype = bcw.GetPrimaryAmmoType();
	int secondaryammotype = bcw.GetSecondaryAmmoType();

	if (clip1 < 0)
	{
		META_CONPRINT("Weapon doesn't uses clips for primary attack. \n");
	}

	if (clip2 < 0)
	{
		META_CONPRINT("Weapon doesn't uses clips for secondary attack. \n");
	}

	if (primaryammotype < 0)
	{
		META_CONPRINT("Weapon doesn't have a primary ammo type. \n");
	}
	else
	{
		META_CONPRINTF("Weapon's primary ammo type index: %i \n", primaryammotype);
	}

	if (secondaryammotype < 0)
	{
		META_CONPRINT("Weapon doesn't have a secondary ammo type. \n");
	}
	else
	{
		META_CONPRINTF("Weapon's secondary ammo type index: %i \n", secondaryammotype);
	}
}

CON_COMMAND_F(sm_navbot_tool_dump_model_info, "Dumps information about a specific entity model.", FCVAR_CHEAT | FCVAR_GAMEDLL)
{
	DECLARE_COMMAND_ARGS;

	if (args.ArgC() < 2)
	{
		META_CONPRINT("[SM] Usage: sm_navbot_tool_dump_model_info <ent index>\n");
		return;
	}

	CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(atoi(args[1]));

	if (!pEntity)
	{
		return;
	}

	if (!entprops->HasEntProp(pEntity, Prop_Data, "m_OnIgnite"))
	{
		datamap_t* dmap = gamehelpers->GetDataMap(pEntity);
		const char* classname = gamehelpers->GetEntityClassname(pEntity);

		if (dmap && classname)
		{
			META_CONPRINTF("Error: Entity %s <%s|%s> does not derives from CBaseAnimating! \n", args[1], classname, dmap->dataClassName);

			datamap_t* next = dmap;

			while (next != nullptr)
			{
				META_CONPRINTF("    %s\n", next->dataClassName);
				next = next->baseMap;
			}
		}

		return;
	}

	auto ptr = UtilHelpers::GetEntityModelPtr(UtilHelpers::BaseEntityToEdict(pEntity));

	if (ptr)
	{
		int numbones = ptr->numbones();

		for (int i = 0; i < numbones; i++)
		{
			auto bone = ptr->pBone(i);

			META_CONPRINTF("Bone %i: %s\n", i, bone->pszName());
		}

#if SOURCE_ENGINE >= SE_HL2DM && SOURCE_ENGINE <= SE_TF2

		int numseqs = ptr->GetNumSeq();

		for (int i = 0; i < numseqs; i++)
		{
			auto& seq = ptr->pSeqdesc(i);

			META_CONPRINTF("Sequence %i: %s - %s \n", i, seq.pszLabel(), seq.pszActivityName());
		}
#endif // #if SOURCE_ENGINE >= SE_HL2DM && SOURCE_ENGINE <= SE_TF2
	}
}

CON_COMMAND_F(sm_navbot_tool_show_bone_pos, "Shows the position of a specific bone on a given entity.", FCVAR_CHEAT | FCVAR_GAMEDLL)
{
	DECLARE_COMMAND_ARGS;

	if (args.ArgC() < 3)
	{
		META_CONPRINT("[SM] Usage: sm_navbot_tool_show_bone_pos <ent index> <bone name>\n");
		return;
	}

	CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(atoi(args[1]));

	if (!pEntity)
	{
		return;
	}

	if (!entprops->HasEntProp(pEntity, Prop_Data, "m_OnIgnite"))
	{
		datamap_t* dmap = gamehelpers->GetDataMap(pEntity);
		const char* classname = gamehelpers->GetEntityClassname(pEntity);

		if (dmap && classname)
		{
			META_CONPRINTF("Error: Entity %s <%s|%s> does not derives from CBaseAnimating! \n", args[1], classname, dmap->dataClassName);

			datamap_t* next = dmap;

			while (next != nullptr)
			{
				META_CONPRINTF("    %s\n", next->dataClassName);
				next = next->baseMap;
			}
		}

		return;
	}

	auto ptr = UtilHelpers::GetEntityModelPtr(UtilHelpers::BaseEntityToEdict(pEntity));

	if (ptr)
	{
		const char* name = args[2];
		Vector pos;
		QAngle angle;

		if (UtilHelpers::GetBonePosition(pEntity, ptr.get(), name, pos, angle))
		{
			Vector mins{ -2.0f, -2.0f, -2.0f };
			Vector maxs{ 2.0f, 2.0f, 2.0f };

			META_CONPRINTF("Bone position %s angles %s\n", UtilHelpers::textformat::FormatVector(pos), UtilHelpers::textformat::FormatAngles(angle));
			NDebugOverlay::Box(pos, mins, maxs, 255, 255, 0, 255, 10.0f);
		}
		else
		{
			META_CONPRINTF("Bone \"%s\" not found on model! \n", name);
			META_CONPRINT("Use \"sm_navbot_tool_dump_model_info\" to list all bone names. \n");
		}
	}
}

#if SOURCE_ENGINE == SE_TF2        \
	|| SOURCE_ENGINE == SE_CSS     \
	|| SOURCE_ENGINE == SE_DODS    \
	|| SOURCE_ENGINE == SE_HL2DM   \
	|| SOURCE_ENGINE == SE_SDK2013 \
	|| SOURCE_ENGINE == SE_BMS     \
	|| SOURCE_ENGINE == SE_BLADE   \
	|| SOURCE_ENGINE == SE_NUCLEARDAWN \
	|| SOURCE_ENGINE == SE_PVKII

CON_COMMAND_F(sm_navbot_tool_dump_entity_factories, "Entity factories dump.", FCVAR_GAMEDLL | FCVAR_CHEAT)
{
	CEntityFactoryDictionary* dict = reinterpret_cast<CEntityFactoryDictionary*>(servertools->GetEntityFactoryDictionary());
	
	char path[PLATFORM_MAX_PATH];
	smutils->BuildPath(SourceMod::PathType::Path_SM, path, sizeof(path), "logs/entityfactories.txt");

	std::fstream file;
	file.open(path, std::ios_base::out | std::ios_base::trunc);

	file << "NavBot dump of CEntityFactoryDictionary::m_Factories for " << smutils->GetGameFolderName() << "\n";

	int c = 1;

	for (unsigned short i = dict->m_Factories.First(); i != dict->m_Factories.InvalidIndex(); i = dict->m_Factories.Next(i))
	{
		file << c << " - " << dict->m_Factories.GetElementName(i) << "\n";
		c++;
	}

	file.close();

	META_CONPRINTF("Entity factories dumped to %s\n", path);
}

#endif

CON_COMMAND_F(sm_navbot_tool_dump_entity_datamaps, "Dumps the datamap of a specific entity", FCVAR_GAMEDLL | FCVAR_CHEAT)
{
	DECLARE_COMMAND_ARGS;

	if (args.ArgC() < 2)
	{
		META_CONPRINT("[SM] Usage: sm_navbot_tool_dump_entity_datamaps <ent index>\n");
		return;
	}

	CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(atoi(args[1]));

	if (!pEntity)
	{
		return;
	}

	UtilHelpers::datamap::DumpEntityDatamap(pEntity);
}

CON_COMMAND_F(sm_navbot_tool_trace_line, "Trace line debug", FCVAR_GAMEDLL)
{
	DECLARE_COMMAND_ARGS;

	if (args.ArgC() < 4)
	{
		META_CONPRINT("[SM] Usage: sm_navbot_tool_trace_line <mask option> <collision group> <filter option>\n");
		META_CONPRINT("  <mask option> : playersolid playersolidbrushonly npcsolid npcsolidbrushonly water opaque solid blocklos visible shot all team1 team2\n");
		META_CONPRINT("  <collision group> : none player plmove npc\n");
		META_CONPRINT("  <filter option> : simple navtransient navwalkable worldproponly hitall\n");
		return;
	}

	unsigned int mask = 0;
	CBaseExtPlayer host{ UtilHelpers::GetListenServerHost() };
	ITraceFilter* filter;
	trace_t tr;

	Vector start = host.GetEyeOrigin();
	QAngle eyeAngles = host.GetEyeAngles();
	Vector forward;
	AngleVectors(eyeAngles, &forward);
	forward.NormalizeInPlace();
	Vector end = start + (forward * 4096.0f);

	const char* arg1 = args[1];

	if (std::strcmp(arg1, "playersolid") == 0)
	{
		mask = MASK_PLAYERSOLID;
	}
	else if (std::strcmp(arg1, "playersolidbrushonly") == 0)
	{
		mask = MASK_PLAYERSOLID_BRUSHONLY;
	}
	else if (std::strcmp(arg1, "npcsolid") == 0)
	{
		mask = MASK_NPCSOLID;
	}
	else if (std::strcmp(arg1, "npcsolidbrushonly") == 0)
	{
		mask = MASK_NPCSOLID_BRUSHONLY;
	}
	else if (std::strcmp(arg1, "water") == 0)
	{
		mask = MASK_WATER;
	}
	else if (std::strcmp(arg1, "opaque") == 0)
	{
		mask = MASK_OPAQUE;
	}
	else if (std::strcmp(arg1, "solid") == 0)
	{
		mask = MASK_SOLID;
	}
	else if (std::strcmp(arg1, "blocklos") == 0)
	{
		mask = MASK_BLOCKLOS;
	}
	else if (strcasecmp(arg1, "visible") == 0)
	{
		mask = MASK_VISIBLE;
	}
	else if (strcasecmp(arg1, "shot") == 0)
	{
		mask = MASK_SHOT;
	}
	else if (strcasecmp(arg1, "all") == 0)
	{
		mask = MASK_ALL;
	}
	else if (strcasecmp(arg1, "team1") == 0)
	{
		mask = CONTENTS_TEAM1 | MASK_PLAYERSOLID;
	}
	else if (strcasecmp(arg1, "team2") == 0)
	{
		mask = CONTENTS_TEAM2 | MASK_PLAYERSOLID;
	}
	else
	{
		META_CONPRINTF("Unknown mask option %s! \n", arg1);
		return;
	}

	int colgroup = 0;

	const char* arg2 = args[2];

	if (std::strcmp(arg2, "none") == 0)
	{
		colgroup = static_cast<int>(COLLISION_GROUP_NONE);
	}
	else if (std::strcmp(arg2, "player") == 0)
	{
		colgroup = static_cast<int>(COLLISION_GROUP_PLAYER);
	}
	else if (std::strcmp(arg2, "plmove") == 0)
	{
		colgroup = static_cast<int>(COLLISION_GROUP_PLAYER_MOVEMENT);
	}
	else if (std::strcmp(arg2, "npc") == 0)
	{
		colgroup = static_cast<int>(COLLISION_GROUP_NPC);
	}

	const char* arg3 = args[3];

	trace::CTraceFilterSimple simplefilter(host.GetEntity(), colgroup);
	CTraceFilterTransientAreas navfilter(host.GetEntity(), colgroup);
	CTraceFilterWalkableEntities navwalkablefilter(host.GetEntity(), colgroup, WALK_THRU_EVERYTHING);
	CTraceFilterWorldAndPropsOnly wponlyfilter;
	CTraceFilterHitAll hitallfilter;

	if (strcasecmp(arg3, "simple") == 0)
	{
		filter = &simplefilter;
	}
	else if (strcasecmp(arg3, "navtransient") == 0)
	{
		filter = &navfilter;
	}
	else if (strcasecmp(arg3, "navwalkable") == 0)
	{
		filter = &navwalkablefilter;
	}
	else if (strcasecmp(arg3, "worldproponly") == 0)
	{
		filter = &wponlyfilter;
	}
	else if (strcasecmp(arg3, "hitall") == 0)
	{
		filter = &hitallfilter;
	}
	else
	{
		META_CONPRINTF("Unknown filter option %s! \n", arg1);
		return;
	}

	trace::line(start, end, mask, filter, tr);
	NDebugOverlay::Line(start, tr.endpos, 0, 127, 0, true, 20.0f);

	if (!tr.DidHit())
	{
		META_CONPRINTF("TRACE DID NOT HIT ANYTHING! \n");
		return;
	}

	META_CONPRINTF("HIT! \n Fraction: %g Pos: %s \n", tr.fraction, UtilHelpers::textformat::FormatVector(tr.endpos));
	NDebugOverlay::Sphere(tr.endpos, 8.0f, 255, 0, 0, true, 20.0f);
	META_CONPRINTF("Entity: %s\n", UtilHelpers::textformat::FormatEntity(tr.m_pEnt));
	META_CONPRINTF("Normal: %s \n", UtilHelpers::textformat::FormatVector(tr.plane.normal));
	META_CONPRINTF("Surface Name: %s \n", tr.surface.name != nullptr ? tr.surface.name : "NULL Surface Name!");
	META_CONPRINTF("Surface flags: %i Surface props: %i \n", static_cast<int>(tr.surface.flags), static_cast<int>(tr.surface.surfaceProps));
	META_CONPRINTF("Contents Mask: %i \n", tr.contents);

	{
		char contents[256];
		std::memset(contents, 0, sizeof(contents));

		if ((tr.contents & CONTENTS_SOLID) != 0)
		{
			ke::SafeStrcat(contents, sizeof(contents), "CONTENTS_SOLID ");
		}

		if ((tr.contents & CONTENTS_PLAYERCLIP) != 0)
		{
			ke::SafeStrcat(contents, sizeof(contents), "CONTENTS_PLAYERCLIP ");
		}

		if ((tr.contents & CONTENTS_MONSTER) != 0)
		{
			ke::SafeStrcat(contents, sizeof(contents), "CONTENTS_MONSTER ");
		}

		if ((tr.contents & CONTENTS_MONSTERCLIP) != 0)
		{
			ke::SafeStrcat(contents, sizeof(contents), "CONTENTS_MONSTERCLIP ");
		}

		if ((tr.contents & CONTENTS_LADDER) != 0)
		{
			ke::SafeStrcat(contents, sizeof(contents), "CONTENTS_LADDER ");
		}

		if ((tr.contents & CONTENTS_WATER) != 0)
		{
			ke::SafeStrcat(contents, sizeof(contents), "CONTENTS_WATER ");
		}

#if SOURCE_ENGINE > SE_DARKMESSIAH
		if ((tr.contents & CONTENTS_BLOCKLOS) != 0)
		{
			ke::SafeStrcat(contents, sizeof(contents), "CONTENTS_BLOCKLOS ");
		}
#endif // SOURCE_ENGINE > SE_DARKMESSIAH

#if SOURCE_ENGINE <= SE_DARKMESSIAH
		if ((tr.contents & CONTENTS_MIST) != 0)
		{
			ke::SafeStrcat(contents, sizeof(contents), "CONTENTS_MIST ");
		}
#endif // SOURCE_ENGINE <= SE_DARKMESSIAH


		if ((tr.contents & CONTENTS_HITBOX) != 0)
		{
			ke::SafeStrcat(contents, sizeof(contents), "CONTENTS_HITBOX ");
		}

		if ((tr.contents & CONTENTS_GRATE) != 0)
		{
			ke::SafeStrcat(contents, sizeof(contents), "CONTENTS_GRATE ");
		}

		if ((tr.contents & CONTENTS_MOVEABLE) != 0)
		{
			ke::SafeStrcat(contents, sizeof(contents), "CONTENTS_MOVEABLE ");
		}

		if ((tr.contents & CONTENTS_WINDOW) != 0)
		{
			ke::SafeStrcat(contents, sizeof(contents), "CONTENTS_WINDOW ");
		}

		if ((tr.contents & CONTENTS_OPAQUE) != 0)
		{
			ke::SafeStrcat(contents, sizeof(contents), "CONTENTS_OPAQUE ");
		}

		if ((tr.contents & CONTENTS_TRANSLUCENT) != 0)
		{
			ke::SafeStrcat(contents, sizeof(contents), "CONTENTS_TRANSLUCENT ");
		}

		if ((tr.contents & CONTENTS_TEAM1) != 0)
		{
			ke::SafeStrcat(contents, sizeof(contents), "CONTENTS_TEAM1 ");
		}

		if ((tr.contents & CONTENTS_TEAM2) != 0)
		{
			ke::SafeStrcat(contents, sizeof(contents), "CONTENTS_TEAM2 ");
		}

		META_CONPRINTF("Contents: %s \n", contents);
	}

	{
		surfacedata_t* surfacedata = physprops->GetSurfaceData(tr.surface.surfaceProps);

		if (surfacedata)
		{
			META_CONPRINT("---- PHYSICS ---- \n");
			META_CONPRINTF("Dampening: %g - Density: %g - Elasticity: %g - Friction: %g - Thickness: %g \n", 
				surfacedata->physics.dampening, surfacedata->physics.density, surfacedata->physics.elasticity, surfacedata->physics.friction, surfacedata->physics.thickness);
			
			META_CONPRINT("---- GAME ---- \n");
			META_CONPRINTF("Max Speed Factor: %g - Jump Factor: %g - Material: %u - Climbable: %i \n", 
				surfacedata->game.maxSpeedFactor, surfacedata->game.jumpFactor, surfacedata->game.material, static_cast<int>(surfacedata->game.climbable));
		}
		else
		{
			META_CONPRINT("NULL Surface Data! \n");
		}
	}
}

CON_COMMAND_F(sm_navbot_tool_force_round_restart, "Forces NavBot to run the round restart routines.", FCVAR_GAMEDLL | FCVAR_CHEAT)
{
	extmanager->GetMod()->OnRoundStart();
	CNavMesh::NotifyRoundRestart();
	META_CONPRINTF("Round restart forced! \n");
}

CON_COMMAND_F(sm_navbot_tool_entinfo, "Shows information about a specific entity.", FCVAR_GAMEDLL | FCVAR_CHEAT)
{
	DECLARE_COMMAND_ARGS;

	if (args.ArgC() < 2)
	{
		META_CONPRINT("[SM] Usage: sm_navbot_tool_entinfo <ent index> \n");
		return;
	}

	CBaseEntity* entity = gamehelpers->ReferenceToEntity(atoi(args[1]));

	META_CONPRINTF("%s \n", UtilHelpers::textformat::FormatEntity(entity));

	if (!entity)
	{
		return;
	}

	ServerClass* svclass = gamehelpers->FindEntityServerClass(entity);

	if (svclass && svclass->GetName())
	{
		META_CONPRINTF("Server Class name: %s \n", svclass->GetName());
	}

	ICollideable* collider = reinterpret_cast<IServerUnknown*>(entity)->GetCollideable();

	META_CONPRINTF("Collision Origin: %s Collision Angles: %s\n", 
		UtilHelpers::textformat::FormatVector(collider->GetCollisionOrigin()), UtilHelpers::textformat::FormatAngles(collider->GetCollisionAngles()));
	META_CONPRINTF("World Space Center: %s \n", UtilHelpers::textformat::FormatVector(UtilHelpers::getWorldSpaceCenter(entity)));
}