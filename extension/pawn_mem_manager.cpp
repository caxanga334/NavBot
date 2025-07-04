#include NAVBOT_PCH_FILE
#include <extension.h>
#include "pawn_mem_manager.h"

void CSourcePawnMemoryManager::OnMapEnd()
{
	m_navigatorIndex = 1;
	m_pluginnavigators.clear();
}

cell_t CSourcePawnMemoryManager::AllocNewNavigator()
{
	std::unique_ptr<CPluginMeshNavigator> nav = std::make_unique<CPluginMeshNavigator>();

	cell_t index = m_navigatorIndex;
	m_pluginnavigators[index] = std::move(nav);
	m_navigatorIndex++;

	return index;
}

CPluginMeshNavigator* CSourcePawnMemoryManager::GetNavigatorByIndex(cell_t index)
{
	auto it = m_pluginnavigators.find(index);

	if (it == m_pluginnavigators.end())
	{
		return nullptr;
	}

	return it->second.get();
}

void CSourcePawnMemoryManager::DeleteNavigator(cell_t index)
{
	m_pluginnavigators.erase(index);
}
