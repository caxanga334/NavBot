#ifndef __NAVBOT_SDKPORTS_ENTITY_FACTORY_H_
#define __NAVBOT_SDKPORTS_ENTITY_FACTORY_H_

#include <utldict.h>

class IServerNetworkable;
class IEntityFactory;

// This is the glue that hooks .MAP entity class names to our CPP classes
class IEntityFactoryDictionary
{
public:
	virtual void InstallFactory(IEntityFactory * pFactory, const char* pClassName) = 0;
	virtual IServerNetworkable* Create(const char* pClassName) = 0;
	virtual void Destroy(const char* pClassName, IServerNetworkable* pNetworkable) = 0;
	virtual IEntityFactory* FindFactory(const char* pClassName) = 0;
	virtual const char* GetCannonicalName(const char* pClassName) = 0;
};

/*
IEntityFactoryDictionary* EntityFactoryDictionary();

inline bool CanCreateEntityClass(const char* pszClassname)
{
	return (EntityFactoryDictionary() != nullptr && EntityFactoryDictionary()->FindFactory(pszClassname) != nullptr);
}
*/

class IEntityFactory
{
public:
	virtual IServerNetworkable * Create(const char* pClassName) = 0;
	virtual void Destroy(IServerNetworkable* pNetworkable) = 0;
	virtual size_t GetEntitySize() = 0;
};

template <class T>
class CEntityFactory : public IEntityFactory
{
public:
	CEntityFactory(const char* pClassName)
	{
		EntityFactoryDictionary()->InstallFactory(this, pClassName);
	}

	IServerNetworkable* Create(const char* pClassName)
	{
		T* pEnt = _CreateEntityTemplate((T*)NULL, pClassName);
		return pEnt->NetworkProp();
	}

	void Destroy(IServerNetworkable* pNetworkable)
	{
		if (pNetworkable)
		{
			pNetworkable->Release();
		}
	}

	virtual size_t GetEntitySize()
	{
		return sizeof(T);
	}
};

class CEntityFactoryDictionary : public IEntityFactoryDictionary
{
public:
	CEntityFactoryDictionary() {}

	/*
	virtual void InstallFactory(IEntityFactory* pFactory, const char* pClassName);
	virtual IServerNetworkable* Create(const char* pClassName);
	virtual void Destroy(const char* pClassName, IServerNetworkable* pNetworkable);
	virtual const char* GetCannonicalName(const char* pClassName);
	*/
	void ReportEntitySizes() {}

private:
	IEntityFactory* FindFactory(const char* pClassName) { return nullptr; }
public:
	CUtlDict< IEntityFactory*, unsigned short > m_Factories;
};

#endif