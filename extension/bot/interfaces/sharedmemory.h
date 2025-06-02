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

		bool operator==(const EntityInfo& other);
		bool operator!=(const EntityInfo& other);

		bool IsObsolete();

		void Update();
		// Entity's last known position
		const Vector& GetLastKnownPosition() const { return m_lastknownposition; }
		// Entity's last known nav area
		const CNavArea* GetLastKnownNavArea() const { return m_lastknownarea; }
		// Time in seconds since this entity info instance was created.
		float GetTimeSinceCreation() const;
		// Time in seconds since this entity info instance was last updated.
		float GetTimeSinceLastUpdated() const;

	private:
		CHandle<CBaseEntity> m_handle;
		Vector m_lastknownposition;
		CNavArea* m_lastknownarea;
		float m_timecreated; // time stamp when this entity info was created
		float m_timeupdated; // time stamp when this entity info was last updated
	};

	virtual void Reset();
	virtual void Update();
	virtual void Frame();
	virtual void OnRoundRestart();

protected:

private:
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
