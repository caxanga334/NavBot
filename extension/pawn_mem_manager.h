#ifndef NAVBOT_SOURCEPAWN_MEMORY_MANAGER_H_
#define NAVBOT_SOURCEPAWN_MEMORY_MANAGER_H_

/**
* Until Sourcepawn/Sourcemod gets better support for 64 bits pointer address.
* We will use indexes for manaing pawn allocated objects.
*/

#include <memory>
#include <unordered_map>
#include <sp_vm_types.h>
#include <bot/interfaces/path/pluginnavigator.h>

class CSourcePawnMemoryManager
{
public:
	CSourcePawnMemoryManager()
	{
		m_navigatorIndex = 1;
	}

	~CSourcePawnMemoryManager()
	{
	}

	void OnMapEnd();

	// Creates a new plugin navigator, return it's storage index
	cell_t AllocNewNavigator();

	CPluginMeshNavigator* GetNavigatorByIndex(cell_t index);
	void DeleteNavigator(cell_t index);

private:
	std::unordered_map<cell_t, std::unique_ptr<CPluginMeshNavigator>> m_pluginnavigators;
	cell_t m_navigatorIndex;
};


#endif // !NAVBOT_SOURCEPAWN_MEMORY_MANAGER_H_
