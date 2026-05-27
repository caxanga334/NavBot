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

	static constexpr auto REPORTED_ENTITY_BECOME_OBSOLETE_AFTER = 90.0f; // Any entity information older than this is 'expired'.

	/**
	 * @brief Represents the data of an entity reported by a bot.
	 */
	class ReportedEntityData
	{
	public:
		ReportedEntityData(CBaseEntity* pEntity);

		bool operator==(const ReportedEntityData& other) const;
		bool operator!=(const ReportedEntityData& other) const;
		explicit operator bool() const { return IsValid(); }
		// Returns true if the entity handle dereferences into a valid entity.
		bool IsValid() const { return GetEntity() != nullptr; }
		// Returns true if this reported entity instance is obsolete.
		bool IsObsolete() const;
		// Gets the stored entity. Will return NULL if the entity no longer exists.
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
		/**
		 * @brief Returns true if this reported entity instance was recently updated within the given time.
		 * @param time Time limit to consider recently updated.
		 * @return True if the given time is equal or less to the time since last updated, false otherwise.
		 */
		bool WasRecentlyUpdatedWithin(float time) const { return GetTimeSinceLastUpdated() <= time; }
		// Returns the entity's classname.
		const std::string& GetClassname() const { return m_classname; }
		/**
		 * @brief Checks if this reported entity classname matches the given pattern.
		 * @param pattern Entity classname pattern.
		 * @return True if the pattern matches, false otherwise.
		 */
		bool ClassnameMatches(const char* pattern) const;
		/**
		 * @brief Sets the entity's cleared status.
		 * @param state State to set.
		 */
		void SetClearedStatus(bool state) { m_cleared = state; }
		// Returns true if this entity
		bool IsCleared() const { return m_cleared; }

	private:
		CHandle<CBaseEntity> m_handle;
		Vector m_lastknownposition;
		CNavArea* m_lastknownarea;
		float m_timecreated; // time stamp when this entity info was created
		float m_timeupdated; // time stamp when this entity info was last updated
		std::string m_classname; // entity classname
		bool m_cleared; // remembers if this entity was cleared in a combat search
	};

	// Called to reset the shared memory interface.
	virtual void Reset();
	// Called at intervals, prefer using this for expensive calls.
	virtual void Update();
	// Called every server frame, avoid doing expensive calls here.
	virtual void Frame();
	// Called when the round is restarted.
	virtual void OnRoundRestart() { Reset(); }

	/**
	 * @brief Reports an entity is currently visible. This will create or update an instance.
	 * @param entity Entity being reported.
	 * @return Pointer to the reported entity instance.
	 */
	ReportedEntityData* ReportEntityVisible(CBaseEntity* entity)
	{
		for (auto& red : m_reportedentitiesvec)
		{
			if (red.GetEntity() == entity)
			{
				red.Update();
				return &red;
			}
		}

		ReportedEntityData& red = m_reportedentitiesvec.emplace_back(entity);
		return &red;
	}
	/**
	 * @brief Searches for a reported entity instance of the given entity.
	 * @param entity Entity to search the instance of.
	 * @return Pointer to a reporte entity instance. NULL if none is found.
	 */
	const ReportedEntityData* GetReportedEntityInstance(CBaseEntity* entity) const
	{
		for (auto& red : m_reportedentitiesvec)
		{
			if (red && red.GetEntity() == entity)
			{
				return &red;
			}
		}

		return nullptr;
	}
	/**
	 * @brief Updates the reported entity instance of the given entity as cleared.
	 * @param entity Entity to update.
	 */
	void UpdateReportedEntityAsCleared(CBaseEntity* entity)
	{
		for (auto& red : m_reportedentitiesvec)
		{
			if (red && red.GetEntity() == entity)
			{
				red.SetClearedStatus(true);
				return;
			}
		}
	}
	/**
	 * @brief Removes an entity info entry of the given entity.
	 * @param entity Entity to remove info of.
	 */
	void ForgetEntity(CBaseEntity* entity)
	{
		m_reportedentitiesvec.erase(std::remove_if(std::begin(m_reportedentitiesvec), std::end(m_reportedentitiesvec), [&entity](const ReportedEntityData& obj) {
			if (obj)
			{
				return obj.GetEntity() == entity;
			}

			return false;
		}), std::end(m_reportedentitiesvec));
	}
	/**
	 * @brief Collects valid reported entity instances into a vector.
	 * @param vec Vector to store the collected instances.
	 * @return Number of instances collected
	 */
	std::size_t CollectReportedEntities(std::vector<const ISharedBotMemory::ReportedEntityData*>& vec) const
	{
		std::size_t c = 0;

		for (const ReportedEntityData& info : m_reportedentitiesvec)
		{
			if (!info.IsObsolete())
			{
				vec.push_back(&info);
				c++;
			}
		}

		return c;
	}
	// Returns the number of reported entities stored.
	std::size_t GetReportedEntitiesCount() const { return m_reportedentitiesvec.size(); }
	/**
	 * @brief Runs a function on every entity info stored.
	 * @tparam F a lambda expression or class with void (const ISharedBotMemory::ReportedEntityData& entinfo) function.
	 * @param func Function to run.
	 */
	template <typename F>
	void ForEveryReportedEntity(const F& func) const
	{
		for (const ReportedEntityData& info : m_reportedentitiesvec)
		{
			if (!info.IsObsolete())
			{
				func(info);
			}
		}
	}

	void IncrementDefendersCount() { m_defenders++; }
	void DecrementDefendersCount()
	{
		if (--m_defenders < 0)
		{
			m_defenders = 0;
		}
	}
	int GetDefendersCount() const { return m_defenders; }

protected:
	std::vector<ReportedEntityData>& GetEntityInfoStorageVector() { return m_reportedentitiesvec; }

	void UpdateReportedEntities()
	{
		// Remove obsolete instances.
		m_reportedentitiesvec.erase(std::remove_if(std::begin(m_reportedentitiesvec), std::end(m_reportedentitiesvec), [](const ReportedEntityData& obj) {
			return obj.IsObsolete();
		}), std::end(m_reportedentitiesvec));
	}

private:
	std::vector<ReportedEntityData> m_reportedentitiesvec; // Vector of entities reported by bots.
	int m_defenders; // number of bots doing defensive tasks

};

/**
 * @brief bsmu: Bot Shared Memory Utils.
 * 
 * Namespace for utility function and classes for the bot shared memory interface
 */
namespace bsmu
{
	/**
	 * @brief Notifies the shared bot memory interface that a bot is doing defensive tasks.
	 */
	template <typename Bot>
	class OnDefending
	{
	public:
		OnDefending() :
			botmem(nullptr)
		{
		}

		OnDefending(Bot* bot)
		{
			botmem = bot->GetSharedMemoryInterface();
			botmem->IncrementDefendersCount();
		}

		~OnDefending()
		{
			if (botmem)
			{
				botmem->DecrementDefendersCount();
			}
		}

		// Call this function once to notify the bot is defending
		void Notify(Bot* bot)
		{
			if (!botmem)
			{
				botmem = bot->GetSharedMemoryInterface();
				botmem->IncrementDefendersCount();
			}
		}

	private:
		ISharedBotMemory* botmem;
	};
}

#endif // !__NAVBOT_BOT_SHARED_MEMORY_INTERFACE_H_
