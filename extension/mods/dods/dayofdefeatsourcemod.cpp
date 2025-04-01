#include <cstring>

#include <extension.h>
#include <bot/dods/dodsbot.h>
#include <util/entprops.h>
#include <util/helpers.h>
#include "dayofdefeatsourcemod.h"

void CDODObjectiveResource::Init(CBaseEntity* entity)
{
	m_iNumControlPoints = entprops->GetPointerToEntData<int>(entity, Prop_Send, "m_iNumControlPoints");
	m_bCPIsVisible = entprops->GetPointerToEntData<bool>(entity, Prop_Send, "m_bCPIsVisible");
	m_iAlliesReqCappers = entprops->GetPointerToEntData<int>(entity, Prop_Send, "m_iAlliesReqCappers");
	m_iAxisReqCappers = entprops->GetPointerToEntData<int>(entity, Prop_Send, "m_iAxisReqCappers");
	m_bBombPlanted = entprops->GetPointerToEntData<bool>(entity, Prop_Send, "m_bBombPlanted");
	m_iBombsRequired = entprops->GetPointerToEntData<int>(entity, Prop_Send, "m_iBombsRequired");
	m_iBombsRemaining = entprops->GetPointerToEntData<int>(entity, Prop_Send, "m_iBombsRemaining");
	m_bBombBeingDefused = entprops->GetPointerToEntData<bool>(entity, Prop_Send, "m_bBombBeingDefused");
	m_iNumAllies = entprops->GetPointerToEntData<int>(entity, Prop_Send, "m_iNumAllies");
	m_iNumAxis = entprops->GetPointerToEntData<int>(entity, Prop_Send, "m_iNumAxis");
	m_iCappingTeam = entprops->GetPointerToEntData<int>(entity, Prop_Send, "m_iCappingTeam");
	m_iOwner = entprops->GetPointerToEntData<int>(entity, Prop_Send, "m_iOwner");
}

CDayOfDefeatSourceMod::CDayOfDefeatSourceMod() :
	CBaseMod(), m_objectiveres()
{
	ListenForGameEvent("dod_round_start");
}

CDayOfDefeatSourceMod::~CDayOfDefeatSourceMod()
{
}

void CDayOfDefeatSourceMod::FireGameEvent(IGameEvent* event)
{
	if (event)
	{
		if (std::strcmp("dod_round_start", event->GetName()) == 0)
		{
			OnRoundStart();
		}
	}

	CBaseMod::FireGameEvent(event);
}

CBaseBot* CDayOfDefeatSourceMod::AllocateBot(edict_t* edict)
{
	return new CDoDSBot(edict);
}

void CDayOfDefeatSourceMod::OnRoundStart()
{
	FindObjectiveResourceEntity();
}

const CDODObjectiveResource* CDayOfDefeatSourceMod::GetDODObjectiveResource() const
{
	if (m_objectiveentity.Get() != nullptr)
	{
		return &m_objectiveres;
	}

	return nullptr;
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

#ifdef EXT_DEBUG

CON_COMMAND(sm_dod_navbot_debug_control_point_index, "Tests access to the CControlPoint::m_iPointIndex member variable.")
{
	UtilHelpers::ForEachEntityOfClassname("dod_control_point", [](int index, edict_t* edict, CBaseEntity* entity) {

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
	});
}

#endif // EXT_DEBUG

