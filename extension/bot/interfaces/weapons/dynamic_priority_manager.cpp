#include NAVBOT_PCH_FILE
#include "dynamic_priority_manager.h"

CDynamicPriorityManager::CDynamicPriorityManager()
{
}

CDynamicPriorityManager::~CDynamicPriorityManager()
{
}

CDynamicPriorityManager& CDynamicPriorityManager::GetManager()
{
	static CDynamicPriorityManager instance;
	return instance;
}

bool CDynamicPriorityManager::RegisterFactory(const std::string& name, Factory* factory)
{
	// no duplicates
	if (factory_map.find(name) != factory_map.end())
	{
		delete factory;
		return false;
	}

	auto i = factory_map.emplace(name, std::unique_ptr<Factory>(factory));
	return i.second;
}

const CDynamicPriorityManager::Factory* CDynamicPriorityManager::FindFactory(const std::string& name) const
{
	auto it = factory_map.find(name);

	if (it == factory_map.cend())
	{
		return nullptr;
	}

	return it->second.get();
}

/* standard factories */

class CHealthFactory : public CDynamicPriorityManager::Factory
{
	// Inherited via Factory
	IDynamicWeaponPriority* Create() const override
	{
		return new CDynamicPriorityHealth;
	}
};

class CEnemyRangeFactory : public CDynamicPriorityManager::Factory
{
	// Inherited via Factory
	IDynamicWeaponPriority* Create() const override
	{
		return new CDynamicPriorityEnemyRange;
	}
};

class CHasSecondaryAmmoFactory : public CDynamicPriorityManager::Factory
{
	// Inherited via Factory
	IDynamicWeaponPriority* Create() const override
	{
		return new CDynamicPriorityHasSecondaryAmmo;
	}
};

class CBotAggressionFactory : public CDynamicPriorityManager::Factory
{
	// Inherited via Factory
	IDynamicWeaponPriority* Create() const override
	{
		return new CDynamicPriotityBotAggression;
	}
};

class CEnemyClassnameMatchesFactory : public CDynamicPriorityManager::Factory
{
	// Inherited via Factory
	IDynamicWeaponPriority* Create() const override
	{
		return new CDynamicPriotityEnemyClassnameMatches;
	}
};

void CDynamicPriorityManager::CreateStandardFactories()
{
	CDynamicPriorityManager& manager = CDynamicPriorityManager::GetManager();
	manager.RegisterFactory("health", new CHealthFactory);
	manager.RegisterFactory("range", new CEnemyRangeFactory);
	manager.RegisterFactory("secondary_ammo", new CHasSecondaryAmmoFactory);
	manager.RegisterFactory("aggression", new CBotAggressionFactory);
	manager.RegisterFactory("classname", new CEnemyClassnameMatchesFactory);
}