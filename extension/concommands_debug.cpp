#include <memory>
#include <string>
#include <sstream>
#include <chrono>
#include <queue>

#include <extension.h>
#include <manager.h>
#include <mods/basemod.h>
#include <bot/basebot.h>
#include <bot/bot_shared_utils.h>
#include <bot/interfaces/behavior.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <entities/tf2/tf_entities.h>
#include <navmesh/nav.h>
#include <navmesh/nav_area.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_pathfind.h>
#include <navmesh/nav_trace.h>
#include <sdkports/debugoverlay_shared.h>
#include <sdkports/sdk_ehandle.h>
#include <sdkports/sdk_traces.h>
#include <sdkports/sdk_game_util.h>
#include <util/helpers.h>
#include <util/librandom.h>
#include <util/ehandle_edict.h>
#include <util/sdkcalls.h>
#include <util/entprops.h>
#include <sm_argbuffer.h>
#include <am-platform.h>

CON_COMMAND(sm_navbot_info, "Prints information about the extension.")
{
	Msg("--- BEGIN NavBot Info ---\n");

#if defined(KE_ARCH_X64)
	Msg("Arch: x86-64\n");
#elif defined(KE_ARCH_X86)
	Msg("Arch: x86\n");
#endif // KE_ARCH_X64

	Msg("Extension Version: %s\n", SMEXT_CONF_VERSION);
	Msg("Source Engine Branch: %i\n", g_SMAPI->GetSourceEngineBuild());
	Msg("Server Type: %s\n", engine->IsDedicatedServer() ? "Dedicated" : "Listen");
	CBaseMod* mod = extmanager->GetMod();
	Msg("Game Folder: %s\n", smutils->GetGameFolderName() != nullptr ? smutils->GetGameFolderName() : "");
	Msg("Current Mod (Extension): %s\n", mod->GetModName());
	Msg("Current Map: %s\n", STRING(gpGlobals->mapname));
	Msg("Nav Mesh:\n    Status: %s\n    Version: %i\n    Subversion: %i\n", TheNavMesh->IsLoaded() ? "Loaded" : "NOT Loaded", CNavMesh::NavMeshVersion, TheNavMesh->GetSubVersionNumber());
	Msg("Update Rate: %3.5f\n", mod->GetModSettings()->GetUpdateRate());
	Msg("Tick Rate: %3.2f\n", (1.0f / gpGlobals->interval_per_tick));
	Msg("--- END NavBot Info ---\n");
}

#ifdef EXT_DEBUG
CON_COMMAND(sm_navbot_debug_bot_look, "Debug the bot look functions.")
{
	edict_t* host = gamehelpers->EdictOfIndex(1);
	Vector target = UtilHelpers::getWorldSpaceCenter(host);

	extmanager->ForEachBot([&target](CBaseBot* bot) {
		bot->GetControlInterface()->AimAt(target, IPlayerController::LOOK_CRITICAL, 10.0f, "Debug command!");
	});
}

CON_COMMAND(sm_navbot_debug_bot_snap_look, "Debug the bot look functions.")
{
	edict_t* host = gamehelpers->EdictOfIndex(1);
	Vector target = UtilHelpers::getWorldSpaceCenter(host);

	extmanager->ForEachBot([&target](CBaseBot* bot) {
		bot->GetControlInterface()->SnapAimAt(target, IPlayerController::LOOK_CRITICAL);
	});
}

CON_COMMAND(sm_navbot_debug_bot_impulse, "All bots sends a specific impulse command.")
{
	if (args.ArgC() < 2)
	{
		rootconsole->ConsolePrint("[SM] Usage: sm_navbot_debug_bot_impulse <impulse number>");
		return;
	}

	int impulse = atoi(args[1]);

	if (impulse <= 0 || impulse > 255)
	{
		rootconsole->ConsolePrint("Invalid impulse command number %i", impulse);
		return;
	}

	extmanager->ForEachBot([&impulse](CBaseBot* bot) {
		bot->SetImpulseCommand(impulse);
	});
}

CON_COMMAND(sm_navbot_debug_bot_send_command, "All bots sends a client command.")
{
	if (args.ArgC() < 2)
	{
		rootconsole->ConsolePrint("[SM] Usage: sm_navbot_debug_bot_send_command <command>");
		return;
	}

	const char* command = args[1];

	extmanager->ForEachBot([&command](CBaseBot* bot) {
		bot->DelayedFakeClientCommand(command);
	});
}

CON_COMMAND(sm_navbot_debug_bot_send_button, "All bots sends a client command.")
{
	if (args.ArgC() < 3)
	{
		rootconsole->ConsolePrint("[SM] Usage: sm_navbot_debug_bot_send_button <button> <time>");
		rootconsole->ConsolePrint("  Buttons: jump crouch use attack1");
		return;
	}

	const char* button = args[1];
	float time = atof(args[2]);

	time = std::clamp(time, 1.0f, 10.0f);

	extmanager->ForEachBot([&button, &time](CBaseBot* bot) {
		
		if (strcasecmp(button, "jump") == 0)
		{
			bot->GetControlInterface()->PressJumpButton(time);
		}
		else if (strcasecmp(button, "crouch") == 0)
		{
			bot->GetControlInterface()->PressCrouchButton(time);
		}
		else if (strcasecmp(button, "use") == 0)
		{
			bot->GetControlInterface()->PressUseButton(time);
		}
		else if (strcasecmp(button, "attack1") == 0)
		{
			bot->GetControlInterface()->PressAttackButton(time);
		}
	});
}

CON_COMMAND(sm_navbot_debug_bot_dump_current_path, "Dumps the current bot path to the console.")
{
	CBaseBot* bot = nullptr;

	// start at 2 since 1 is the listen server host
	for (int i = 2; i <= gpGlobals->maxClients; i++)
	{
		bot = extmanager->GetBotByIndex(i);

		if (bot)
		{
			break; // stop at the first bot found
		}
	}

	if (!bot)
	{
		return;
	}

	CMeshNavigator* nav = bot->GetActiveNavigator();

	if (!nav)
	{
		Msg("Bot doesn't have an active navigator!\n");
		return;
	}

	auto goal = nav->GetGoalSegment();

	if (!goal)
	{
		Warning("NULL goal segment!\n");
		return;
	}

	Msg("Path Dump for %s\n", bot->GetClientName());

	int counter = 0;
	float range = (bot->GetAbsOrigin() - goal->goal).AsVector2D().Length();
	Msg("Range to Goal: %3.2f \n", range);
	Msg("[%i] Current Goal: <%3.2f, %3.2f, %3.2f> type #%i\n", counter, goal->goal.x, goal->goal.y, goal->goal.z, static_cast<int>(goal->type));

	const CBasePathSegment* next = goal;

	for (;;)
	{
		next = nav->GetNextSegment(next);

		if (!next)
		{
			return;
		}

		counter++;
		Msg("[%i] Path Segment: <%3.2f, %3.2f, %3.2f> type #%i\n", counter, next->goal.x, next->goal.y, next->goal.z, static_cast<int>(next->type));
	}
}

CON_COMMAND(sm_navbot_debug_vectors, "[LISTEN SERVER] Debug player vectors")
{
	auto edict = gamehelpers->EdictOfIndex(1);

	if (edict == nullptr)
	{
		return;
	}

	// CBaseExtPlayer can be used for both players and bots
	CBaseExtPlayer player(edict);

	Vector mins(-4.0f, -4.0f, -4.0f);
	Vector maxs(4.0f, 4.0f, 4.0f);

	auto origin = player.GetAbsOrigin();
	auto angles = player.GetAbsAngles();
	auto eyepos = player.GetEyeOrigin();
	auto eyeangles = player.GetEyeAngles();

	rootconsole->ConsolePrint("Origin: <%3.2f> <%3.2f> <%3.2f>", origin.x, origin.y, origin.z);
	rootconsole->ConsolePrint("Angles: <%3.2f> <%3.2f> <%3.2f>", angles.x, angles.y, angles.z);
	rootconsole->ConsolePrint("Eye Position: <%3.2f> <%3.2f> <%3.2f>", eyepos.x, eyepos.y, eyepos.z);
	rootconsole->ConsolePrint("Eye Angles: <%3.2f> <%3.2f> <%3.2f>", eyeangles.x, eyeangles.y, eyeangles.z);

	debugoverlay->AddBoxOverlay(origin, mins, maxs, angles, 0, 255, 0, 200, 15.0f);
	debugoverlay->AddBoxOverlay(eyepos, mins, maxs, eyeangles, 0, 0, 255, 200, 15.0f);

	Vector forward;

	AngleVectors(eyeangles, &forward);

	forward = forward * 300.0f;

	debugoverlay->AddLineOverlay(eyepos, eyepos + forward, 255, 0, 0, true, 15.0f);
}

CON_COMMAND_F(sm_navbot_debug_worldcenter, "Debugs the extension CBaseEntity::WorldSpaceCenter implementation.", FCVAR_GAMEDLL)
{
	if (args.ArgC() < 2)
	{
		Msg("Usage: sm_navbot_debug_worldcenter <classname> \n");
		return;
	}

	auto classname = args[1];
	int index = UtilHelpers::FindEntityByClassname(-1, classname);
	CBaseEntity* entity = nullptr;
	edict_t* edict = nullptr;

	if (!UtilHelpers::IndexToAThings(index, &entity, &edict))
	{
		Warning("Failed to obtain an entity pointer! \n");
		return;
	}

	entities::HBaseEntity baseent(entity);
	Vector method1 = UtilHelpers::getWorldSpaceCenter(edict);
	Vector method2 = baseent.WorldSpaceCenter();

	Msg("Method 1 (OLD): %3.4f %3.4f %3.4f \n", method1.x, method1.y, method1.z);
	Msg("Method 2 (NEW): %3.4f %3.4f %3.4f \n", method2.x, method2.y, method2.z);
}

CON_COMMAND_F(sm_debug_cbasehandles, "Debug CBaseHandle", FCVAR_CHEAT)
{
	CBaseEntity* baseentity = nullptr;
	CBaseHandle handle;
	handle.Set(nullptr);

	if (UtilHelpers::IndexToAThings(1, &baseentity, nullptr))
	{
		IServerEntity* serverent = reinterpret_cast<IServerEntity*>(baseentity);
		handle.Set(serverent);
		
		Msg("Handle Entity: %i(%i) -- %p \n", handle.GetEntryIndex(), handle.GetSerialNumber(), serverent);

		auto out = handle.Get();
		IServerEntity* ent2 = reinterpret_cast<IServerEntity*>(out);
		CBaseEntity* b2 = reinterpret_cast<CBaseEntity*>(out);
		auto edict = ent2->GetNetworkable()->GetEdict();
		Msg("Base Entity: %i \n", gamehelpers->EntityToBCompatRef(b2));
		Msg("Stored Entity: %p, %i \n", edict, gamehelpers->IndexOfEdict(edict));
		edict_t* ed1 = UtilHelpers::GetEdictFromCBaseHandle(handle);
		CBaseEntity* b3 = UtilHelpers::GetBaseEntityFromCBaseHandle(handle);
		Msg("Edict: %i \n", gamehelpers->IndexOfEdict(ed1));
		Msg("Entity: %i \n", gamehelpers->EntityToBCompatRef(b3));
		return;
	}

	Msg("Failed to get CBaseEntity! \n");
}

CON_COMMAND_F(sm_navbot_debug_new_handles, "Tests new entity handles", FCVAR_CHEAT)
{
	edict_t* host = gamehelpers->EdictOfIndex(1);

	Msg("In: %p\n", host);

	CHandleEdict handle;
	handle.Set(host);

	Msg("Entry: %i Serial Number: %i\n", handle.GetEntryIndex(), handle.GetSerialNumber());

	edict_t* out = handle.Get();
	Msg("Out: %p\n", out);
	CBaseEntity* be = handle.ToBaseEntity();
	Msg("Base Entity: %p\n", be);
}

#if SOURCE_ENGINE == SE_TF2

CON_COMMAND_F(sm_navbot_tf_debug_test_buy_upgrade, "Testing sending KeyValue commands", FCVAR_CHEAT)
{
	edict_t* client = gamehelpers->EdictOfIndex(1); // gets the listen server host
	KeyValues* kvcmd1 = new KeyValues("MvM_UpgradesBegin");
	UtilHelpers::FakeClientCommandKeyValues(client, kvcmd1);

	KeyValues* kvcmd2 = new KeyValues("MVM_Upgrade");
	KeyValues* kUpg = kvcmd2->FindKey("Upgrade", true);
	kUpg->SetInt("itemslot", -1);
	kUpg->SetInt("Upgrade", 59); // move speed bonus
	kUpg->SetInt("count", 1);
	gameclients->ClientCommandKeyValues(client, kvcmd2);

	KeyValues* kvcmd3 = new KeyValues("MvM_UpgradesDone");
	kvcmd3->SetInt("num_upgrades", 1);
	gameclients->ClientCommandKeyValues(client, kvcmd3);

	Msg("Upgrade command sent!\n");
}

#endif // SOURCE_ENGINE == SE_TF2

#if SOURCE_ENGINE == SE_DODS && defined(WIN32)

CON_COMMAND_F(sm_navbot_dod_debug_vcall, "Testing Virtual function calling.", FCVAR_CHEAT)
{
	static SourceMod::ICallWrapper* pCall = nullptr;

	if (!pCall)
	{
		constexpr int OFFSET = 149; // WorldSpaceCenter

		SourceMod::PassInfo ret;
		ret.flags = PASSFLAG_BYVAL;
		ret.size = sizeof(void*);
		ret.type = SourceMod::PassType::PassType_Basic;

		pCall = g_pBinTools->CreateVCall(OFFSET, 0, 0, &ret, nullptr, 0);
	}

	CBaseEntity* host = gamehelpers->ReferenceToEntity(1);
	ArgBuffer<void*> vstk(host);

	Vector result;
	Vector* retval = nullptr;
	pCall->Execute(vstk, &retval);

	result = *retval;
	Msg("CBaseEntity::WorldSpaceCenter() <%3.2f, %3.2f, %3.2f>\n", result.x, result.y, result.z);
}

#endif // SOURCE_ENGINE == SE_DODS

CON_COMMAND_F(sm_navbot_debug_new_traces, "Debug new trace functions.", FCVAR_CHEAT)
{
	edict_t* host = gamehelpers->EdictOfIndex(1);
	entities::HBaseEntity be(host);
	CBaseExtPlayer player(host);
	
	Vector start = player.GetEyeOrigin();
	Vector forward;
	player.EyeVectors(&forward);
	forward.NormalizeInPlace();

	Vector end = start + (forward * 2048.0f);

	trace_t result;
	trace::line(start, end, MASK_SOLID, player.GetEntity(), COLLISION_GROUP_NONE, result);
	NDebugOverlay::Line(start, result.endpos, 255, 0, 0, true, 20.0f);
	Msg("Trace Result: fraction %3.6f \n", result.fraction);
}

CON_COMMAND_F(sm_navbot_debug_sdkcalls, "Debug new SDKCalls functions.", FCVAR_CHEAT)
{
	edict_t* ent = gamehelpers->EdictOfIndex(1);
	CBaseExtPlayer player(ent);
	CBaseEntity* baseent = gamehelpers->ReferenceToEntity(1);

	for (int i = 0; i <= 5; i++)
	{
		CBaseEntity* pWeapon = player.GetWeaponOfSlot(i);

		Msg("[SLOT %i]: %p\n", i, pWeapon);
	}

	CBaseEntity* newWeapon = player.GetWeaponOfSlot(2); // generally melee

	if (newWeapon != nullptr)
	{
		player.SelectWeapon(newWeapon);
	}
}

CON_COMMAND_F(sm_nav_debug_area_collector, "Debugs NavMeshCollector", FCVAR_CHEAT)
{
	CBaseExtPlayer player(gamehelpers->EdictOfIndex(1));
	player.UpdateLastKnownNavArea(true);
	
	CNavArea* start = player.GetLastKnownNavArea();

	if (start == nullptr)
	{
		Warning("Failed to get starting area!\n");
		return;
	}

	float limit = 512.0f;

	if (args.ArgC() >= 2)
	{
		limit = atof(args[2]);
	}

	INavAreaCollector<CNavArea> collector(start, limit);

	collector.Execute();

	if (collector.IsCollectedAreasEmpty())
	{
		Warning("No areas were collected!\n");
		return;
	}

	auto& collected = collector.GetCollectedAreas();

	Msg("Collected %zu areas, travel limit: %3.2f\n", collected.size(), limit);

	Vector startPos = player.GetEyeOrigin();

	for (auto area : collected)
	{
		NDebugOverlay::Line(startPos, area->GetCenter(), 0, 128, 0, true, 20.0f);
	}
}

CON_COMMAND_F(sm_snap_my_angles, "sNAP", FCVAR_CHEAT)
{
	CBaseExtPlayer player(gamehelpers->EdictOfIndex(1));
	
	QAngle angles(0.0f, 0.0f, 0.0f);

	angles.y = randomgen->GetRandomReal<vec_t>(0.0f, 359.0f);

	player.SnapEyeAngles(angles);
}

CON_COMMAND_F(sm_snap_to_origin, "Snaps the player view angles to an origin", FCVAR_CHEAT)
{
	CBaseExtPlayer player(UtilHelpers::GetListenServerHost());
	QAngle angles(0.0f, 0.0f, 0.0f);
	Vector origin = player.GetEyeOrigin();
	Vector to = (vec3_origin - origin);
	to.NormalizeInPlace();
	VectorAngles(to, angles);
	player.SnapEyeAngles(angles);
	NDebugOverlay::Line(origin, vec3_origin, 0, 128, 0, true, 15.0f);
}

CON_COMMAND_F(sm_navbot_debug_entlist, "Debugs the entity list.", FCVAR_CHEAT)
{
	UtilHelpers::ForEveryEntity([](int index, edict_t* edict, CBaseEntity* entity) {
		Msg("Entity #%i: %s (%p <%p>) [%i] \n", index, gamehelpers->GetEntityClassname(entity) ? gamehelpers->GetEntityClassname(entity) : "NULL CLASSNAME", entity, edict, reinterpret_cast<IServerUnknown*>(entity)->GetRefEHandle().GetEntryIndex());
	});
}

#if SOURCE_ENGINE == SE_TF2 || SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_BMS

CON_COMMAND_F(sm_get_entity_size, "Returns the entity size", FCVAR_CHEAT)
{
	if (args.ArgC() < 2)
	{
		Msg("Usage: sm_get_entity_size <entity classname> \n");
		return;
	}

	const char* classname = args[1];

	IEntityFactory* factory = servertools->GetEntityFactoryDictionary()->FindFactory(classname);

	if (factory == nullptr)
	{
		Warning("ERROR: Could not find Entity Factor for classname '%s' \n", classname);
		return;
	}

	size_t size = factory->GetEntitySize();

	Msg("Entity (%s) class size is: %zu \n", classname, size);
}

#endif // SOURCE_ENGINE == SE_TF2 || SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_BMS

CON_COMMAND_F(sm_navbot_debug_surf_props, "Shows surface properties.", FCVAR_CHEAT)
{
	CBaseExtPlayer host{ gamehelpers->EdictOfIndex(1) };
	trace_t tr;
	CTraceFilterWorldAndPropsOnly filter;

	Vector start = host.GetEyeOrigin();
	QAngle eyeAngles = host.GetEyeAngles();
	Vector forward;
	AngleVectors(eyeAngles, &forward);
	forward.NormalizeInPlace();
	Vector end = start + (forward * 4096.0f);

	trace::line(start, end, MASK_SOLID, &filter, tr);

	if (tr.fraction < 1.0f)
	{
		NDebugOverlay::Line(start, tr.endpos, 0, 255, 0, true, 20.0f);
		NDebugOverlay::Cross3D(tr.endpos, 12.0f, 0, 255, 0, true, 20.0f);

		const char* name = tr.surface.name;

		if (name != nullptr)
		{
			Msg("Surface name: %s\n", name);
		}

		{
			Vector normal = tr.plane.normal;

			Msg("Surface Normal: %3.4f, %3.4f, %3.4f\n", normal.x, normal.y, normal.z);
		}

		if ((tr.contents & CONTENTS_LADDER) != 0)
		{
			Msg("Hit Ladder!\n");
		}

		if ((tr.contents & CONTENTS_PLAYERCLIP) != 0)
		{
			Msg("Hit Player Clip!\n");
		}

		if ((tr.contents & CONTENTS_WATER) != 0)
		{
			Msg("Hit Water!\n");
		}

		if ((tr.contents & CONTENTS_SOLID) != 0)
		{
			Msg("Hit Solid!\n");
		}

		if ((tr.contents & CONTENTS_BLOCKLOS) != 0)
		{
			Msg("Hit Block LOS!\n");
		}

		surfacedata_t* surfacedata = physprops->GetSurfaceData(tr.surface.surfaceProps);

		if (surfacedata != nullptr)
		{
			Msg("---- PHYSICS ----\n");
			Msg("Friction: %3.4f\nElasticity: %3.4f\nDensity: %3.4f\nThickness: %3.4f\nDampening: %3.4f\n",
				surfacedata->physics.friction,
				surfacedata->physics.elasticity,
				surfacedata->physics.density,
				surfacedata->physics.thickness,
				surfacedata->physics.dampening);

			Msg("---- GAME ----\n");
			Msg("Max Speed Factor: %3.4f\nJump Factor: %3.2f\nMaterial: <%i>\nClimbable: %i\n", 
				surfacedata->game.maxSpeedFactor,
				surfacedata->game.jumpFactor,
				surfacedata->game.material,
				static_cast<int>(surfacedata->game.climbable));
		}
		else
		{
			Msg("Got NULL surfacedata_t!\n");
		}
	}
	else
	{
		Msg("Trace did not hit anything!\n");
	}

}

// SDKs that may have func_useableladder entities
#if SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_SDK2013

CON_COMMAND_F(sm_navbot_debug_useable_ladder, "Useable ladder debug command", FCVAR_CHEAT)
{
	CBaseExtPlayer host{ UtilHelpers::GetListenServerHost() };
	Vector center = host.GetAbsOrigin();

	CBaseEntity* ladder = nullptr;

	UtilHelpers::ForEachEntityInSphere(center, 512, [&ladder](int index, edict_t* edict, CBaseEntity* entity) {
		if (edict != nullptr && entity != nullptr)
		{
			if (strcmp(gamehelpers->GetEntityClassname(entity), "func_useableladder") == 0)
			{
				ladder = entity;
				return false;
			}
		}

		return true;
	});

	if (ladder == nullptr)
	{
		Msg("No ladder found!\n");
		return;
	}

	entities::HBaseEntity ent{ ladder };

	Vector origin = reinterpret_cast<IServerEntity*>(ladder)->GetCollideable()->GetCollisionOrigin();
	Vector topPosition, bottomPosition;
	Vector* top = entprops->GetPointerToEntData<Vector>(ladder, Prop_Send, "m_vecPlayerMountPositionTop");
	Vector* bottom = entprops->GetPointerToEntData<Vector>(ladder, Prop_Send, "m_vecPlayerMountPositionBottom");
	Vector* dir = entprops->GetPointerToEntData<Vector>(ladder, Prop_Send, "m_vecLadderDir");
	bool* fakeladder = entprops->GetPointerToEntData<bool>(ladder, Prop_Send, "m_bFakeLadder");

	ent.ComputeAbsPosition((*top + origin), &topPosition);
	ent.ComputeAbsPosition((*bottom + origin), &bottomPosition);

	Msg("Ladder Origin: %3.2f, %3.2f, %3.2f\n", origin.x, origin.y, origin.z);
	Msg("Ladder Top: %3.2f, %3.2f, %3.2f\n", topPosition.x, topPosition.y, topPosition.z);
	Msg("Ladder Bottom: %3.2f, %3.2f, %3.2f\n", bottomPosition.x, bottomPosition.y, bottomPosition.z);
	Msg("Ladder Dir: %3.2f, %3.2f, %3.2f\n", dir->x, dir->y, dir->z);
	Msg("Ladder is %s <%i>\n", *fakeladder ? "FAKE" : "REAL", static_cast<int>(*fakeladder));

	NDebugOverlay::VertArrow(bottomPosition, topPosition, 10.0f, 0, 255, 0, 255, true, 20.0f);
}

#endif // SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_SDK2013

CON_COMMAND_F(sm_navbot_debug_find_ledge, "finds the ledge", FCVAR_CHEAT)
{
	CBaseExtPlayer host{ UtilHelpers::GetListenServerHost() };
	Vector start = host.GetAbsOrigin();
	Vector mins = host.GetMins();
	Vector maxs = host.GetMaxs();
	Vector forward;
	QAngle eyeAngles = host.GetEyeAngles();
	AngleVectors(eyeAngles, &forward);
	forward.z = 0.0f;
	forward.NormalizeInPlace();

	CTraceFilterWorldAndPropsOnly filter;
	bool lasthit = false;
	Vector hitpos;
	int it = 0;
	constexpr auto stepSize = 16.0f;

	do
	{
		it++;
		trace_t tr;
		Vector pos1 = start + (forward * (stepSize * static_cast<float>(it)));
		Vector pos2 = pos1;
		pos2.z -= navgenparams->step_height;
		trace::hull(pos1, pos2, mins, maxs, MASK_PLAYERSOLID, &filter, tr);

		if (trace::pointoutisdeworld(pos1))
		{
			Msg("Outside world!\n");
			break;
		}

		if (tr.DidHit())
		{
			NDebugOverlay::SweptBox(pos1, pos2, mins, maxs, vec3_angle, 255, 0, 0, 255, 20.0f);

			if (!lasthit)
			{
				lasthit = true;
			}

			hitpos = tr.endpos;
		}
		else
		{
			NDebugOverlay::SweptBox(pos1, pos2, mins, maxs, vec3_angle, 0, 153, 0, 255, 20.0f);

			if (lasthit)
			{
				NDebugOverlay::Cross3D(hitpos, 16.0f, 0, 255, 255, true, 20.0f);
				NDebugOverlay::Text(hitpos, "LEDGE", false, 20.0f);
				break;
			}
		}

	} while (it <= 20);
}

CON_COMMAND(sm_debug_line_intercept, "intercept")
{
	static bool setup = false;
	static Vector origin = vec3_origin;
	static Vector mins = vec3_origin;
	static Vector maxs = vec3_origin;

	CBaseExtPlayer host{ UtilHelpers::GetListenServerHost() };

	if (setup == false)
	{
		QAngle eyeAngles = host.GetEyeAngles();
		Vector forward;
		AngleVectors(eyeAngles, &forward);
		forward.NormalizeInPlace();

		origin = host.GetAbsOrigin() + (forward * 256.0f);
		mins.Init(-64.0f, -64.0f, 0.0f);
		maxs.Init(64.0f, 64.0f, 128.0f);

		setup = true;
		NDebugOverlay::Box(origin, mins, maxs, 0, 128, 0, 128, 10.0f);
		Msg("setup == true\n");
		return;
	}

	Vector start = host.GetEyeOrigin();
	QAngle eyeAngles = host.GetEyeAngles();
	Vector dir;
	AngleVectors(eyeAngles, &dir);
	dir.NormalizeInPlace();
	Vector end = start + (dir * 1024.0f);

	if (UtilHelpers::LineIntersectsAABB(start, end, origin, mins, maxs))
	{
		Msg("HIT!\n");
		NDebugOverlay::Box(origin, mins, maxs, 0, 128, 0, 128, 10.0f);
	}
	else
	{
		Msg("MISS!\n");
		NDebugOverlay::Box(origin, mins, maxs, 255, 0, 0, 128, 10.0f);
	}

	NDebugOverlay::Line(start, end, 0, 200, 200, true, 10.0f);
}

CON_COMMAND(sm_navbot_debug_gamerules_ptr, "Tests if the extension is able to get a valid game rules pointer.")
{
	void* pGameRules = g_pSDKTools->GetGameRules();
	META_CONPRINTF("pGameRules = %p\n", pGameRules);

	if (pGameRules != nullptr)
	{
		META_CONPRINT("Testing pointer with SDKCall\n");
		bool result = sdkcalls->CGameRules_ShouldCollide(reinterpret_cast<CGameRules*>(pGameRules), COLLISION_GROUP_PLAYER, COLLISION_GROUP_NPC);

		META_CONPRINTF("SDK Call Result: %s\n", result ? "TRUE" : "FALSE");
	}
}

CON_COMMAND(sm_navbot_debug_cvar_value, "Reports the value of ConVars.")
{
	if (args.ArgC() < 2)
	{
		META_CONPRINT("[SM] Usage: sm_navbot_debug_cvar_value <convar name>\n");
		return;
	}

	const char* name = args[1];

	ConVarRef cvref(name, false);

	if (!cvref.IsValid())
	{
		META_CONPRINTF("Invalid convar named \"%s\"!\n", name);
		return;
	}

	int iValue = cvref.GetInt();
	bool bValue = cvref.GetBool();
	float flValue = cvref.GetFloat();
	const char* szValue = cvref.GetString();

	META_CONPRINTF("Values for ConVar \"%s\": \n", name);
	META_CONPRINTF("    Integer: %i\n    Float: %f\n    Bool: %s\n    String: %s\n", iValue, flValue, bValue ? "TRUE" : "FALSE", szValue);

	ConVar* cvar = g_pCVar->FindVar(name);

	if (cvar != nullptr)
	{
		META_CONPRINT("Got valid ConVar pointer from ICVar interface!\n");

		iValue = cvar->GetInt();
		bValue = cvar->GetBool();
		flValue = cvar->GetFloat();
		szValue = cvar->GetString();

		META_CONPRINTF("    Integer: %i\n    Float: %f\n    Bool: %s\n    String: %s\n", iValue, flValue, bValue ? "TRUE" : "FALSE", szValue);
	}
}

CON_COMMAND(sm_navbot_debug_roundstate, "Debug the game rules round state.")
{
	RoundState state = entprops->GameRules_GetRoundState();
	rootconsole->ConsolePrint("Got round state %i", static_cast<int>(state));
}

CON_COMMAND(sm_navbot_debug_entsinbox, "Debug EntitiesInBox implementation.")
{
	CBaseExtPlayer host(UtilHelpers::GetListenServerHost());

	QAngle eyeAngles = host.GetEyeAngles();
	Vector position = host.GetAbsOrigin();
	Vector mins(-512.0f, -512.0f, -256.0f);
	Vector maxs(512.0f, 512.0f, 128.0f);

	mins = mins + position;
	maxs = maxs + position;

	UtilHelpers::CEntityEnumerator enumerator;
	UtilHelpers::EntitiesInBox(mins, maxs, enumerator);

	int i = 0;

	Msg("Entities In Box: \n");
	NDebugOverlay::Box(vec3_origin, mins, maxs, 0, 128, 0, 96, 30.0f);

	enumerator.ForEach([&i](CBaseEntity* entity) {
		Msg("#%i ", reinterpret_cast<IHandleEntity*>(entity)->GetRefEHandle().GetEntryIndex());
		i++;

		NDebugOverlay::EntityBounds(entity, 0, 50, 235, 180, 30.0f);

		if (i > 9)
		{
			Msg("\n");
			i = 0;
		}

		return true;
	});
}

CON_COMMAND(sm_navbot_debug_find_cover, "Debugs the find cover utility")
{
	if (TheNavMesh->GetNavAreaCount() == 0)
	{
		Warning("Find Cover needs a nav mesh to search! \n");
		return;
	}

	static bool spot{ false };
	static Vector vecSpot{ 0.0f, 0.0f, 0.0f };

	edict_t* host = gamehelpers->EdictOfIndex(1);

	if (!spot)
	{
		vecSpot = UtilHelpers::getEntityOrigin(host);
		Msg("Cover From position set to %3.2f %3.2f %3.2f \n", vecSpot.x, vecSpot.y, vecSpot.z);
		spot = true;
		return;
	}

	spot = false;
	const Vector& origin = UtilHelpers::getEntityOrigin(host);
	botsharedutils::FindCoverCollector search{ vecSpot, 512.0f, true, true, 2048.0f, origin };

	search.Execute();

	if (search.IsCollectedAreasEmpty())
	{
		Warning("Failed to find cover! \n");
		return;
	}

	CNavArea* coverArea = search.GetRandomCollectedArea();
	coverArea->DrawFilled(255, 0, 255, 200, 20.0f);

	NDebugOverlay::Line(origin, coverArea->GetCenter(), 0, 255, 0, true, 20.0f);
	Msg("Found cover area! %i\n", coverArea->GetID());
}

CON_COMMAND_F(sm_debug_trace_line, "Trace line debug", FCVAR_GAMEDLL)
{
	if (args.ArgC() < 3)
	{
		META_CONPRINT("[SM] Usage: sm_debug_trace_line <mask option> <filter option>\n");
		META_CONPRINT("  <mask option> : playersolid npcsolid visible shot all\n");
		META_CONPRINT("  <filter option> : simple navtransient\n");
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

	if (strcasecmp(arg1, "playersolid") == 0)
	{
		mask = MASK_PLAYERSOLID;
	}
	else if (strcasecmp(arg1, "npcsolid") == 0)
	{
		mask = MASK_NPCSOLID;
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
	else
	{
		META_CONPRINTF("Unknown mask option %s! \n", arg1);
		return;
	}

	const char* arg2 = args[2];

	trace::CTraceFilterSimple simplefilter(host.GetEntity(), COLLISION_GROUP_NONE);
	CTraceFilterTransientAreas navfilter(host.GetEntity(), COLLISION_GROUP_NONE);

	if (strcasecmp(arg2, "simple") == 0)
	{
		filter = &simplefilter;
	}
	else if (strcasecmp(arg2, "navtransient") == 0)
	{
		filter = &navfilter;
	}
	else
	{
		META_CONPRINTF("Unknown filter option %s! \n", arg1);
		return;
	}

	trace::line(start, end, mask, filter, tr);

	if (tr.DidHit())
	{
		META_CONPRINTF("HIT! \n    fraction: %3.4f \n", tr.fraction);

		NDebugOverlay::Sphere(tr.endpos, 8.0f, 255, 0, 0, true, 20.0f);

		CBaseEntity* pEntity = tr.m_pEnt;

		if (pEntity)
		{
			const char* classname = gamehelpers->GetEntityClassname(pEntity);
			int index = reinterpret_cast<IHandleEntity*>(pEntity)->GetRefEHandle().GetEntryIndex();

			META_CONPRINTF("    #%i<%s>", index, classname);

			datamap_t* map = gamehelpers->GetDataMap(pEntity);

			if (map)
			{
				META_CONPRINTF(" [%s] \n", map->dataClassName);
			}
			else
			{
				META_CONPRINTF(" \n");
			}
		}
	}
	else
	{
		META_CONPRINTF("NO ENTITY HIT \n");
	}
}

CON_COMMAND(sm_navbot_debug_vis, "Visibility debug")
{
	if (args.ArgC() < 2)
	{
		Msg("[SM] Usage: sm_navbot_debug_vis <ent index>\n");
		return;
	}

	CBaseExtPlayer host(gamehelpers->EdictOfIndex(1));
	CBaseEntity* pOther = gamehelpers->ReferenceToEntity(atoi(args[1]));
	entities::HBaseEntity ent(pOther);

	if (!pOther)
	{
		Msg("%s is not a valid entity index! \n", args[1]);
		return;
	}

	auto tstart = std::chrono::high_resolution_clock::now();
	std::array<byte, MAX_MAP_CLUSTERS / 8> pvs;
	std::fill(std::begin(pvs), std::end(pvs), 0);
	Vector center = UtilHelpers::getWorldSpaceCenter(pOther);
	Vector origin = host.GetEyeOrigin();
	engine->ResetPVS(pvs.data(), static_cast<int>(pvs.size()));
	engine->AddOriginToPVS(origin);
	int clusterIndex = engine->GetClusterForOrigin(center);
	bool inPVS = engine->CheckOriginInPVS(center, pvs.data(), static_cast<int>(pvs.size()));
	auto tend = std::chrono::high_resolution_clock::now();

	const std::chrono::duration<double, std::milli> millis = (tend - tstart);

	META_CONPRINTF("PVS took %f ms.\n", millis.count());

	
	tstart = std::chrono::high_resolution_clock::now();
	trace_t tr;
	trace::CTraceFilterNoNPCsOrPlayers filter(pOther, COLLISION_GROUP_NONE);

	trace::line(origin, ent.EyePosition(), MASK_VISIBLE, &filter, tr);

	if (tr.DidHit())
	{
		trace::line(origin, ent.WorldSpaceCenter(), MASK_VISIBLE, &filter, tr);

		if (tr.DidHit())
		{
			trace::line(origin, ent.GetAbsOrigin(), MASK_VISIBLE, &filter, tr);
		}
	}

	bool vis = tr.fraction >= 1.0f && !tr.startsolid;

	tend = std::chrono::high_resolution_clock::now();

	const std::chrono::duration<double, std::milli> millis2 = (tend - tstart);

	META_CONPRINTF("Trace took %f ms.\n", millis2.count());

	Msg("inPVS = %s \n", inPVS ? "TRUE" : "FALSE");
	Msg("VIS = %s \n", vis ? "TRUE" : "FALSE");
}

CON_COMMAND(sm_navbot_debug_dump_player_models, "Dump the models of every player.")
{
	UtilHelpers::ForEachPlayer([](int client, edict_t* entity, SourceMod::IGamePlayer* player) {
		if (player->IsInGame())
		{
			const char* model = STRING(entity->GetIServerEntity()->GetModelName());

			if (model)
			{
				META_CONPRINTF("Player %s #%i model %s \n", player->GetName(), client, model);
			}
		}
	});
}

CON_COMMAND(sm_navbot_debug_touching, "List entities you're touching.")
{
	edict_t* player = gamehelpers->EdictOfIndex(1);

	Vector mins = player->GetCollideable()->OBBMins() + player->GetCollideable()->GetCollisionOrigin();
	Vector maxs = player->GetCollideable()->OBBMaxs() + player->GetCollideable()->GetCollisionOrigin();

	UtilHelpers::CEntityEnumerator enumerator;
	UtilHelpers::EntitiesInBox(mins, maxs, enumerator);

	enumerator.ForEach([](CBaseEntity* entity) -> bool {

		META_CONPRINTF("Touching %s \n", gamehelpers->GetEntityClassname(entity));

		return true;
	});
}

#endif // EXT_DEBUG