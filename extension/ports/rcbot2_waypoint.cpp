#include NAVBOT_PCH_FILE
#include "rcbot2_waypoint.h"

CRCBot2Waypoint::CRCBot2Waypoint()
{
	// RCBot2 wpt ID isn't stored on the waypoint itself, this is a bit experimental as the Waypoint ID seems to be based on the load order.
	m_id = s_nextID;
	s_nextID++;
	m_origin = vec3_origin;
	m_aimYaw = 0;
	m_flags = 0;
	m_areaNumber = 0;
	m_radius = 0.0f;
	m_isUsed = false;
}

CRCBot2WaypointLoader::CRCBot2WaypointLoader()
{
	m_waypoints.reserve(4096);
}

void CRCBot2Waypoint::Load(std::fstream& file, const int version)
{
	file.read(reinterpret_cast<char*>(&m_origin), sizeof(Vector));

	// RCBot2 adds 32 to the player's origin, see the CClient::getOrigin function in utils/RCBot2_meta/bot_client.cpp
	m_origin.z -= 32.0f;

	file.read(reinterpret_cast<char*>(&m_aimYaw), sizeof(int));
	file.read(reinterpret_cast<char*>(&m_flags), sizeof(int));
	file.read(reinterpret_cast<char*>(&m_isUsed), sizeof(bool));

	int pathCount = 0;
	file.read(reinterpret_cast<char*>(&pathCount), sizeof(int));

	for (int i = 0; i < pathCount; i++)
	{
		int id = -1;
		file.read(reinterpret_cast<char*>(&id), sizeof(int));

		if (id > 0)
		{
			m_paths.push_back(std::move(id));
		}
	}

	file.read(reinterpret_cast<char*>(&m_areaNumber), sizeof(int));
	file.read(reinterpret_cast<char*>(&m_radius), sizeof(float));
}

bool CRCBot2WaypointLoader::Load(std::fstream& file)
{
	CRCBot2WaypointHeader header;
	CRCBot2WaypointAuthorInfo authorinfo;

	file.read(reinterpret_cast<char*>(&header), sizeof(header));

	if (!file.good())
	{
		META_CONPRINT("RCBot2 Waypoint Loader: Failed to read waypoint header!\n");
		return false;
	}

	if (std::strcmp(header.szFileType, CRCBot2WaypointLoader::RCBOT2_WAYPOINT_FILE_TYPE.data()) != 0)
	{
		META_CONPRINTF("RCBot2 Waypoint Loader: Waypoint header file type mismatch! %s != %s\n", header.szFileType, CRCBot2WaypointLoader::RCBOT2_WAYPOINT_FILE_TYPE.data());
		return false;
	}

	if (header.iVersion < CRCBot2WaypointLoader::MIN_SUPPORTED_VERSION)
	{
		META_CONPRINT("RCBot2 Waypoint Loader: Waypoint file is too old!\n");
		return false;
	}

	if (header.iVersion > CRCBot2WaypointLoader::MAX_SUPPORTED_VERSION)
	{
		META_CONPRINT("RCBot2 Waypoint Loader: Waypoint file version is newer than the max supported version!\n");
		return false;
	}

	// we don't care about map names mismatches
	file.read(reinterpret_cast<char*>(&authorinfo), sizeof(authorinfo));

	META_CONPRINTF("RCBot2 Waypoint Loader: %i waypoints. \n", header.iNumWaypoints);

	CRCBot2Waypoint::s_nextID = 0;

	for (int i = 0; i < header.iNumWaypoints; i++)
	{
		CRCBot2Waypoint& waypoint = m_waypoints.emplace_back();

		waypoint.Load(file, header.iVersion);

		if (!file.eof() && !file.good())
		{
			META_CONPRINTF("RCBot2 Waypoint Loader: Bad read. Waypoint %i out of %i. \n", i, header.iNumWaypoints);
			return false;
		}
	}

	// removed unused waypoints
	m_waypoints.erase(std::remove_if(m_waypoints.begin(), m_waypoints.end(), [](const CRCBot2Waypoint& waypoint) {
		return !waypoint.IsUsed();
	}), m_waypoints.end());

	META_CONPRINTF("RCBot2 Waypoint Loader: %zu waypoints loaded. \n", m_waypoints.size());

	return true;
}

const CRCBot2Waypoint* CRCBot2WaypointLoader::GetNearestWaypoint(const Vector& origin, const float maxRange) const
{
	const CRCBot2Waypoint* out = nullptr;
	float smallest = std::numeric_limits<float>::max();

	for (auto& waypoint : m_waypoints)
	{
		const float range = (origin - waypoint.GetOrigin()).Length();

		if (maxRange > 0.0f && range > maxRange) { continue; }

		if (range < smallest)
		{
			smallest = range;
			out = &waypoint;
		}
	}

	return out;
}

const CRCBot2Waypoint* CRCBot2WaypointLoader::GetNearestFlaggedWaypoint(const Vector& origin, int flags, const float maxRange) const
{
	const CRCBot2Waypoint* out = nullptr;
	float smallest = std::numeric_limits<float>::max();

	for (auto& waypoint : m_waypoints)
	{
		const float range = (origin - waypoint.GetOrigin()).Length();

		if (maxRange > 0.0f && range > maxRange) { continue; }

		if (!waypoint.HasFlags(flags)) { continue; }

		if (range < smallest)
		{
			smallest = range;
			out = &waypoint;
		}
	}

	return out;
}
