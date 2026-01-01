#include NAVBOT_PCH_FILE
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
#include <sdkports/sdk_entityoutput.h>
#include <util/ehandle_edict.h>
#include <sm_argbuffer.h>
#include <am-platform.h>

#if SOURCE_ENGINE <= SE_DARKMESSIAH
#include <util/commandargs_episode1.h>
#include <sdkports/sdk_convarref_ep1.h>
#endif // SOURCE_ENGINE <= SE_DARKMESSIAH

#ifdef AUTO_GENERATED_VERSION
#include "generated_version.h"
#endif // AUTO_GENERATED_VERSION


CON_COMMAND(sm_navbot_info, "Prints information about the extension.")
{
	Msg("--- BEGIN NavBot Info ---\n");

#if defined(KE_ARCH_X64)
	Msg("Arch: x86-64\n");
#elif defined(KE_ARCH_X86)
	Msg("Arch: x86\n");
#endif // KE_ARCH_X64

	Msg("Extension Version: %s\n", extension->GetExtensionVerString());

#ifdef AUTO_GENERATED_VERSION
	Msg("Git Commit: %s\n", GIT_COMMIT_HASH);
	Msg("Git URL: %s\n", GIT_URL);
#endif // AUTO_GENERATED_VERSION

	Msg("Source Engine Branch: %i\n", g_SMAPI->GetSourceEngineBuild());
	Msg("Server Type: %s\n", engine->IsDedicatedServer() ? "Dedicated" : "Listen");
	CBaseMod* mod = extmanager->GetMod();
	Msg("Game Folder: %s\n", smutils->GetGameFolderName() != nullptr ? smutils->GetGameFolderName() : "");
	Msg("Mod Folder: %s\n", mod->GetModFolder().c_str());
	Msg("Current Mod (Extension): %s\n", mod->GetModName());
	Msg("Current Map: %s\n", STRING(gpGlobals->mapname));
	Msg("Nav Mesh:\n    Status: %s\n    Version: %i\n    Subversion: %i\n", TheNavMesh->IsLoaded() ? "Loaded" : "NOT Loaded", CNavMesh::NavMeshVersion, TheNavMesh->GetSubVersionNumber());
	Msg("Update Rate: %3.5f\n", mod->GetModSettings()->GetUpdateRate());
	Msg("Tick Rate: %3.2f\n", (1.0f / gpGlobals->interval_per_tick));
	Msg("--- END NavBot Info ---\n");
}

CON_COMMAND_F_COMPLETION(sm_navbot_debug_bot_sensor_memory, "Debugs the bot Sensor interface's entity memory.", FCVAR_CHEAT | FCVAR_GAMEDLL, CExtManager::AutoComplete_BotNames)
{
	DECLARE_COMMAND_ARGS;

	if (args.ArgC() < 2)
	{
		META_CONPRINT("[SM] Usage: sm_navbot_debug_sensor_memory <bot name> \n");
		return;
	}

	CBaseBot* bot = extmanager->FindBotByName(args[1]);

	if (!bot)
	{
		META_CONPRINTF("Error: bot of name \"%s\" not found!\n", args[1]);
		return;
	}

	META_CONPRINTF("Showing ISensor debug information for %s \n", bot->GetClientName());
	bot->GetSensorInterface()->ShowDebugInformation();
}

#ifdef EXT_DEBUG
CON_COMMAND(sm_navbot_debug_bot_look, "Debug the bot look functions.")
{
	DECLARE_COMMAND_ARGS;

	edict_t* host = gamehelpers->EdictOfIndex(1);
	Vector target = UtilHelpers::getWorldSpaceCenter(host);

	if (args.ArgC() > 2)
	{
		const char* arg1 = args[1];
		float x, y, z;

		int count = sscanf(arg1, "%f %f %f", &x, &y, &z);

		if (count == 3)
		{
			target.x = x;
			target.y = y;
			target.z = z;
		}
	}
	
	auto functor = [&target](CBaseBot* bot) {
		bot->GetControlInterface()->AimAt(target, IPlayerController::LOOK_CRITICAL, 10.0f, "Debug command!");
	};
	extmanager->ForEachBot(functor);
}

CON_COMMAND(sm_navbot_debug_bot_snap_look, "Debug the bot look functions.")
{
	edict_t* host = gamehelpers->EdictOfIndex(1);
	Vector target = UtilHelpers::getWorldSpaceCenter(host);
	auto functor = [&target](CBaseBot* bot) {
		bot->GetControlInterface()->SnapAimAt(target, IPlayerController::LOOK_CRITICAL);
	};

	extmanager->ForEachBot(functor);
}

CON_COMMAND(sm_navbot_debug_bot_hear_me, "Simulates the bots hearing you.")
{
	IServerEntity* host = reinterpret_cast<IServerEntity*>(gamehelpers->ReferenceToEntity(1));

	auto functor = [&host](CBaseBot* bot) {
		bot->OnSound(host->GetBaseEntity(), host->GetCollideable()->GetCollisionOrigin(), IEventListener::SoundType::SOUND_GENERIC, 10000.0f);
		CKnownEntity* known = bot->GetSensorInterface()->AddKnownEntity(host->GetBaseEntity());
		known->UpdateHeard();
	};

	extmanager->ForEachBot(functor);
}

CON_COMMAND(sm_navbot_debug_bot_impulse, "All bots sends a specific impulse command.")
{
	DECLARE_COMMAND_ARGS;

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

	auto func = [&impulse](CBaseBot* bot) {
		bot->SetImpulseCommand(impulse);
	};

	extmanager->ForEachBot(func);
}

CON_COMMAND(sm_navbot_debug_bot_send_command, "All bots sends a client command.")
{
	DECLARE_COMMAND_ARGS;

	if (args.ArgC() < 2)
	{
		rootconsole->ConsolePrint("[SM] Usage: sm_navbot_debug_bot_send_command <command>");
		return;
	}

	const char* command = args[1];

	auto func = [&command](CBaseBot* bot) {
		bot->DelayedFakeClientCommand(command);
	};

	extmanager->ForEachBot(func);
}

CON_COMMAND(sm_navbot_debug_bot_send_button, "All bots sends a client command.")
{
	DECLARE_COMMAND_ARGS;

	if (args.ArgC() < 3)
	{
		rootconsole->ConsolePrint("[SM] Usage: sm_navbot_debug_bot_send_button <button> <time>");
		rootconsole->ConsolePrint("  Buttons: jump crouch use attack1 attack2 attack3 reload speed run walk");
		return;
	}

	const char* button = args[1];
	float time = atof(args[2]);

	time = std::clamp(time, 0.0f, 10.0f);
	auto func = [&button, &time](CBaseBot* bot) {

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
		else if (strcasecmp(button, "attack2") == 0)
		{
			bot->GetControlInterface()->PressSecondaryAttackButton(time);
		}
		else if (strcasecmp(button, "attack3") == 0)
		{
			bot->GetControlInterface()->PressSpecialAttackButton(time);
		}
		else if (strcasecmp(button, "reload") == 0)
		{
			bot->GetControlInterface()->PressReloadButton(time);
		}
		else if (strcasecmp(button, "speed") == 0)
		{
			bot->GetControlInterface()->PressSpeedButton(time);
		}
		else if (strcasecmp(button, "run") == 0)
		{
			bot->GetControlInterface()->PressRunButton(time);
		}
		else if (strcasecmp(button, "walk") == 0)
		{
			bot->GetControlInterface()->PressWalkButton(time);
		}
	};

	extmanager->ForEachBot(func);
}

CON_COMMAND(sm_navbot_debug_bot_select_weapon, "Tests weapon selection.")
{
	DECLARE_COMMAND_ARGS;
	CBaseBot* bot = nullptr;

	if (args.ArgC() < 3)
	{
		META_CONPRINT("[SM] Usage: sm_navbot_debug_bot_select_weapon <entindex> <method>\n");
		META_CONPRINT("Valid methods: sdkcall usercmd inventory\n");
		return;
	}

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
		META_CONPRINT("Add a bot first! \n");
		return;
	}

	CBaseEntity* pWeapon = gamehelpers->ReferenceToEntity(atoi(args[1]));

	if (!pWeapon)
	{
		META_CONPRINT("NULL weapon!\n");
		return;
	}

	const CBotWeapon* weapon = bot->GetInventoryInterface()->GetWeaponOfEntity(pWeapon);

	if (!weapon)
	{
		META_CONPRINT("Weapon not registered in the bot's inventory!\n");
		return;
	}

	const char* szMethod = args[2];

	if (std::strcmp(szMethod, "sdkcall") == 0)
	{
		bot->SelectWeapon(pWeapon);
	}
	else if (std::strcmp(szMethod, "usercmd") == 0)
	{
		bot->SelectWeaponByUserCmd(weapon->GetIndex(), weapon->GetBaseCombatWeapon().GetSubTypeFromProperty());
	}
	else if (std::strcmp(szMethod, "inventory") == 0)
	{
		bot->GetInventoryInterface()->EquipWeapon(weapon);
	}
	else
	{
		META_CONPRINTF("Unknown equip method! %s\n", szMethod);
	}
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

	const BotPathSegment* next = goal;

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

CON_COMMAND(sm_navbot_debug_bot_dump_path_kv, "Dumps the current bot path as a KeyValues file")
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

	const BotPathSegment* goal = nav->GetGoalSegment();

	if (!goal)
	{
		Warning("NULL goal segment!\n");
		return;
	}

	Msg("Path Dump for %s\n", bot->GetClientName());

	KeyValues* root = new KeyValues("BotPath");

	{
		KeyValues* sub = new KeyValues("BotData");

		sub->SetString("EyeAngles", UtilHelpers::textformat::FormatAngles(bot->GetEyeAngles()));
		sub->SetString("EyePosition", UtilHelpers::textformat::FormatVector(bot->GetEyeOrigin()));
		sub->SetString("Position", UtilHelpers::textformat::FormatVector(bot->GetAbsOrigin()));

		root->AddSubKey(sub);
	}

	{
		KeyValues* sub = new KeyValues("Info");

		sub->SetFloat("TravelDistance", nav->GetTravelDistance());
		sub->SetString("Destination", UtilHelpers::textformat::FormatVector(nav->GetPathDestination()));

		root->AddSubKey(sub);
	}

	const BotPathSegment* segment = nav->GetFirstSegment();
	int n = 0;

	while (segment != nullptr)
	{
		KeyValues* kvSeg = new KeyValues("PathSegment");

		kvSeg->SetInt("Index", n);
		kvSeg->SetString("IsCurrentGoal", segment == goal ? "Yes" : "No");
		kvSeg->SetInt("Area", static_cast<int>(segment->area->GetID()));
		kvSeg->SetString("TraverseHow", NavTraverseTypeToString(segment->how));

		if (segment->ladder)
		{
			kvSeg->SetInt("Ladder", static_cast<int>(segment->ladder->GetID()));
		}
		else
		{
			kvSeg->SetString("Ladder", "NULL");
		}

		kvSeg->SetString("Position", UtilHelpers::textformat::FormatVector(segment->goal));
		kvSeg->SetString("Type", AIPath::SegmentTypeToString(segment->type));
		kvSeg->SetFloat("Length", segment->length);
		kvSeg->SetFloat("Distance", segment->distance);
		kvSeg->SetFloat("Curvature", segment->curvature);
		kvSeg->SetString("Forward", UtilHelpers::textformat::FormatVector(segment->forward));
		kvSeg->SetString("PortalCenter", UtilHelpers::textformat::FormatVector(segment->portalcenter));
		kvSeg->SetFloat("PortalHalfWidth", segment->portalhalfwidth);

		root->AddSubKey(kvSeg);
		n++;
		segment = nav->GetNextSegment(segment);
	}

	bool saved = UtilHelpers::sdkcompat::SaveKeyValuesToFile(root, "botpathdump.txt", "MOD");
	root->deleteThis();

	if (!saved)
	{
		META_CONPRINT("Failed to save keyvalues files!\n");
		return;
	}

	char path[PLATFORM_MAX_PATH];
	smutils->BuildPath(Path_Game, path, sizeof(path), "botpathdump.txt");
	META_CONPRINTF("File saved to: %s \n", path);
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
	DECLARE_COMMAND_ARGS;

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
	DECLARE_COMMAND_ARGS;

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
	auto func = [](int index, edict_t* edict, CBaseEntity* entity) {
		Msg("Entity #%i: %s (%p <%p>) [%i] \n", index, gamehelpers->GetEntityClassname(entity) ? gamehelpers->GetEntityClassname(entity) : "NULL CLASSNAME", entity, edict, reinterpret_cast<IServerUnknown*>(entity)->GetRefEHandle().GetEntryIndex());
	};
	UtilHelpers::ForEveryEntity(func);
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

#if SOURCE_ENGINE <= SE_DARKMESSIAH
		if ((tr.contents & CONTENTS_MIST) != 0)
		{
			Msg("Hit Mist!\n");
		}
#else
		if ((tr.contents & CONTENTS_BLOCKLOS) != 0)
		{
			Msg("Hit Block LOS!\n");
		}
#endif // SOURCE_ENGINE <= SE_DARKMESSIAH

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
#if SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_ORANGEBOX

CON_COMMAND_F(sm_navbot_debug_useable_ladder, "Useable ladder debug command", FCVAR_CHEAT)
{
	CBaseExtPlayer host{ UtilHelpers::GetListenServerHost() };
	Vector center = host.GetAbsOrigin();

	CBaseEntity* ladder = nullptr;
	auto func = [&ladder](int index, edict_t* edict, CBaseEntity* entity) {
		if (edict != nullptr && entity != nullptr)
		{
			if (strcmp(gamehelpers->GetEntityClassname(entity), "func_useableladder") == 0)
			{
				ladder = entity;
				return false;
			}
		}

		return true;
	};


	UtilHelpers::ForEachEntityInSphere(center, 512, func);

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

#endif // SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_ORANGEBOX

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
	DECLARE_COMMAND_ARGS;

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
	auto func = [&i](CBaseEntity* entity) {
		Msg("#%i ", reinterpret_cast<IHandleEntity*>(entity)->GetRefEHandle().GetEntryIndex());
		i++;

		NDebugOverlay::EntityBounds(entity, 0, 50, 235, 180, 30.0f);

		if (i > 9)
		{
			Msg("\n");
			i = 0;
		}

		return true;
	};

	enumerator.ForEach(func);
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
	DECLARE_COMMAND_ARGS;

	if (args.ArgC() < 4)
	{
		META_CONPRINT("[SM] Usage: sm_debug_trace_line <mask option> <collision group> <filter option>\n");
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
		else
		{
			META_CONPRINT("HIT BUT ENTITY IS NULL! \n");
		}
	}
	else
	{
		META_CONPRINTF("NO ENTITY HIT \n");
	}
}

CON_COMMAND(sm_navbot_debug_vis, "Visibility debug")
{
	DECLARE_COMMAND_ARGS;

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
	auto func = [](int client, edict_t* entity, SourceMod::IGamePlayer* player) {
		if (player->IsInGame())
		{
			const char* model = STRING(entity->GetIServerEntity()->GetModelName());

			if (model)
			{
				META_CONPRINTF("Player %s #%i model %s \n", player->GetName(), client, model);
			}
		}
	};

	UtilHelpers::ForEachPlayer(func);
}

CON_COMMAND(sm_navbot_debug_touching, "List entities you're touching.")
{
	edict_t* player = gamehelpers->EdictOfIndex(1);

	Vector mins = player->GetCollideable()->OBBMins() + player->GetCollideable()->GetCollisionOrigin();
	Vector maxs = player->GetCollideable()->OBBMaxs() + player->GetCollideable()->GetCollisionOrigin();

	UtilHelpers::CEntityEnumerator enumerator;
	UtilHelpers::EntitiesInBox(mins, maxs, enumerator);
	auto func = [](CBaseEntity* entity) -> bool {

		META_CONPRINTF("Touching %s \n", gamehelpers->GetEntityClassname(entity));

		return true;
	};

	enumerator.ForEach(func);
}

CON_COMMAND(sm_navbot_debug_spread_danger, "Spread danger to nearby areas")
{
	DECLARE_COMMAND_ARGS;

	edict_t* player = gamehelpers->EdictOfIndex(1);
	const Vector& origin = UtilHelpers::getEntityOrigin(player);
	CNavArea* area = TheNavMesh->GetNearestNavArea(origin, 512.0f);
	float travellimit = 1024.0f;
	float danger = 5000.0f;
	float limit = 10000.0f;

	if (args.ArgC() >= 2)
	{
		travellimit = atof(args[1]);
	}

	if (args.ArgC() >= 3)
	{
		danger = atof(args[2]);
	}

	if (args.ArgC() >= 4)
	{
		limit = atof(args[3]);
	}

	if (area)
	{
		botsharedutils::SpreadDangerToNearbyAreas spread(area, NAV_TEAM_ANY, travellimit, danger, limit);
		spread.Execute();

		META_CONPRINTF("Applied %3.2f danger (limit: %3.2f) to %zu nav areas. Travel distance limit: %3.2f \n", danger, limit, spread.GetCollectedAreasCount(), travellimit);
	}
}

CON_COMMAND(sm_navbot_debug_createfakeclient, "Debug CreateFakeClient.")
{
	DECLARE_COMMAND_ARGS;

	if (args.ArgC() < 2)
	{
		META_CONPRINT("[SM] Usage: sm_navbot_debug_fakeclient <method> \n  0 = CreateFakeClient\n  1 = CreateFakeClientEx\n  2 = BotManager \n");
		return;
	}

	char name[MAX_PLAYER_NAME_LENGTH];
	V_snprintf(name, MAX_PLAYER_NAME_LENGTH, "BOT %04i", randomgen->GetRandomInt<int>(1, 9999));
	int method = atoi(args[1]);
	edict_t* edict = nullptr;

	if (method == 0)
	{
		edict = engine->CreateFakeClient(name);
	}
#if SOURCE_ENGINE == SE_TF2 || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_CSS
	else if (method == 1)
	{
		edict = engine->CreateFakeClientEx(name);
	}
#else
	else if (method == 1)
	{
		META_CONPRINT("CreateFakeClientEx is not available for the current engine branch! \n");
		return;
	}
#endif // SOURCE_ENGINE == SE_TF2 || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_CSS


	else if (method == 2)
	{
		edict = botmanager->CreateBot(name);
	}

	if (!edict)
	{
		META_CONPRINT("NULL bot edict! \n");
		return;
	}

	int index = gamehelpers->IndexOfEdict(edict);
	entprops->SetEntProp(index, Prop_Send, "m_fFlags", 0); // clear flags
	entprops->SetEntProp(index, Prop_Send, "m_fFlags", FL_CLIENT | FL_FAKECLIENT); // set client and fakeclient flags

	CBaseEntity* pEntity = UtilHelpers::EdictToBaseEntity(edict);
	IPlayerInfo* info = playerinfomanager->GetPlayerInfo(edict);
	IBotController* controller = botmanager->GetBotController(edict);

	META_CONPRINTF("Created fake client: %i <%p> PI: %p BC: %p\n", gamehelpers->IndexOfEdict(edict), pEntity, info, controller);
}

CON_COMMAND(sm_navbot_debug_player_info, "Debugs the player info interfaces.")
{
	for (int client = 1; client <= gpGlobals->maxClients; client++)
	{
		edict_t* edict = gamehelpers->EdictOfIndex(client);
		
		if (!UtilHelpers::IsValidEdict(edict))
			continue;

		IPlayerInfo* info = playerinfomanager->GetPlayerInfo(edict);
		IBotController* controller = botmanager->GetBotController(edict);

		META_CONPRINTF("Player %i edict %p player info %p bot controller %p \n", client, edict, info, controller);
	}
}

CON_COMMAND(sm_navbot_debug_ent_iface, "Debug entity interface")
{
	DECLARE_COMMAND_ARGS;

	if (args.ArgC() < 2)
	{
		META_CONPRINT("[SM] Usage: sm_navbot_debug_ent_iface <classname> \n");
		return;
	}

	const char* classname = args[1];
	int ent = UtilHelpers::FindEntityByClassname(INVALID_EHANDLE_INDEX, classname);
	
	if (ent == INVALID_EHANDLE_INDEX)
	{
		META_CONPRINTF("No entity of classname \"%s\" was found! \n", classname);
		return;
	}

	CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(ent);

	IServerEntity* svent = reinterpret_cast<IServerEntity*>(pEntity);
	IServerUnknown* svunk = reinterpret_cast<IServerUnknown*>(pEntity);
	IServerNetworkable* svnet = svent->GetNetworkable();
	IServerNetworkable* unknet = svunk->GetNetworkable();

	META_CONPRINTF("Entity <%s>\nNetworkable (SE): %p \nNetworkable (SU): %p \n", classname, svnet, unknet);
	META_CONPRINTF("Edict (SE): %p\nEdict (SU): %p\n", svnet->GetEdict(), unknet->GetEdict());
	META_CONPRINTF("Base Net (SE): %p\nBase Net (SU): %p\n", svnet->GetBaseNetworkable(), unknet->GetBaseNetworkable());
}

CON_COMMAND(sm_navbot_debug_vector_angles, "")
{
	CBaseExtPlayer player{ UtilHelpers::GetListenServerHost() };

	const QAngle& eyeAngles = player.GetLocalEyeAngles();
	Vector eyeForward;
	AngleVectors(eyeAngles, &eyeForward);
	META_CONPRINTF("Eye Forward: %g %g %g\n", eyeForward.x, eyeForward.y, eyeForward.z);
	QAngle ang1{ 0.0f, -45.0f, 0.0f };
	QAngle ang2{ 0.0f, 45.0f, 0.0f };
	Vector f1;
	Vector f2;
	VectorRotate(eyeForward, ang1, f1);
	VectorRotate(eyeForward, ang2, f2);
	META_CONPRINTF("-45: %g %g %g\n", f1.x, f1.y, f1.z);
	META_CONPRINTF("+45: %g %g %g\n", f2.x, f2.y, f2.z);
	Vector eyePos = player.GetEyeOrigin();
	eyePos.z -= 32.0f;

	debugoverlay->AddLineOverlay(eyePos, eyePos + (eyeForward * 512.0f), 0, 180, 0, true, 10.0f);
	debugoverlay->AddLineOverlay(eyePos, eyePos + (f1 * 512.0f), 255, 0, 0, true, 10.0f);
	debugoverlay->AddLineOverlay(eyePos, eyePos + (f2 * 512.0f), 0, 0, 255, true, 10.0f);
}

CON_COMMAND(sm_navbot_debug_strafe_jump_calcs, "")
{
	static bool set = false;
	static Vector end;

	CBaseExtPlayer player{ UtilHelpers::GetListenServerHost() };

	if (!set)
	{
		set = true;
		end = player.GetAbsOrigin();
		META_CONPRINTF("End Position set! \n");
		return;
	}

	set = false;

	Vector start = player.GetAbsOrigin();
	trace::CTraceFilterSimple filter(player.GetEntity(), COLLISION_GROUP_PLAYER_MOVEMENT);
	trace_t tr;
	const float halfhull = navgenparams->half_human_width;
	const Vector mins{ -halfhull, -halfhull, 0.0f };
	const Vector maxs{ halfhull, halfhull, navgenparams->half_human_height };
	const Vector dir = UtilHelpers::math::BuildDirectionVector(start, end);
	const Vector offset{ 0.0f, 0.0f, navgenparams->half_human_height };
	const float midrange = ((end - start).Length()) * 0.5f;
	start += offset;
	constexpr float arrow_width = 4.0f;

	trace::hull(start, end + offset, mins, maxs, MASK_PLAYERSOLID, &filter, tr);

	// it should always hit on strafe jumps
	if (tr.DidHit())
	{
		for (float y = 5.0f; y <= 45.0f; y += 5.0f)
		{
			QAngle angle{ 0.0f, y, 0.0f };
			Vector newdir;
			VectorRotate(dir, angle, newdir);
			Vector end2 = start + (newdir * midrange);

			trace::hull(start, end2, mins, maxs, MASK_PLAYERSOLID, &filter, tr);

			if (!tr.DidHit())
			{
				NDebugOverlay::HorzArrow(start, end2, arrow_width, 255, 255, 0, 255, true, 10.0f);
				NDebugOverlay::HorzArrow(end2, end, arrow_width, 0, 200, 255, 255, true, 10.0f);
				return;
			}
			else
			{
				NDebugOverlay::HorzArrow(start, end2, arrow_width, 255, 0, 0, 255, true, 10.0f);
			}
		}

		for (float y = -5.0f; y >= -45.0f; y -= 5.0f)
		{
			QAngle angle{ 0.0f, y, 0.0f };
			Vector newdir;
			VectorRotate(dir, angle, newdir);
			Vector end2 = start + (newdir * midrange);

			trace::hull(start, end2, mins, maxs, MASK_PLAYERSOLID, &filter, tr);

			if (!tr.DidHit())
			{
				NDebugOverlay::HorzArrow(start, end2, arrow_width, 255, 255, 0, 255, true, 10.0f);
				NDebugOverlay::HorzArrow(end2, end, arrow_width, 0, 200, 255, 255, true, 10.0f);
				return;
			}
			else
			{
				NDebugOverlay::HorzArrow(start, end2, arrow_width, 255, 0, 0, 255, true, 10.0f);
			}
		}
	}

}

CON_COMMAND(sm_nav_debug_partially_visible, "")
{
	CNavArea* start = TheNavMesh->GetMarkedArea();

	if (!start)
	{
		start = TheNavMesh->GetSelectedArea();

		if (!start)
		{
			return;
		}
	}

	INavAreaCollector<CNavArea> collector{ start, 5000.0f };

	collector.Execute();
	auto& vec = collector.GetCollectedAreas();

	for (CNavArea* area : vec)
	{
		if (area == start)
		{
			continue;
		}

		if (start->IsPartiallyVisible(area))
		{
			area->DrawFilled(0, 180, 0, 200, 15.0f);
		}
		else
		{
			area->DrawFilled(255, 0, 0, 200, 15.0f);
		}
	}
	
}

CON_COMMAND(sm_nav_debug_completely_visible, "")
{
	CNavArea* start = TheNavMesh->GetMarkedArea();

	if (!start)
	{
		start = TheNavMesh->GetSelectedArea();

		if (!start)
		{
			return;
		}
	}

	INavAreaCollector<CNavArea> collector{ start, 5000.0f };

	collector.Execute();
	auto& vec = collector.GetCollectedAreas();

	for (CNavArea* area : vec)
	{
		if (area == start)
		{
			continue;
		}

		if (start->IsCompletelyVisible(area))
		{
			area->DrawFilled(0, 180, 0, 200, 15.0f);
		}
		else
		{
			area->DrawFilled(255, 0, 0, 200, 15.0f);
		}
	}

}

CON_COMMAND(sm_navbot_debug_find_water_surface, "")
{
	CBaseExtPlayer player{ UtilHelpers::GetListenServerHost() };

	Vector start = player.GetAbsOrigin();
	Vector surface = trace::getwatersurface(start);

	NDebugOverlay::Cross3D(surface, 16.0f, 0, 200, 0, true, 20.0f);
}

CON_COMMAND(sm_navbot_debug_nav_ladder_dot, "")
{
	CNavLadder* ladder = TheNavMesh->GetMarkedLadder();

	if (!ladder) { META_CONPRINT("MARK A LADDER FIRST!\n"); return; }

	CBaseExtPlayer player{ UtilHelpers::GetListenServerHost() };

	Vector origin = player.GetAbsOrigin();
	Vector bottom = ladder->m_bottom;
	Vector to = UtilHelpers::math::BuildDirectionVector(origin, bottom);
	const float dot = DotProduct(to, ladder->GetNormal());

	META_CONPRINTF("Dot: %g\n    %g %g %g", dot, to.x, to.y, to.z);
}

CON_COMMAND(sm_navbot_debug_ent_io, "")
{
	DECLARE_COMMAND_ARGS;

	if (args.ArgC() < 2)
	{
		META_CONPRINT("[SM] Usage: sm_navbot_debug_ent_io <ent index>\n");
		return;
	}

	CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(atoi(args[1]));

	if (!pEntity)
	{
		return;
	}

	datamap_t* datamap = gamehelpers->GetDataMap(pEntity);

	while (datamap != nullptr)
	{
		int nfields = datamap->dataNumFields;

		for (int i = 0; i < nfields; i++)
		{
			typedescription_t* dataDesc = &datamap->dataDesc[i];

			if (datamap->dataDesc[i].flags & FTYPEDESC_INPUT)
			{
				META_CONPRINTF("--- INPUT: %s ---\n", dataDesc->externalName);
			}

			if (dataDesc->fieldType == FIELD_CUSTOM && (dataDesc->flags & FTYPEDESC_OUTPUT) != 0)
			{
#if SOURCE_ENGINE >= SE_LEFT4DEAD
				CBaseEntityOutput* pOutput = reinterpret_cast<CBaseEntityOutput*>(reinterpret_cast<std::intptr_t>(pEntity) + static_cast<int>(dataDesc->fieldOffset));
#else
				CBaseEntityOutput* pOutput = reinterpret_cast<CBaseEntityOutput*>(reinterpret_cast<std::intptr_t>(pEntity) + static_cast<int>(dataDesc->fieldOffset[0]));
#endif // SOURCE_ENGINE >= SE_LEFT4DEAD

				META_CONPRINTF("--- OUTPUT: %s ---\n", dataDesc->externalName);

				for (CEventAction* ev = pOutput->GetFirstAction(); ev != nullptr; ev = ev->m_pNext)
				{
					META_CONPRINT("-- EVENT ACTION --\n");

					if (ev->m_iTarget != NULL_STRING)
					{
						const char* target = STRING(ev->m_iTarget);
						META_CONPRINTF("    Target: %s \n", target);
					}

					if (ev->m_iTargetInput != NULL_STRING)
					{
						const char* input = STRING(ev->m_iTargetInput);
						META_CONPRINTF("    Input: %s \n", input);
					}
				}
			}
		}

		datamap = datamap->baseMap;
	}
}

CON_COMMAND(sm_navbot_debug_dump_entity_inheritance, "")
{
	DECLARE_COMMAND_ARGS;

	if (args.ArgC() < 2)
	{
		META_CONPRINT("[SM] Usage: sm_navbot_debug_dump_entity_inheritance <ent index>\n");
		return;
	}

	CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(atoi(args[1]));

	if (!pEntity)
	{
		return;
	}

	datamap_t* dmap = gamehelpers->GetDataMap(pEntity);

	if (!dmap)
	{
		return;
	}

	META_CONPRINTF("Entity inheritance for %s \n", gamehelpers->GetEntityClassname(pEntity));

	std::array<char, 512> dump;
	std::fill(dump.begin(), dump.end(), '\0');

	ke::SafeSprintf(dump.data(), dump.size(), "%s ", dmap->dataClassName);

	while (dmap != nullptr)
	{
		datamap_t* next = dmap->baseMap;

		if (!next) { break; }

		size_t pos = ke::SafeStrcat(dump.data(), dump.size(), next->dataClassName);

		if (pos + 2U >= dump.size()) { break; }

		dump[pos] = ' ';
		dump[pos + 1U] = '\0';

		dmap = next;
	}

	META_CONPRINTF("%s\n", dump.data());
}

CON_COMMAND(sm_navbot_debug_command_args, "Debug command arguments functions.")
{
	DECLARE_COMMAND_ARGS;

	const char* foo = args.FindArg("-foo");

	if (foo && foo[0] != '\0')
	{
		META_CONPRINTF("Found \"-foo\" argument: %s \n", foo);
	}

	META_CONPRINT("Dumping all arguments: \n");

	for (int i = 0; i < args.ArgC(); i++)
	{
		META_CONPRINTF("%s\n", args[i]);
	}
}

CON_COMMAND(sm_navbot_debug_closest_point, "Gets the closest point of the given entity AABB.")
{
	DECLARE_COMMAND_ARGS;

	if (args.ArgC() < 2)
	{
		META_CONPRINT("[SM] Usage: sm_navbot_debug_closest_point <ent index>\n");
		return;
	}

	CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(atoi(args[1]));

	if (!pEntity)
	{
		return;
	}

	CBaseExtPlayer* host = extmanager->GetListenServerHost();

	if (host)
	{
		Vector from = host->GetEyeOrigin();
		Vector point;
		UtilHelpers::math::CalcClosestPointOfEntity(pEntity, from, point);
		NDebugOverlay::Line(from, point, 255, 255, 0, true, 30.0f);
		Vector mins{ -1.0f, -1.0f, -1.0f };
		Vector maxs{ 1.0f, 1.0f, 1.0f };
		NDebugOverlay::Box(point, mins, maxs, 255, 255, 0, 255, 30.0f);
	}

}

CON_COMMAND(sm_navbot_debug_fireinput, "Fires an entity input.")
{
	DECLARE_COMMAND_ARGS;

	if (args.ArgC() < 3)
	{
		META_CONPRINT("[SM] Usage: sm_navbot_debug_fireinput <ent index> <input name>\n");
		META_CONPRINT("Optional arguments: -caller <ent index>  -activator <ent index> -outputid <integer>\n");
		return;
	}

	CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(atoi(args[1]));

	if (!pEntity)
	{
		return;
	}

	CBaseExtPlayer* host = extmanager->GetListenServerHost();

	if (host)
	{
		const char* input = args[2];
		variant_t variant;
		int outputID = 0;
		CBaseEntity* pCaller = host->GetEntity();
		CBaseEntity* pActivator = host->GetEntity();

		if (args.ArgC() >= 5)
		{
			const char* arg = args.FindArg("-caller");
			CBaseEntity* ent = nullptr;

			if (arg)
			{
				ent = gamehelpers->ReferenceToEntity(atoi(arg));

				if (ent)
				{
					pCaller = ent;
				}
			}

			arg = args.FindArg("-activator");

			if (arg)
			{
				ent = gamehelpers->ReferenceToEntity(atoi(arg));

				if (ent)
				{
					pActivator = ent;
				}
			}

			arg = args.FindArg("-outputid");

			if (arg)
			{
				outputID = atoi(arg);
			}
		}


		META_CONPRINTF("Firing Input \"%s\" on %s\n", input, UtilHelpers::textformat::FormatEntity(pEntity));
		UtilHelpers::io::FireInput(pEntity, input, pActivator, pCaller, variant, outputID);
	}

}

CON_COMMAND_F(sm_navbot_debug_ent_targetname, "Debugs entities targetnames", FCVAR_GAMEDLL | FCVAR_CHEAT)
{
	DECLARE_COMMAND_ARGS;

	if (args.ArgC() < 2)
	{
		META_CONPRINT("[SM] Usage: sm_navbot_debug_ent_targetname <ent index>\n");
		return;
	}

	CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(atoi(args[1]));

	if (!pEntity)
	{
		return;
	}

	char targetname[128];
	std::memset(targetname, 0, sizeof(targetname));
	size_t length = 0;
	entprops->GetEntPropString(pEntity, Prop_Data, "m_iName", targetname, sizeof(targetname), length);

	META_CONPRINTF("Entity %s target name is %s <%zu>\n", UtilHelpers::textformat::FormatEntity(pEntity), targetname, length);

	if (targetname[0] == '\0')
	{
		META_CONPRINTF("targetname[0] == 0\n");
	}
}

CON_COMMAND(sm_nav_debug_snap_to_line, "")
{
	CBaseExtPlayer* player = extmanager->GetListenServerHost();
	Vector pos = player->GetAbsOrigin();

	CNavArea* area = TheNavMesh->GetMarkedArea();

	if (!area)
	{
		return;
	}

	const Vector& center = area->GetCenter();
	Vector to = UtilHelpers::math::BuildDirectionVector(pos, center);
	QAngle angles;
	VectorAngles(to, angles);
	const float yaw = angles.y;
	const bool AlongX = (yaw < 45.0f || yaw > 315.0f) || (yaw > 135.0f && yaw < 225.0f);

	if (!AlongX)
	{
		pos.x = center.x;
	}
	else
	{
		pos.y = center.y;
	}

	NDebugOverlay::Cross3D(pos, 10.0f, 255, 0, 0, true, 10.0f);
}

#endif // EXT_DEBUG