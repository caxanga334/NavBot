#ifndef __NAVBOT_INTERFACE_WEAPONS_DYNAMIC_PRIORITY_MANAGER_H_
#define __NAVBOT_INTERFACE_WEAPONS_DYNAMIC_PRIORITY_MANAGER_H_

#include "dynamic_priorities.h"

class CDynamicPriorityManager
{
public:
	CDynamicPriorityManager();
	~CDynamicPriorityManager();

	// creates IDynamicWeaponPriority instances
	class Factory
	{
	public:
		virtual ~Factory() {}

		// creates a new IDynamicWeaponPriority instance.
		virtual IDynamicWeaponPriority* Create() const = 0;
	};

	// Manager access singleton
	static CDynamicPriorityManager& GetManager();
	static void CreateStandardFactories();

	/**
	 * @brief Registers a new factory.
	 * Memory is managed by the manager.
	 * @param name Name of the factory.
	 * @param factory Pointer to the factory. (Never delete this, the manager is responsible for freeing memory).
	 * @return True if the factory was registered, false otherwise.
	 */
	bool RegisterFactory(const std::string& name, Factory* factory);
	// Finds a named factory.
	const Factory* FindFactory(const std::string& name) const;
	// Removes a factory
	void RemoveFactory(const std::string& name)
	{
		auto it = factory_map.find(name);

		if (it != factory_map.end())
		{
			factory_map.erase(it);
		}
	}

	void FindAndRemoveFactory(Factory* factory)
	{
		if (!factory) { return; }

		for (auto it = factory_map.begin(); it != factory_map.end(); it++)
		{
			if (it->second.get() == factory)
			{
				factory_map.erase(it);
				return;
			}
		}
	}

	// Notify the extension is shutting down
	void OnShutdown()
	{
		factory_map.clear();
	}

private:
	std::unordered_map<std::string, std::unique_ptr<Factory>> factory_map;
};


#endif // !__NAVBOT_INTERFACE_WEAPONS_DYNAMIC_PRIORITY_MANAGER_H_
