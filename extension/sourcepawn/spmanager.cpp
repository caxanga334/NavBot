#include NAVBOT_PCH_FILE
#include "extension.h"
#include <bot/basebot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include "spmanager.h"


std::unique_ptr<SourcePawnManager> spmanager;

SourcePawnManager::SourcePawnManager()
{
	m_meshnavigator_type = NO_HANDLE_TYPE;
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
	if (type == m_meshnavigator_type)
	{
		CMeshNavigator* nav = reinterpret_cast<CMeshNavigator*>(object);
		delete nav;
	}
}

bool SourcePawnManager::GetHandleApproxSize(SourceMod::HandleType_t type, void* object, unsigned int* pSize)
{
	if (type == m_meshnavigator_type)
	{
		*pSize = static_cast<unsigned int>(sizeof(CMeshNavigator)) + static_cast<unsigned int>(sizeof(CBasePathSegment) * 128);

		return true;
	}

	return false;
}

void SourcePawnManager::SetupHandles()
{
	using namespace SourceMod;

	{
		// CMeshNavigator

		TypeAccess trules;
		HandleAccess hrules;
		handlesys->InitAccessDefaults(&trules, &hrules);

		trules.access[HTypeAccess_Create] = true;
		trules.access[HTypeAccess_Inherit] = true;
		trules.ident = myself->GetIdentity();

		m_meshnavigator_type = handlesys->CreateType("CMeshNavigator", this, NO_HANDLE_TYPE, &trules, &hrules, myself->GetIdentity(), nullptr);

		if (m_meshnavigator_type == NO_HANDLE_TYPE)
		{
			smutils->LogError(myself, "Failed to setup CMeshNavigator handle!");
		}
	}

	
}
