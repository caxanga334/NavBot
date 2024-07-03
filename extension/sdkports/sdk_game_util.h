#ifndef SDKPORTS_GAME_UTIL_H_
#define SDKPORTS_GAME_UTIL_H_

#include <eiface.h>

// Ported from SDK2013 game/server/util.h

abstract_class IEntityFactory
{
public:
	virtual IServerNetworkable * Create(const char* pClassName) = 0;
	virtual void Destroy(IServerNetworkable* pNetworkable) = 0;
	virtual size_t GetEntitySize() = 0;
};

abstract_class IEntityFactoryDictionary
{
public:
	virtual void InstallFactory(IEntityFactory * pFactory, const char* pClassName) = 0;
	virtual IServerNetworkable* Create(const char* pClassName) = 0;
	virtual void Destroy(const char* pClassName, IServerNetworkable* pNetworkable) = 0;
	virtual IEntityFactory* FindFactory(const char* pClassName) = 0;
	virtual const char* GetCannonicalName(const char* pClassName) = 0;
};

#endif // !SDKPORTS_GAME_UTIL_H_
