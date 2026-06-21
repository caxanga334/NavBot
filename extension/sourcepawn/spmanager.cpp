#include NAVBOT_PCH_FILE
#include <any>
#include <bot/basebot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <bot/interfaces/weapons/dynamic_priority_manager.h>
#include <navmesh/nav_pathfind.h>
#include <natives/navcollector.h>
#include <natives/navblocker.h>
#include "spmanager.h"


std::unique_ptr<SourcePawnManager> spmanager;

SourcePawnManager::SourcePawnManager()
{
	std::fill(std::begin(m_handletypes), std::end(m_handletypes), BAD_HANDLE);
}

SourcePawnManager::~SourcePawnManager()
{
}

void SourcePawnManager::Init()
{
	spmanager = std::make_unique<SourcePawnManager>();
}

void SourcePawnManager::OnHandleDestroy(SourceMod::HandleType_t type, void* object)
{
	if (type == m_handletypes[HANDLE_NAVIGATOR])
	{
		CMeshNavigator* nav = reinterpret_cast<CMeshNavigator*>(object);
		delete nav;
		return;
	}

	if (type == m_handletypes[HANDLE_WEAPONPRIOFACTORY])
	{
		CDynamicPriorityManager::Factory* obj = reinterpret_cast<CDynamicPriorityManager::Factory*>(object);
		CDynamicPriorityManager& manager = CDynamicPriorityManager::GetManager();
		manager.FindAndRemoveFactory(obj);
		return;
	}

	if (type == m_handletypes[HANDLE_NAVCOLLECTOR])
	{
		natives::navmesh::collector::destroy(object);
		return;
	}

	if (type == m_handletypes[HANDLE_NAVAREAVECTOR])
	{
		std::vector<CNavArea*>* vec = reinterpret_cast<std::vector<CNavArea*>*>(object);
		delete vec;
		return;
	}

	if (type == m_handletypes[HANDLE_NAVBLOCKER])
	{
		natives::navmesh::navblocker::onhandledeleted(object);
		return;
	}
}

bool SourcePawnManager::GetHandleApproxSize(SourceMod::HandleType_t type, void* object, unsigned int* pSize)
{
	if (type == m_handletypes[HANDLE_NAVIGATOR])
	{
		*pSize = static_cast<unsigned int>(sizeof(CMeshNavigator)) + static_cast<unsigned int>(sizeof(BotPathSegment) * 128);

		return true;
	}

	if (type == m_handletypes[HANDLE_WEAPONPRIOFACTORY])
	{
		*pSize = static_cast<unsigned int>(sizeof(CDynamicPriorityManager::Factory)) + static_cast<unsigned int>(sizeof(std::any) * 8);
		return true;
	}

	if (type == m_handletypes[HANDLE_WEAPONPRIOINSTANCE])
	{
		*pSize = sizeof(void*);
		return true;
	}

	if (type == m_handletypes[HANDLE_NAVCOLLECTOR])
	{
		*pSize = static_cast<unsigned int>(sizeof(INavAreaCollector<CNavArea>)) + static_cast<unsigned int>(sizeof(CNavArea) * 512);
		return true;
	}

	if (type == m_handletypes[HANDLE_NAVAREAVECTOR])
	{
		*pSize = static_cast<unsigned int>(sizeof(std::vector<CNavArea>)) + static_cast<unsigned int>(sizeof(CNavArea) * 512);
		return true;
	}

	return false;
}

void SourcePawnManager::SetupHandles()
{
	SetupNavigatorHandle();
	SetupWeaponDynPrioFactoryHandle();
	SetupWeaponDynPrioInstanceHandle();
	SetupNavCollectorHandle();
	SetupNavAreaVectorHandle();
}

void SourcePawnManager::SetupNavigatorHandle()
{
	using namespace SourceMod;
	// CMeshNavigator

	TypeAccess trules;
	HandleAccess hrules;
	handlesys->InitAccessDefaults(&trules, &hrules);

	trules.access[HTypeAccess_Create] = true;
	trules.access[HTypeAccess_Inherit] = true;
	trules.ident = myself->GetIdentity();

	m_handletypes[HANDLE_NAVIGATOR] = handlesys->CreateType("CMeshNavigator", this, NO_HANDLE_TYPE, &trules, &hrules, myself->GetIdentity(), nullptr);

	if (m_handletypes[HANDLE_NAVIGATOR] == NO_HANDLE_TYPE)
	{
		smutils->LogError(myself, "Failed to setup CMeshNavigator handle!");
	}
}

void SourcePawnManager::SetupWeaponDynPrioFactoryHandle()
{
	using namespace SourceMod;
	// CSourcePawnDynamicFactory

	TypeAccess trules;
	HandleAccess hrules;
	handlesys->InitAccessDefaults(&trules, &hrules);

	trules.access[HTypeAccess_Create] = true;
	trules.access[HTypeAccess_Inherit] = true;
	trules.ident = myself->GetIdentity();

	m_handletypes[HANDLE_WEAPONPRIOFACTORY] = handlesys->CreateType("CSourcePawnDynamicFactory", this, NO_HANDLE_TYPE, &trules, &hrules, myself->GetIdentity(), nullptr);

	if (m_handletypes[HANDLE_WEAPONPRIOFACTORY] == NO_HANDLE_TYPE)
	{
		smutils->LogError(myself, "Failed to setup CSourcePawnDynamicFactory handle!");
	}
}

void SourcePawnManager::SetupWeaponDynPrioInstanceHandle()
{
	using namespace SourceMod;
	// IDynamicWeaponPriority

	TypeAccess trules;
	HandleAccess hrules;
	handlesys->InitAccessDefaults(&trules, &hrules);

	trules.access[HTypeAccess_Create] = true;
	trules.access[HTypeAccess_Inherit] = true;
	trules.ident = myself->GetIdentity();
	hrules.access[HandleAccess_Read] = 0;
	hrules.access[HandleAccess_Delete] = HANDLE_RESTRICT_IDENTITY | HANDLE_RESTRICT_OWNER;
	hrules.access[HandleAccess_Clone] = HANDLE_RESTRICT_IDENTITY | HANDLE_RESTRICT_OWNER;

	m_handletypes[HANDLE_WEAPONPRIOINSTANCE] = handlesys->CreateType("IDynamicWeaponPriority", this, NO_HANDLE_TYPE, &trules, &hrules, myself->GetIdentity(), nullptr);

	if (m_handletypes[HANDLE_WEAPONPRIOINSTANCE] == NO_HANDLE_TYPE)
	{
		smutils->LogError(myself, "Failed to setup IDynamicWeaponPriority handle!");
	}
}

void SourcePawnManager::SetupNavCollectorHandle()
{
	using namespace SourceMod;

	TypeAccess trules;
	HandleAccess hrules;
	handlesys->InitAccessDefaults(&trules, &hrules);

	trules.access[HTypeAccess_Create] = true;
	trules.access[HTypeAccess_Inherit] = true;
	trules.ident = myself->GetIdentity();
	hrules.access[HandleAccess_Read] = 0;

	m_handletypes[HANDLE_NAVCOLLECTOR] = handlesys->CreateType("CSourcePawnAreaCollector", this, NO_HANDLE_TYPE, &trules, &hrules, myself->GetIdentity(), nullptr);

	if (m_handletypes[HANDLE_NAVCOLLECTOR] == NO_HANDLE_TYPE)
	{
		smutils->LogError(myself, "Failed to setup CSourcePawnAreaCollector handle!");
	}
}

void SourcePawnManager::SetupNavAreaVectorHandle()
{
	using namespace SourceMod;

	TypeAccess trules;
	HandleAccess hrules;
	handlesys->InitAccessDefaults(&trules, &hrules);

	trules.access[HTypeAccess_Create] = true;
	trules.access[HTypeAccess_Inherit] = true;
	trules.ident = myself->GetIdentity();
	hrules.access[HandleAccess_Read] = 0;

	m_handletypes[HANDLE_NAVAREAVECTOR] = handlesys->CreateType("std::vector<CNavArea>", this, NO_HANDLE_TYPE, &trules, &hrules, myself->GetIdentity(), nullptr);

	if (m_handletypes[HANDLE_NAVAREAVECTOR] == NO_HANDLE_TYPE)
	{
		smutils->LogError(myself, "Failed to setup std::vector<CNavArea> handle!");
	}
}

void SourcePawnManager::SetupNavBlockerHandle()
{
	using namespace SourceMod;

	TypeAccess trules;
	HandleAccess hrules;
	handlesys->InitAccessDefaults(&trules, &hrules);

	trules.access[HTypeAccess_Create] = true;
	trules.access[HTypeAccess_Inherit] = true;
	trules.ident = myself->GetIdentity();
	hrules.access[HandleAccess_Read] = 0;
	hrules.access[HandleAccess_Clone] = HANDLE_RESTRICT_OWNER;

	m_handletypes[HANDLE_NAVBLOCKER] = handlesys->CreateType("CSourcePawnNavBlocker", this, NO_HANDLE_TYPE, &trules, &hrules, myself->GetIdentity(), nullptr);

	if (m_handletypes[HANDLE_NAVBLOCKER] == NO_HANDLE_TYPE)
	{
		smutils->LogError(myself, "Failed to setup CSourcePawnNavBlocker handle!");
	}
}

