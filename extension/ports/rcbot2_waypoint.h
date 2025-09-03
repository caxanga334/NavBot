#ifndef __NAVBOT_RCBOT2_WAYPOINT_H_
#define __NAVBOT_RCBOT2_WAYPOINT_H_

#include <vector>

class CRCBot2WaypointAuthorInfo
{
public:
	char szAuthor[32];
	char szModifiedBy[32];
};

class CRCBot2WaypointHeader
{
public:
	char szFileType[16];
	char szMapName[64];
	int iVersion;
	int iNumWaypoints;
	int iFlags;
};

class CRCBot2Waypoint
{
public:
	CRCBot2Waypoint();

	static constexpr int W_FL_NONE = 0;
	static constexpr int W_FL_JUMP = 1 << 0;
	static constexpr int W_FL_CROUCH = 1 << 1;
	static constexpr int W_FL_UNREACHABLE = 1 << 2;

	static constexpr int W_FL_FLAG = 1 << 4;

	static constexpr int W_FL_CAPPOINT = 1 << 5;

	static constexpr int W_FL_NOBLU = 1 << 6;
	static constexpr int W_FL_NOAXIS = 1 << 6;
	static constexpr int W_FL_NOTERRORIST = 1 << 6; // Counter-Strike: Source --> Terrorists cannot use this waypoint

	static constexpr int W_FL_NORED = 1 << 7;
	static constexpr int W_FL_NOALLIES = 1 << 7;
	static constexpr int W_FL_NOCOUNTERTR = 1 << 7; // Counter-Strike: Source --> Counter-Terrorists cannot use this waypoint

	static constexpr int W_FL_ROCKET_JUMP = 1 << 10;

	static constexpr int W_FL_SNIPER = 1 << 11;

	static constexpr int W_FL_SENTRY = 1 << 14;
	static constexpr int W_FL_MACHINEGUN = 1 << 14;

	static constexpr int W_FL_DOUBLEJUMP = 1 << 15;
	static constexpr int W_FL_TELE_ENTRANCE = 1 << 16;
	static constexpr int W_FL_TELE_EXIT = 1 << 17;
	static constexpr int W_FL_DEFEND = 1 << 18;
	static constexpr int W_FL_ROUTE = 1 << 20;

	void Load(std::fstream& file, const int version);

	const Vector& GetOrigin() const { return m_origin; }
	const int GetAimYaw() const { return m_aimYaw; }
	const int GetFlags() const { return m_flags; }
	const int GetAreaNumber() const { return m_areaNumber; }
	const float GetRadius() const { return m_radius; }
	const bool IsUsed() const { return m_isUsed; }
	const bool HasFlags(const int flags) const { return (m_flags & flags) != 0; }
	const int GetID() const { return m_id; }
	const std::vector<int>& GetPaths() const { return m_paths; }

	inline static int s_nextID{ 0 };

private:
	int m_id;
	Vector m_origin;
	int m_aimYaw;
	int m_flags;
	int m_areaNumber;
	float m_radius;
	bool m_isUsed;
	std::vector<int> m_paths;
};

class CRCBot2WaypointLoader
{
public:
	static constexpr int MIN_SUPPORTED_VERSION = 4;
	static constexpr int MAX_SUPPORTED_VERSION = 5;
	static constexpr std::string_view RCBOT2_WAYPOINT_FILE_TYPE = "RCBot2";

	CRCBot2WaypointLoader();

	// Parses an rcbot2 waypoint file
	bool Load(std::fstream& file);

	// Gets a waypoint of the given ID
	const CRCBot2Waypoint* GetWaypointOfID(const int id) const
	{
		for (const CRCBot2Waypoint& waypoint : m_waypoints)
		{
			if (waypoint.GetID() == id)
			{
				return &waypoint;
			}
		}

		return nullptr;
	}

	const std::vector<CRCBot2Waypoint>& GetWaypoints() const { return m_waypoints; }

private:
	std::vector<CRCBot2Waypoint> m_waypoints;
};

#endif // !__NAVBOT_RCBOT2_WAYPOINT_H_
