#ifndef __NAVBOT_BOT_SHARED_MEMORY_INTERFACE_H_
#define __NAVBOT_BOT_SHARED_MEMORY_INTERFACE_H_

class CBaseEntity;
class CBaseBot;
class CNavArea;

#include <sdkports/sdk_ehandle.h>

/**
 * @brief Interface for storing shared information between all bots.
 */
class ISharedBotMemory
{
public:
	ISharedBotMemory();
	virtual ~ISharedBotMemory();

	static constexpr auto ENTITY_INFO_EXPIRE_TIME = 60.0f; // Any entity information older than this is 'expired'.

	class EntityInfo
	{
	public:
		EntityInfo(CBaseEntity* pEntity);

		bool operator==(const EntityInfo& other) const;
		bool operator!=(const EntityInfo& other) const;

		bool IsObsolete();
		CBaseEntity* GetEntity() const { return m_handle.Get(); }
		void Update();
		// Entity's last known position
		const Vector& GetLastKnownPosition() const { return m_lastknownposition; }
		// Entity's last known nav area
		const CNavArea* GetLastKnownNavArea() const { return m_lastknownarea; }
		// Time in seconds since this entity info instance was created.
		float GetTimeSinceCreation() const;
		// Time in seconds since this entity info instance was last updated.
		float GetTimeSinceLastUpdated() const;
		const std::string& GetClassname() const { return m_classname; }
		bool ClassnameMatches(const char* pattern) const;

	private:
		CHandle<CBaseEntity> m_handle;
		Vector m_lastknownposition;
		CNavArea* m_lastknownarea;
		float m_timecreated; // time stamp when this entity info was created
		float m_timeupdated; // time stamp when this entity info was last updated
		std::string m_classname;
	};

	virtual void Reset();
	virtual void Update();
	virtual void Frame();
	virtual void OnRoundRestart();

	/**
	 * @brief Registers or updates an entity info.
	 * @param entity Entity to be stored.
	 * @return Pointer to entity info.
	 */
	const EntityInfo* AddEntityInfo(CBaseEntity* entity);
	/**
	 * @brief Gets an entity info instance.
	 * @param entity Entity to get the entity info of.
	 * @return Entity info instance or NULL if not registered.
	 */
	const EntityInfo* GetEntityInfo(CBaseEntity* entity) const;
	/**
	 * @brief Collects entity infos into a vector. Obsolete infos are excluded.
	 * @param out Vector to store the entity infos
	 */
	void CollectEntityInfos(std::vector<const EntityInfo*>& out)
	{
		for (EntityInfo& info : m_ents)
		{
			if (!info.IsObsolete())
			{
				out.push_back(&info);
			}
		}
	}

protected:
	std::vector<EntityInfo>* GetEntityInfoStorageVector() { return &m_ents; }

	void PurgeInvalidEntityInfos();

private:
	std::vector<EntityInfo> m_ents; // entity info stoarge


};

/**
 * @brief bsmu: Bot Shared Memory Utils.
 * 
 * Namespace for utility function and classes for the bot shared memory interface
 */
namespace bsmu
{

}

#endif // !__NAVBOT_BOT_SHARED_MEMORY_INTERFACE_H_
