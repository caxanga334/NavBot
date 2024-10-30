#ifndef _NAV_WAYPOINT_H_
#define _NAV_WAYPOINT_H_

#include <vector>
#include <array>
#include <variant>
#include <sdkports/sdk_ehandle.h>
#include "nav.h"

class CBaseBot;

struct WaypointConnect
{
	WaypointConnect()
	{
		connection = static_cast<WaypointID>(0);
	}

	WaypointConnect(CWaypoint* other)
	{
		connection = other;
	}

	bool operator==(const WaypointConnect& other)
	{
		if (std::holds_alternative<CWaypoint*>(connection) == false)
		{
			return false;
		}

		if (std::holds_alternative<CWaypoint*>(other.connection) == false)
		{
			return false;
		}

		return std::get<CWaypoint*>(connection) == std::get<CWaypoint*>(other.connection);
	}

	// Gets the connected waypoint. Warning: May throw
	CWaypoint* GetOther() const
	{
		return std::get<CWaypoint*>(connection);
	}

	// Converts the stored ID into a pointer
	void ConvertFromIDtoPointer();

	std::variant<WaypointID, CWaypoint*> connection;
};

class CWaypoint
{
public:
	CWaypoint();
	virtual ~CWaypoint();

	static constexpr auto WAYPOINT_HEIGHT = 72.0f;
	static constexpr auto WAYPOINT_AIM_HEIGHT = 64.0f;
	static constexpr auto WAYPOINT_TEXT_HEIGHT = 50.0f;
	static constexpr auto WAYPOINT_AIM_LENGTH = 32.0f;
	static constexpr auto WAYPOINT_CONNECT_HEIGHT = 58.0f;
	static constexpr std::size_t MAX_AIM_ANGLES = 4U;
	static constexpr auto UPDATE_INTERVAL = 0.5f;
	static constexpr auto WAYPOINT_EDIT_DRAW_RANGE = 1024.0f;
	static constexpr auto WAYPOINT_DELETE_SEARCH_DIST = 256.0f;
	static constexpr auto WAYPOINT_DEFAULT_RADIUS = 0.0f;
	static WaypointID g_NextWaypointID;

	enum BaseFlags : int
	{
		BASEFLAGS_NONE = 0
	};

	bool operator==(const CWaypoint& other)
	{
		return this->m_ID == other.m_ID;
	}

	virtual void Reset();
	virtual void Update();
	virtual bool IsEnabled() const { return true; }
	virtual bool CanBeUsedByBot(CBaseBot* bot) const;

	virtual void Save(std::fstream& filestream, uint32_t version);
	virtual NavErrorType Load(std::fstream& filestream, uint32_t version, uint32_t subVersion);
	virtual NavErrorType PostLoad();

	// Draws this waypoint during editing
	virtual void Draw() const;

	void Use(CBaseBot* user, const float duration = 10.0f) const;
	bool IsBeingUsed() const;
	void StopUsing(CBaseBot* user) const;

	WaypointID GetID() const { return m_ID; }
	const Vector& GetOrigin() const { return m_origin; }
	void SetOrigin(const Vector& origin) { m_origin = origin; }
	// Gets a random position within the waypoint's radius.
	Vector GetRandomPoint() const;
	/**
	 * @brief Gets a stored angle from the waypoint
	 * @param index Index to the angle array, from 0 to MAX_AIM_ANGLES - 1
	 * @return Reference to angle.
	 */
	const QAngle& GetAngle(std::size_t index) const;

	std::size_t GetNumOfAvailableAngles() const { return static_cast<std::size_t>(m_numAimAngles); }

	void AddAngle(QAngle& angle);
	void SetAngle(QAngle& angle, std::size_t index);
	void ClearAngles();

	/**
	 * @brief Runs a function on every available aim angle of this waypoint.
	 * @tparam T Function to run. void (const QAngle& angle)
	 * @param func Function to run.
	 */
	template <typename T>
	void ForEveryAngle(T func)
	{
		for (std::size_t i = 0U; i < GetNumOfAvailableAngles(); i++)
		{
			const QAngle& angle = m_aimAngles[i];
			func(angle);
		}
	}

	void SetFlags(BaseFlags val)
	{
		m_flags |= val;
	}

	void ClearFlags(BaseFlags val)
	{
		m_flags &= ~val;
	}

	bool HasFlags(BaseFlags val) const
	{
		return (m_flags & val) ? true : false;
	}

	void SetTeam(int team)
	{
		m_teamNum = team;
	}

	int GetTeam() const { return m_teamNum; }

	void SetRadius(float val) { m_radius = val; }
	float GetRadius() const { return m_radius; }

	void ConnectTo(const CWaypoint* other);
	void DisconnectFrom(const CWaypoint* other);
	bool IsConnectedTo(const CWaypoint* other) const;
	bool IsConnectedTo(WaypointID id) const;

	void NotifyWaypointDestruction(const CWaypoint* other);

	const std::vector<WaypointConnect>& GetConnections() const { return m_connections; }

	virtual void PrintInfo();

protected:
	friend class CNavMesh;

	WaypointID m_ID; // Waypoint ID
	Vector m_origin; // Waypoint origin
	std::array<QAngle, MAX_AIM_ANGLES> m_aimAngles; // Waypoint aim angles
	int m_numAimAngles; // Number of aim angles this waypoint has
	int m_flags; // Waypoint flags (base)
	int m_teamNum; // Waypoint team
	float m_radius; // Waypoint radius
	mutable CHandle<CBaseEntity> m_user; // Entity of the bot using this waypoint
	mutable CountdownTimer m_expireUserTimer; // timer to expire the user
	std::vector<WaypointConnect> m_connections; // Waypoint connections

	// Called when a bot 'uses' this waypoint
	virtual void OnUse(CBaseBot* user) const {}
	/**
	 * @brief Called when a bot stops using this waypoint.
	 * @param user Bot that release this waypoint. NULL if it was an automatic release.
	 */
	virtual void OnStopUse(CBaseBot* user) const {}

	// Additional mod specific waypoint draw text
	virtual void DrawModText(char* text, std::size_t size) const {}
};

extern ConVar sm_nav_waypoint_edit;

#endif