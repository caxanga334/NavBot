#include <memory>
#include <string>
#include <sstream>

#include <extension.h>
#include <manager.h>
#include <bot/basebot.h>
#include <bot/interfaces/behavior.h>
#include <bot/interfaces/path/basepath.h>
#include <entities/tf2/tf_entities.h>
#include <navmesh/nav.h>
#include <navmesh/nav_area.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_pathfind.h>
#include <sdkports/debugoverlay_shared.h>
#include <sdkports/sdk_ehandle.h>
#include <sdkports/sdk_traces.h>
#include <util/helpers.h>
#include <util/librandom.h>
#include <util/ehandle_edict.h>
#include <util/sdkcalls.h>
#include <util/entprops.h>
#include <sm_argbuffer.h>

#ifdef EXT_DEBUG
CON_COMMAND(sm_navbot_debug_bot_look, "Debug the bot look functions.")
{
	edict_t* host = gamehelpers->EdictOfIndex(1);
	Vector target = UtilHelpers::getWorldSpaceCenter(host);

	extmanager->ForEachBot([&target](CBaseBot* bot) {
		bot->GetControlInterface()->AimAt(target, IPlayerController::LOOK_CRITICAL, 10.0f, "Debug command!");
	});
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

CON_COMMAND_F(sm_navbot_debug_do_not_use, "Do not even think about executing this command.", FCVAR_CHEAT)
{
	throw std::runtime_error("Exception test!");
}

CON_COMMAND(sm_navbot_debug_event_propagation, "Event propagation test")
{
	auto& bots = extmanager->GetAllBots();

	for (auto& botptr : bots)
	{
		auto bot = botptr.get();
		bot->OnTestEventPropagation();
		bot->GetBehaviorInterface()->ShouldFreeRoam(bot);
	}
}

CON_COMMAND(sm_navbot_debug_pathfind, "Path finding debug.")
{
	static Vector start;
	static Vector end;
	static bool reset = false;

	auto edict = gamehelpers->EdictOfIndex(1);

	if (edict == nullptr)
	{
		return;
	}

	// CBaseExtPlayer can be used for both players and bots
	CBaseExtPlayer player(edict);

	if (reset == false)
	{
		start = player.GetAbsOrigin();
		reset = true;
		NDebugOverlay::SweptBox(start, start, player.GetMins(), player.GetMaxs(), vec3_angle, 0, 220, 255, 255, 15.0f);
		rootconsole->ConsolePrint("Start position set to <%3.2f, %3.2f, %3.2f>", start.x, start.y, start.z);
		return;
	}

	reset = false;
	end = player.GetAbsOrigin();

	CNavArea* startArea = TheNavMesh->GetNearestNavArea(start, 256.0f, true, true);

	if (startArea == nullptr)
	{
		rootconsole->ConsolePrint("Path find: Failed to get starting area!");
		return;
	}

	CNavArea* goalArea = TheNavMesh->GetNearestNavArea(end, 256.0f, true, true);

	ShortestPathCost cost;
	CNavArea* closest = nullptr;
	bool path = NavAreaBuildPath(startArea, goalArea, &end, cost, &closest, 10000.0f, -2);

	CNavArea* from = nullptr;
	CNavArea* to = nullptr;

	for (CNavArea* area = closest; area != nullptr; area = area->GetParent())
	{
		if (from == nullptr) // set starting area;
		{
			from = area;
			continue;
		}

		to = area;

		NDebugOverlay::HorzArrow(from->GetCenter(), to->GetCenter(), 4.0f, 0, 255, 0, 255, true, 40.0f);

		rootconsole->ConsolePrint("Path: from #%i to #%i. gap: %3.2f", from->GetID(), to->GetID(), from->ComputeAdjacentConnectionGapDistance(to));

		from = to;
	}

	rootconsole->ConsolePrint("Path found!");
}

CON_COMMAND(sm_navbot_debug_botpath, "Debug bot path")
{
	CBaseBot* bot = nullptr;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		bot = extmanager->GetBotByIndex(i);

		if (bot != nullptr)
		{
			break;
		}
	}

	if (bot == nullptr)
	{
		rootconsole->ConsolePrint("Add a bot to the game first!");
		return;
	}

	auto edict = gamehelpers->EdictOfIndex(1);

	if (edict == nullptr)
	{
		return;
	}

	// CBaseExtPlayer can be used for both players and bots
	CBaseExtPlayer player(edict);
	ShortestPathCost cost;
	CPath path;
	Vector start = player.GetAbsOrigin();

	path.ComputePathToPosition(bot, start, cost, 0.0f, true);
	path.DrawFullPath(30.0f);

	rootconsole->ConsolePrint("Showing path from bot position to your position.");
}

CON_COMMAND(sm_navbot_debug_rng, "Debug random number generator")
{
	for (int i = 0; i < 15; i++)
	{
		int n = librandom::generate_random_int(0, 100);
		rootconsole->ConsolePrint("Random Number: %i", n);
	}
}

CON_COMMAND_F(free_nullptr_crash_for_everyone, "Crashes your game LMAO", FCVAR_CHEAT)
{
	auto edict = gamehelpers->EdictOfIndex(2040);
	auto& origin = edict->GetCollideable()->GetCollisionOrigin();
	Msg("%3.2f %3.2f %3.2f \n", origin.x, origin.y, origin.z);
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

CON_COMMAND_F(sm_nav_debug_area_collector, "Debugs nav area collector.", FCVAR_CHEAT)
{
	edict_t* host = gamehelpers->EdictOfIndex(1);
	CBaseExtPlayer player(host);
	player.UpdateLastKnownNavArea(true);
	CNavArea* area = player.GetLastKnownNavArea();

	if (!area)
	{
		Warning("No Last Known Nav Area!\n");
		return;
	}

	std::vector<CNavArea*> collectedAreas;
	collectedAreas.reserve(512);
	Vector origin = player.GetAbsOrigin();

	NavCollectSurroundingAreas(area, collectedAreas, [](CNavArea* previousArea, CNavArea* currentArea, const float totalCostSoFar, float& newCost) -> bool {

		if (previousArea == nullptr) // first area in search
		{
			return true;
		}

		float cost = totalCostSoFar;
		auto link = previousArea->GetSpecialLinkConnectionToArea(currentArea);

		if (link)
		{
			cost += link->GetConnectionLength();
		}
		else
		{
			cost += (previousArea->GetCenter() - currentArea->GetCenter()).Length();
		}

		newCost = cost;

		if (cost >= 1300.0f)
		{
			return false;
		}

		return true;
	});

	for (auto area : collectedAreas)
	{
		NDebugOverlay::Line(origin, area->GetCenter(), 0, 200, 255, true, 20.0f);
	}

	Msg("Collected %i areas \n", collectedAreas.size());
}

CON_COMMAND_F(sm_nav_debug_area_collector_2, "Debugs nav area collector.", FCVAR_CHEAT)
{
	edict_t* host = gamehelpers->EdictOfIndex(1);
	CBaseExtPlayer player(host);
	player.UpdateLastKnownNavArea(true);
	CNavArea* area = player.GetLastKnownNavArea();
	Vector origin = player.GetEyeOrigin();

	if (!area)
	{
		Warning("No Last Known Nav Area!\n");
		return;
	}

	INavAreaCollector<CNavArea> collector(area, 512.0f);
	collector.Execute();
	
	auto& vec = collector.GetCollectedAreas();
	Vector offset(0.0f, 0.0f, 32.0f);

	for (auto area : vec)
	{
		NDebugOverlay::Line(origin, area->GetCenter() + offset, 0, 200, 255, true, 20.0f);
	}

	Msg("Collected %i areas.\n", collector.GetCollectedAreasCount());
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
	trace::line(start, end, MASK_SOLID, result);
	Msg("Trace Result: fraction %3.6f \n", result.fraction);
}

CON_COMMAND_F(sm_navbot_debug_sdkcalls, "Debug new SDKCalls functions.", FCVAR_CHEAT)
{
	edict_t* ent = gamehelpers->EdictOfIndex(1);
	CBaseExtPlayer player(ent);

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

CON_COMMAND(sm_navbot_debug_tests, "Debug stuff")
{
	using namespace SourceMod;

	int index = UtilHelpers::FindEntityByClassname(-1, "item_teamflag");
	CBaseEntity* entity = gamehelpers->ReferenceToEntity(index);

	if (entity == nullptr)
	{
		Warning("Failed to get a pointer to a item_teamflag entity!\n");
		return;
	}

	ServerClass* pClass = gamehelpers->FindEntityServerClass(entity);
	sm_sendprop_info_t info;

	if (pClass != nullptr && gamehelpers->FindSendPropInfo(pClass->GetName(), "m_flTimeToSetPoisonous", &info))
	{
		static constexpr auto size = sizeof(EHANDLE) + sizeof(int);
		int offset = info.actual_offset + size;
		Msg("Computed offset for CCaptureFlag::m_vecResetPos %i\n", offset);

		Vector res;
		
		if (entprops->GetEntDataVector(index, offset, res))
		{
			Msg("Value at offset: %3.2f %3.2f %3.2f\n", res.x, res.y, res.z);

			entities::HBaseEntity be(entity);
			res = be.GetAbsOrigin();
			Msg("Abs origin: %3.2f %3.2f %3.2f\n", res.x, res.y, res.z);
		}
	}

	tfentities::HCaptureFlag flag(entity);

	Vector out = flag.GetReturnPosition();
	Msg("Flag return position: %3.2f %3.2f %3.2f\n", out.x, out.y, out.z);
}

#endif // EXT_DEBUG