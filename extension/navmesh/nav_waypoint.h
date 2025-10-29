#ifndef _NAV_WAYPOINT_H_
#define _NAV_WAYPOINT_H_

#include <cstring>
#include <vector>
#include <array>
#include <variant>
#include <optional>
#include <sdkports/sdk_ehandle.h>
#include "nav.h"

class CBaseBot;
class CRCBot2Waypoint;

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

	static bool IsEditing();

	static constexpr auto WAYPOINT_HEIGHT = 72.0f;
	static constexpr auto WAYPOINT_AIM_HEIGHT = 64.0f;
	static constexpr auto WAYPOINT_TEXT_HEIGHT = 50.0f;
	static constexpr auto WAYPOINT_MOD_TEXT_HEIGHT = WAYPOINT_TEXT_HEIGHT - 4.0f;
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
		BASEFLAGS_NONE = 0, // No flags
		BASEFLAGS_DEFEND = 0x00000001, // This is a good spot to defend a position from
		BASEFLAGS_SNIPER = 0x00000002, // This is a good spot to snipe from
		BASEFLAGS_ROAM = 0x00000004, // This is a destination point for roaming bots
		BASEFLAGS_CROUCH = 0x00000008, // Bots should crouch when using this waypoint
	};

	inline static BaseFlags StringToBaseFlags(const char* szFlags)
	{
		if (std::strcmp(szFlags, "defend") == 0)
		{
			return BASEFLAGS_DEFEND;
		}
		else if (std::strcmp(szFlags, "sniper") == 0)
		{
			return BASEFLAGS_SNIPER;
		}
		else if (std::strcmp(szFlags, "roam") == 0)
		{
			return BASEFLAGS_ROAM;
		}
		else if (std::strcmp(szFlags, "crouch") == 0)
		{
			return BASEFLAGS_CROUCH;
		}

		return BASEFLAGS_NONE;
	}

	static void PrintBaseFlagsToConsole();

	bool operator==(const CWaypoint& other)
	{
		return this->m_ID == other.m_ID;
	}

	// Resets temporary waypoint data
	virtual void Reset();
	// Called at intervals
	virtual void Update();
	// invoked when a game round restarts
	virtual void OnRoundRestart() { Reset(); }
	// Is this waypoint enabled?
	virtual bool IsEnabled() const { return true; }
	// Can this team use this waypoint
	virtual bool IsAvailableToTeam(const int teamNum) const
	{
		if (m_teamNum < TEAM_UNASSIGNED)
		{
			return true;
		}

		return m_teamNum == teamNum;
	}
	// Can this bot use this waypoint
	virtual bool CanBeUsedByBot(CBaseBot* bot) const;

	virtual void Save(std::fstream& filestream, uint32_t version);
	virtual NavErrorType Load(std::fstream& filestream, uint32_t version, uint32_t subVersion);
	virtual NavErrorType PostLoad();

	// Draws this waypoint during editing
	virtual void Draw() const;

	/**
	 * @brief Marks this waypoint as being used by this bot. Prevents other bots from using this waypoint.
	 * @param user Bot using this waypoint
	 * @param duration Waypoint use timeout in case the bot doesn't notify it stopped using it.
	 */
	void Use(CBaseBot* user, const float duration = 10.0f) const;
	// True if a bot is using this waypoint.
	bool IsBeingUsed() const;
	// True if the given bot is currently using this waypoint
	bool IsCurrentUser(CBaseBot* me) const;
	/**
	 * @brief Notify that this waypoint is no longer being used.
	 * @param user Bot that was using this waypoint.
	 */
	void StopUsing(CBaseBot* user) const;

	WaypointID GetID() const { return m_ID; }
	const Vector& GetOrigin() const { return m_origin; }
	void SetOrigin(const Vector& origin) { m_origin = origin; }
	// Gets a random position within the waypoint's radius.
	Vector GetRandomPoint() const;

	float DistanceTo(const Vector& other) const;
	float DistanceTo2D(const Vector& other) const;
	float DistanceTo2D(const Vector2D& other) const;

	/**
	 * @brief Gets a stored angle from the waypoint
	 * @param index Index to the angle array, from 0 to MAX_AIM_ANGLES - 1
	 * @return Reference to angle.
	 */
	const QAngle& GetAngle(std::size_t index) const;
	/**
	 * @brief Gets a random angle from waypoint.
	 * @return Angle or NULL optional if this waypoint doesn't have any angles.
	 */
	std::optional<QAngle> GetRandomAngle() const;

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
	inline void ForEveryAngle(T& func) const
	{
		for (std::size_t i = 0U; i < GetNumOfAvailableAngles(); i++)
		{
			const QAngle& angle = m_aimAngles[i];
			func(angle);
		}
	}

	inline void SetFlags(BaseFlags val)
	{
		m_flags |= val;
	}

	inline void ClearFlags(BaseFlags val)
	{
		m_flags &= ~val;
	}

	inline bool HasFlags(BaseFlags val) const
	{
		return (m_flags & val) ? true : false;
	}

	inline void SetTeam(int team)
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

	const bool HasConnections() const { return !m_connections.empty(); }
	const std::vector<WaypointConnect>& GetConnections() const { return m_connections; }

	/**
	 * @brief Runs a function on every connected waypoint.
	 * @tparam T Waypoint class.
	 * @tparam F Function to run. bool (const T* waypoint)
	 * @param functor Function to run. Return false to end loop early.
	 */
	template <typename T, typename F>
	void ForEveryConnectedWaypoint(F functor) const
	{
		for (auto& connect : m_connections)
		{
			const T* wpt = static_cast<T*>(connect.GetOther());

			if (functor(wpt) == false)
			{
				return;
			}
		}
	}

	// Prints information about this waypoint to the developer console. (Called when the waypoint info command is used on this waypoint)
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
	virtual void DrawModText() const {}

public:

	/* Utility classes */

	/**
	 * @brief Utility class for converting the waypoint's angles into world positions for the bot to aim at.
	 * 
	 * Use with the 'ForEveryAngle' function.
	 */
	class BuildAimSpotFunctor
	{
	public:
		/**
		 * @brief Constructor.
		 * @param pos Aim origin (generally the waypoint origin + view height)
		 * @param aimSpotsVec Vector to store the aim positions at.
		 */
		BuildAimSpotFunctor(const Vector& pos, std::vector<Vector>* aimSpotsVec) :
			origin(pos)
		{
			output = aimSpotsVec;
		}

		void operator()(const QAngle& angle);

		Vector origin;
		std::vector<Vector>* output;
	};
};

extern ConVar sm_nav_waypoint_edit;

#endif