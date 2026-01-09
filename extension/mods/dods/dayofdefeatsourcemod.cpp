#include NAVBOT_PCH_FILE
#include <cstring>

#include <extension.h>
#include <manager.h>
#include <bot/dods/dodsbot.h>
#include <util/entprops.h>
#include <util/helpers.h>
#include "dodslib.h"
#include "nav/dods_nav_mesh.h"
#include "dayofdefeatsourcemod.h"

#if SOURCE_ENGINE <= SE_DARKMESSIAH
#include <sdkports/sdk_convarref_ep1.h>
#endif

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

void CDODObjectiveResource::Init(CBaseEntity* entity)
{
	m_iNumControlPoints = entprops->GetPointerToEntData<int>(entity, Prop_Send, "m_iNumControlPoints");
	m_bCPIsVisible = entprops->GetPointerToEntData<int>(entity, Prop_Send, "m_bCPIsVisible");
	m_iAlliesReqCappers = entprops->GetPointerToEntData<int>(entity, Prop_Send, "m_iAlliesReqCappers");
	m_iAxisReqCappers = entprops->GetPointerToEntData<int>(entity, Prop_Send, "m_iAxisReqCappers");
	m_bBombPlanted = entprops->GetPointerToEntData<int>(entity, Prop_Send, "m_bBombPlanted");
	m_iBombsRequired = entprops->GetPointerToEntData<int>(entity, Prop_Send, "m_iBombsRequired");
	m_iBombsRemaining = entprops->GetPointerToEntData<int>(entity, Prop_Send, "m_iBombsRemaining");
	m_bBombBeingDefused = entprops->GetPointerToEntData<int>(entity, Prop_Send, "m_bBombBeingDefused");
	m_iNumAllies = entprops->GetPointerToEntData<int>(entity, Prop_Send, "m_iNumAllies");
	m_iNumAxis = entprops->GetPointerToEntData<int>(entity, Prop_Send, "m_iNumAxis");
	m_iCappingTeam = entprops->GetPointerToEntData<int>(entity, Prop_Send, "m_iCappingTeam");
	m_iOwner = entprops->GetPointerToEntData<int>(entity, Prop_Send, "m_iOwner");
}

CDayOfDefeatSourceMod::CDayOfDefeatSourceMod() :
	CBaseMod(), m_objectiveres(), m_mapUsesBombs(false), m_random_class_allowed(true)
{
	ListenForGameEvent("dod_round_start");
	ListenForGameEvent("dod_bomb_planted");
	ListenForGameEvent("dod_bomb_defused");
}

CDayOfDefeatSourceMod::~CDayOfDefeatSourceMod()
{
}

CDayOfDefeatSourceMod* CDayOfDefeatSourceMod::GetDODMod()
{
	return static_cast<CDayOfDefeatSourceMod*>(extmanager->GetMod());
}

void CDayOfDefeatSourceMod::FireGameEvent(IGameEvent* event)
{
	if (event)
	{
#if defined(EXT_DEBUG) && SOURCE_ENGINE == SE_DODS
		ConColorMsg(Color(71, 98, 145, 255), "CDayOfDefeatSourceMod::FireGameEvent %s \n", event->GetName());
#endif // defined(EXT_DEBUG) && SOURCE_ENGINE == SE_DODS

		if (std::strcmp("dod_round_start", event->GetName()) == 0)
		{
			OnRoundStart();
		}
		else if (std::strcmp("dod_bomb_planted", event->GetName()) == 0)
		{
			// we need to delay this a bit
			m_bombevents.emplace(event, BOMB_PLANTED);
		}
		else if (std::strcmp("dod_bomb_defused", event->GetName()) == 0)
		{
			m_bombevents.emplace(event, BOMB_DEFUSED);
		}
	}

	CBaseMod::FireGameEvent(event);
}

void CDayOfDefeatSourceMod::Update()
{
	CBaseMod::Update();

	if (!m_bombevents.empty())
	{
		auto& event = m_bombevents.front();
		OnBombEvent(event);
		m_bombevents.pop();
	}
}

CBaseBot* CDayOfDefeatSourceMod::AllocateBot(edict_t* edict)
{
	return new CDoDSBot(edict);
}

CNavMesh* CDayOfDefeatSourceMod::NavMeshFactory()
{
	return new CDoDSNavMesh;
}

void CDayOfDefeatSourceMod::OnMapStart()
{
	CBaseMod::OnMapStart();

	{
		// clear the bomb event queue
		std::queue<BombEvent> emptyQueue;
		std::swap(m_bombevents, emptyQueue);
	}

	m_mapUsesBombs = (UtilHelpers::FindEntityByClassname(INVALID_EHANDLE_INDEX, "dod_bomb_target") != INVALID_EHANDLE_INDEX);
}

void CDayOfDefeatSourceMod::OnRoundStart()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CDayOfDefeatSourceMod::OnRoundStart", "NavBot");
#endif // EXT_VPROF_ENABLED

	{
		// clear the bomb event queue
		std::queue<BombEvent> emptyQueue;
		std::swap(m_bombevents, emptyQueue);
	}

	while (!m_bombevents.empty())
	{
		m_bombevents.pop();
	}

	CBaseMod::OnRoundStart();
	librandom::ReSeedGlobalGenerators();

	FindObjectiveResourceEntity();
	FindControlPoints();
	// These two needs to be after findcontrolpoints
	FindAndAssignCaptureAreas();
	FindAndAssignBombTargets();
	UpdateClassLimits();

	CNavMesh::NotifyRoundRestart();

	auto func = [](CBaseBot* bot) {
		bot->OnRoundStateChanged(); // Notify behavior about round restart
	};

	extmanager->ForEachBot(func);
}

const CDODObjectiveResource* CDayOfDefeatSourceMod::GetDODObjectiveResource() const
{
	if (m_objectiveentity.Get() != nullptr)
	{
		return &m_objectiveres;
	}

	return nullptr;
}

const CDayOfDefeatSourceMod::DoDControlPoint* CDayOfDefeatSourceMod::GetControlPointByIndex(int index) const
{
	for (auto& controlpoint : m_controlpoints)
	{
		if (controlpoint.index == index)
		{
			return &controlpoint;
		}
	}

	return nullptr;
}

CBaseEntity* CDayOfDefeatSourceMod::GetControlPointBombToDefuse(CBaseEntity* pControlPoint) const
{
	for (auto& controlpoint : m_controlpoints)
	{
		if (controlpoint.point.Get() == pControlPoint)
		{
			for (auto& bomb : controlpoint.bomb_targets)
			{
				CBaseEntity* pBomb = bomb.Get();

				if (pBomb)
				{
					int state = 0;
					entprops->GetEntProp(pBomb, Prop_Send, "m_iState", state);

					if (state == static_cast<int>(dayofdefeatsource::DoDBombTargetState::BOMB_TARGET_ARMED))
					{
						return pBomb;
					}
				}
			}

			break;
		}
	}

	return nullptr;
}

void CDayOfDefeatSourceMod::CollectControlPointsToAttack(dayofdefeatsource::DoDTeam team, std::vector<const CDayOfDefeatSourceMod::DoDControlPoint*>& points) const
{
	const CDODObjectiveResource* objres = GetDODObjectiveResource();

	if (!objres)
	{
		return;
	}

	for (const CDayOfDefeatSourceMod::DoDControlPoint& controlpoint : m_controlpoints)
	{
#ifdef EXT_DEBUG
		// Valid index check, debug only
		if (controlpoint.index < dayofdefeatsource::INVALID_CONTROL_POINT || controlpoint.index >= MAX_CONTROL_POINTS)
		{
			smutils->LogError(myself,"CollectControlPointsToAttack Invalid Control Point index %i", controlpoint.index);
			continue;
		}
#endif // EXT_DEBUG

		// skip: invalid or not owned by my team
		if (controlpoint.index == dayofdefeatsource::INVALID_CONTROL_POINT || controlpoint.point.Get() == nullptr || 
			objres->GetOwnerTeamIndex(controlpoint.index) == static_cast<int>(team))
		{
			continue;
		}

		// skip: control point is not visible
		if (!objres->IsControlPointVisible(controlpoint.index))
		{
			continue;
		}

		// skip: all bombs already planted
		if (objres->GetNumBombsRequired(controlpoint.index) > 0 && objres->GetNumBombsRemaining(controlpoint.index) == 0)
		{
			continue;
		}

		// skip: doesn't need bombs, check the capture trigger
		if (objres->GetNumBombsRequired(controlpoint.index) == 0)
		{
			CBaseEntity* captureArea = controlpoint.capture_trigger.Get();

			if (!captureArea)
			{
				continue; // no bomb and no capture area?
			}

			bool disabled = false;
			entprops->GetEntPropBool(controlpoint.capture_trigger.GetEntryIndex(), Prop_Data, "m_bDisabled", disabled);

			if (disabled)
			{
				continue; // capture trigger for this control point is disabled
			}
		}

		points.push_back(&controlpoint);
	}
}

void CDayOfDefeatSourceMod::CollectControlPointsToDefend(dayofdefeatsource::DoDTeam team, std::vector<const CDayOfDefeatSourceMod::DoDControlPoint*>& points) const
{
	const CDODObjectiveResource* objres = GetDODObjectiveResource();

	if (!objres)
	{
		return;
	}

	for (const CDayOfDefeatSourceMod::DoDControlPoint& controlpoint : m_controlpoints)
	{
#ifdef EXT_DEBUG
		// Valid index check, debug only
		if (controlpoint.index < dayofdefeatsource::INVALID_CONTROL_POINT || controlpoint.index >= MAX_CONTROL_POINTS)
		{
			smutils->LogError(myself,"CollectControlPointsToDefend Invalid Control Point index %i", controlpoint.index);
			continue;
		}
#endif // EXT_DEBUG

		// skip: invalid or not owned by my team
		if (controlpoint.index == dayofdefeatsource::INVALID_CONTROL_POINT || controlpoint.point.Get() == nullptr ||
			objres->GetOwnerTeamIndex(controlpoint.index) != static_cast<int>(team))
		{
			continue;
		}

		// skip: control point is not visible
		if (!objres->IsControlPointVisible(controlpoint.index))
		{
			continue;
		}

		// skip: all bombs already planted
		if (objres->GetNumBombsRequired(controlpoint.index) > 0 && objres->GetNumBombsRemaining(controlpoint.index) == 0)
		{
			continue;
		}

		// skip: doesn't need bombs, check the capture trigger
		if (objres->GetNumBombsRequired(controlpoint.index) == 0)
		{
			CBaseEntity* captureArea = controlpoint.capture_trigger.Get();

			if (!captureArea)
			{
				continue; // no bomb and no capture area?
			}

			bool disabled = false;
			entprops->GetEntPropBool(controlpoint.capture_trigger.GetEntryIndex(), Prop_Data, "m_bDisabled", disabled);

			if (disabled)
			{
				continue; // capture trigger for this control point is disabled
			}
		}

		points.push_back(&controlpoint);
	}
}

dayofdefeatsource::DoDClassType CDayOfDefeatSourceMod::SelectClassForBot(CDoDSBot* bot) const
{
	// TO-DO: add chance config
	if (m_random_class_allowed && CBaseBot::s_botrng.GetRandomChance(20))
	{
		return dayofdefeatsource::DoDClassType::NUM_DOD_CLASSES;
	}

	auto myteam = bot->GetMyDoDTeam();
	dodslib::DoDClassCountArray as_class;
	dodslib::GetTeamClassCount(myteam, as_class);
	auto myclass = dodslib::GetPlayerClassType(bot->GetEntity());
	std::vector<dayofdefeatsource::DoDClassType> available_classes;
	std::vector<int> weights;
	
	for (int cls = 0; cls < static_cast<int>(dayofdefeatsource::DoDClassType::NUM_DOD_CLASSES); cls++)
	{
		if (static_cast<dayofdefeatsource::DoDClassType>(cls) == myclass)
		{
			continue; // already using this class
		}

		int c = as_class[cls]; // players as this class
		int limit = GetClassLimit(static_cast<dayofdefeatsource::DoDClassType>(cls), myteam);

		// 0 means not allowed to use this class
		if (limit == 0)
		{
			continue;
		}

		// negative numbers means unlimited
		if (limit <= dayofdefeatsource::DOD_NO_CLASS_LIMIT || c < limit)
		{
			available_classes.push_back(static_cast<dayofdefeatsource::DoDClassType>(cls));
			continue;
		}
	}

	if (available_classes.empty())
	{
		return dayofdefeatsource::DoDClassType::DODCLASS_INVALID;
	}

	if (available_classes.size() == 1)
	{
		return available_classes[0];
	}

	// calculate weights
	int t = 0;

	for (auto& cls : available_classes)
	{
		t += as_class[static_cast<int>(cls)];
	}

	for (auto& cls : available_classes)
	{
		// avoid division by zero
		if (as_class[static_cast<int>(cls)] == 0)
		{
			weights.push_back(t);
			continue;
		}

		int weight = t / as_class[static_cast<int>(cls)];
		weights.push_back(weight);
	}

	EXT_ASSERT(weights.size() == available_classes.size(), "Available classes and class weight vector size mismatch!");

	return CBaseBot::s_botrng.GetWeightedRandom(available_classes, weights);
}

int CDayOfDefeatSourceMod::GetClassLimit(dayofdefeatsource::DoDClassType cls, dayofdefeatsource::DoDTeam team) const
{
	if (team == dayofdefeatsource::DoDTeam::DODTEAM_ALLIES)
	{
		switch (cls)
		{
		case dayofdefeatsource::DoDClassType::DODCLASS_RIFLEMAN:
			return m_classlimits_allies.rifleman;
		case dayofdefeatsource::DoDClassType::DODCLASS_ASSAULT:
			return m_classlimits_allies.assault;
		case dayofdefeatsource::DoDClassType::DODCLASS_SUPPORT:
			return m_classlimits_allies.support;
		case dayofdefeatsource::DoDClassType::DODCLASS_SNIPER:
			return m_classlimits_allies.sniper;
		case dayofdefeatsource::DoDClassType::DODCLASS_MACHINEGUNNER:
			return m_classlimits_allies.mg;
		case dayofdefeatsource::DoDClassType::DODCLASS_ROCKET:
			return m_classlimits_allies.rocket;
		default:
			break;
		}
	}

	switch (cls)
	{
	case dayofdefeatsource::DoDClassType::DODCLASS_RIFLEMAN:
		return m_classlimits_axis.rifleman;
	case dayofdefeatsource::DoDClassType::DODCLASS_ASSAULT:
		return m_classlimits_axis.assault;
	case dayofdefeatsource::DoDClassType::DODCLASS_SUPPORT:
		return m_classlimits_axis.support;
	case dayofdefeatsource::DoDClassType::DODCLASS_SNIPER:
		return m_classlimits_axis.sniper;
	case dayofdefeatsource::DoDClassType::DODCLASS_MACHINEGUNNER:
		return m_classlimits_axis.mg;
	case dayofdefeatsource::DoDClassType::DODCLASS_ROCKET:
		return m_classlimits_axis.rocket;
	default:
		break;
	}

	return dayofdefeatsource::DOD_NO_CLASS_LIMIT;
}

CDoDSSharedBotMemory* CDayOfDefeatSourceMod::GetSharedBotMemory(int teamindex)
{
	return static_cast<CDoDSSharedBotMemory*>(CBaseMod::GetSharedBotMemory(teamindex));
}

CModSettings* CDayOfDefeatSourceMod::CreateModSettings() const
{
	return new CDoDModSettings;
}

ISharedBotMemory* CDayOfDefeatSourceMod::CreateSharedMemory() const
{
	return new CDoDSSharedBotMemory;
}

void CDayOfDefeatSourceMod::FindObjectiveResourceEntity()
{
	m_objectiveentity = nullptr;

	int index = UtilHelpers::FindEntityByNetClass(INVALID_EHANDLE_INDEX, "CDODObjectiveResource");

	if (index != INVALID_EHANDLE_INDEX)
	{
		edict_t* edict = gamehelpers->EdictOfIndex(index);

		if (edict)
		{
			CBaseEntity* pEntity = UtilHelpers::EdictToBaseEntity(edict);
			m_objectiveentity = pEntity;
			m_objectiveres.Init(pEntity);

#ifdef EXT_DEBUG
			smutils->LogMessage(myself, "CDayOfDefeatSourceMod::FindObjectiveResourceEntity found objective resource entity #%i <%p>", index, pEntity);
#endif // EXT_DEBUG
		}
	}
}

void CDayOfDefeatSourceMod::FindControlPoints()
{
	for (auto& controlpoint : m_controlpoints)
	{
		controlpoint.Reset();
	}

	std::size_t it = 0;

	auto functor = [&it, this](int index, edict_t* edict, CBaseEntity* entity) {

		if (it >= m_controlpoints.max_size())
		{
			return false;
		}

		if (entity)
		{
			m_controlpoints[it].point = entity;
			m_controlpoints[it].index = dodslib::GetControlPointIndex(entity);
			it++;
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("dod_control_point", functor);

#ifdef EXT_DEBUG
	META_CONPRINTF("Found %zu control point entities. Objective Resource reports %i entities. \n", it, m_objectiveres.GetControlPointCount());
#endif // EXT_DEBUG
}

void CDayOfDefeatSourceMod::FindAndAssignCaptureAreas()
{
	for (auto& controlpoint : m_controlpoints)
	{
		CBaseEntity* entity = controlpoint.point.Get();

		if (entity)
		{
			constexpr int str_len = 256;
			std::size_t length = 0;
			std::string name(str_len, '\0');

			entprops->GetEntPropString(controlpoint.point.GetEntryIndex(), Prop_Data, "m_iName", name.data(), str_len, length);

			if (length == 0U)
			{
				continue; // no targetname set
			}
			
			auto functor = [&controlpoint, &name, &str_len](int index, edict_t* edict, CBaseEntity* entity) {

				std::size_t length = 0;
				std::string assigned_point_name(str_len, '\0');
				entprops->GetEntPropString(index, Prop_Data, "m_iszCapPointName", assigned_point_name.data(), str_len, length);

				if (length == 0U)
				{
					return true;
				}

				if (std::strcmp(name.c_str(), assigned_point_name.c_str()) == 0)
				{
#ifdef EXT_DEBUG
					META_CONPRINTF("Found dod_capture_area #%i assigned to control point #%i <%s> \n", index, controlpoint.point.GetEntryIndex(), name.c_str());
#endif // EXT_DEBUG

					controlpoint.capture_trigger = entity;
					return false; // End loop
				}

				return true;
			};

			UtilHelpers::ForEachEntityOfClassname("dod_capture_area", functor);
		}
	}
}

void CDayOfDefeatSourceMod::FindAndAssignBombTargets()
{
	for (auto& controlpoint : m_controlpoints)
	{
		CBaseEntity* entity = controlpoint.point.Get();

		if (entity)
		{
			constexpr int str_len = 256;
			std::size_t length = 0;
			std::string name(str_len, '\0');

			entprops->GetEntPropString(controlpoint.point.GetEntryIndex(), Prop_Data, "m_iName", name.data(), str_len, length);

			if (length == 0U)
			{
				continue; // no targetname set
			}

			std::size_t it = 0;

			auto functor = [&controlpoint, &name, &str_len, &it](int index, edict_t* edict, CBaseEntity* entity) {

				if (entity)
				{
					std::size_t length = 0;
					std::string assigned_point_name(str_len, '\0');
					entprops->GetEntPropString(index, Prop_Data, "m_iszCapPointName", assigned_point_name.data(), str_len, length);

					if (length == 0U)
					{
						return true; // skip this entity, no capture point assigned
					}

					if (it >= controlpoint.bomb_targets.max_size())
					{
						return false; // bomb array limit reached, end search
					}

					if (std::strcmp(name.c_str(), assigned_point_name.c_str()) == 0)
					{
#ifdef EXT_DEBUG
						META_CONPRINTF("Found dod_bomb_target #%i assigned to control point #%i <%s> [%zu]\n", index, controlpoint.point.GetEntryIndex(), name.c_str(), it);
#endif // EXT_DEBUG

						controlpoint.bomb_targets[it] = entity;
						it++;
						return true;
					}
				}

				return true;
			};

			UtilHelpers::ForEachEntityOfClassname("dod_bomb_target", functor);
		}
	}
}

void CDayOfDefeatSourceMod::OnBombEvent(const CDayOfDefeatSourceMod::BombEvent& event) const
{
	int clientindex = playerhelpers->GetClientOfUserId(event.userid);
	CBaseEntity* planter = nullptr;
	CBaseEntity* cp = nullptr;
	const CDayOfDefeatSourceMod::DoDControlPoint* pointdata = GetControlPointByIndex(event.cpindex);

	if (!pointdata)
	{
		return;
	}

	cp = pointdata->point.Get();

	if (!cp)
	{
		return;
	}

	Vector pos{ vec3_origin };

	if (clientindex != 0)
	{
		planter = gamehelpers->ReferenceToEntity(clientindex);
	}

	int team = event.team;
	bool isDefused = event.isDefused;

	auto func = [pos, planter, cp, team, isDefused](CBaseBot* bot) {
		if (isDefused)
		{
			bot->OnBombDefused(pos, team, planter, cp);
		}
		else
		{
			bot->OnBombPlanted(pos, team, planter, cp);
		}
	};

	extmanager->ForEachBot(func);
}

void CDayOfDefeatSourceMod::UpdateClassLimits()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CDayOfDefeatSourceMod::UpdateClassLimits", "NavBot");
#endif // EXT_VPROF_ENABLED

	ConVarRef mp_allowrandomclass{ "mp_allowrandomclass" };
	ConVarRef mp_limit_allies_assault{ "mp_limit_allies_assault" };
	ConVarRef mp_limit_allies_mg{ "mp_limit_allies_mg" };
	ConVarRef mp_limit_allies_rifleman{ "mp_limit_allies_rifleman" };
	ConVarRef mp_limit_allies_rocket{ "mp_limit_allies_rocket" };
	ConVarRef mp_limit_allies_sniper{ "mp_limit_allies_sniper" };
	ConVarRef mp_limit_allies_support{ "mp_limit_allies_support" };
	ConVarRef mp_limit_axis_assault{ "mp_limit_axis_assault" };
	ConVarRef mp_limit_axis_mg{ "mp_limit_axis_mg" };
	ConVarRef mp_limit_axis_rifleman{ "mp_limit_axis_rifleman" };
	ConVarRef mp_limit_axis_rocket{ "mp_limit_axis_rocket" };
	ConVarRef mp_limit_axis_sniper{ "mp_limit_axis_sniper" };
	ConVarRef mp_limit_axis_support{ "mp_limit_axis_support" };


	m_random_class_allowed = mp_allowrandomclass.GetBool();

	m_classlimits_allies.rifleman = mp_limit_allies_rifleman.GetInt();
	m_classlimits_allies.assault = mp_limit_allies_assault.GetInt();
	m_classlimits_allies.support = mp_limit_allies_support.GetInt();
	m_classlimits_allies.sniper = mp_limit_allies_sniper.GetInt();
	m_classlimits_allies.mg = mp_limit_allies_mg.GetInt();
	m_classlimits_allies.rocket = mp_limit_allies_rocket.GetInt();
	m_classlimits_axis.rifleman = mp_limit_axis_rifleman.GetInt();
	m_classlimits_axis.assault = mp_limit_axis_assault.GetInt();
	m_classlimits_axis.support = mp_limit_axis_support.GetInt();
	m_classlimits_axis.sniper = mp_limit_axis_sniper.GetInt();
	m_classlimits_axis.mg = mp_limit_axis_mg.GetInt();
	m_classlimits_axis.rocket = mp_limit_axis_rocket.GetInt();
}

CDayOfDefeatSourceMod::BombEvent::BombEvent(IGameEvent* event, bool isDefused)
{
	this->team = event->GetInt("team", NAV_TEAM_ANY);
	this->cpindex = event->GetInt("cp", -1);
	this->userid = event->GetInt("userid", -1);
	this->isDefused = isDefused;
}

void CDayOfDefeatSourceMod::RegisterModCommands()
{
	CServerCommandManager& manager = extmanager->GetServerCommandManager();

	auto dbg_control_point_cmd = [](const CConCommandArgs& args) {
		auto functor = [](int index, edict_t* edict, CBaseEntity* entity) {

			if (entity)
			{
				auto datamap = gamehelpers->GetDataMap(entity);
				SourceMod::sm_datatable_info_t info;

				if (gamehelpers->FindDataMapInfo(datamap, "m_iCPGroup", &info))
				{
					unsigned int offset = info.actual_offset - sizeof(int);
					int* pointindex = entprops->GetPointerToEntData<int>(entity, offset);
					std::string printname(512, '\0');
					std::size_t strlen = 0U;
					entprops->GetEntPropString(index, Prop_Data, "m_iszPrintName", printname.data(), 512, strlen);

					rootconsole->ConsolePrint("Control Point entity #%i (%s), point index #%i <%p>", index, printname.c_str(), *pointindex, pointindex);
				}
			}

			return true;
			};

		UtilHelpers::ForEachEntityOfClassname("dod_control_point", functor);
	};

	auto dbg_attackdefend_points = [](const CConCommandArgs& args) {
		CDayOfDefeatSourceMod* mod = CDayOfDefeatSourceMod::GetDODMod();

		std::vector<const CDayOfDefeatSourceMod::DoDControlPoint*> attack;
		std::vector<const CDayOfDefeatSourceMod::DoDControlPoint*> defend;
		attack.reserve(8);
		defend.reserve(8);

		dayofdefeatsource::DoDTeam team = dodslib::GetDoDTeam(UtilHelpers::EdictToBaseEntity(UtilHelpers::GetListenServerHost()));

		mod->CollectControlPointsToAttack(team, attack);
		mod->CollectControlPointsToDefend(team, defend);

		META_CONPRINTF("Control Points to Attack for %s \n", dodslib::GetDoDTeamName(team));

		for (auto point : attack)
		{
			META_CONPRINTF("  Attack: #%i \n", point->index);
		}

		META_CONPRINTF("Control Points to Defend for %s \n", dodslib::GetDoDTeamName(team));

		for (auto point : defend)
		{
			META_CONPRINTF("  Defend: #%i \n", point->index);
		}
	};

	manager.RegisterConCommand("sm_dod_navbot_debug_control_point_index", "Tests access to the CControlPoint::m_iPointIndex member variable.", FCVAR_GAMEDLL, dbg_control_point_cmd);
	manager.RegisterConCommand("sm_dod_navbot_debug_attackdefend_points", "List which control point your current team can attack and defend",
		FCVAR_GAMEDLL, dbg_attackdefend_points, CServerCommandManager::COMMAND_ONLY_ON_LISTEN_SERVERS);
}

SourceMod::SMCResult CDoDModSettings::ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value)
{
	if (std::strcmp(key, "dod_bomb_plant_respond_dist") == 0)
	{
		float dist = atof(value);
		dist = std::clamp(dist, 1000.0f, 10000.0f);
		SetMaxBombPlantedRespondDistance(dist);
		return SourceMod::SMCResult::SMCResult_Continue;
	}

	if (std::strcmp(key, "dod_bomb_max_defusers") == 0)
	{
		int num = atoi(value);
		num = std::clamp(num, 2, 16);
		SetMaxBombDefusers(num);
		return SourceMod::SMCResult::SMCResult_Continue;
	}

	return CModSettings::ReadSMC_KeyValue(states, key, value);
}
