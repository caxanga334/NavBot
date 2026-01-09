#ifndef __NAVBOT_DODS_NAV_WAYPOINT_H_
#define __NAVBOT_DODS_NAV_WAYPOINT_H_

#include <navmesh/nav_waypoint.h>

class CDoDSWaypoint : public CWaypoint
{
public:
	CDoDSWaypoint();

	static void RegisterCommands();

	void Save(std::fstream& filestream, uint32_t version) override;
	NavErrorType Load(std::fstream& filestream, uint32_t version, uint32_t subVersion) override;
	// returns true if this waypoint is assigned to a control point
	bool IsAssignedToaControlPoint() const;
	bool IsAvailableToTeam(const int teamNum) const override;

	enum DoDWPAttributes
	{
		DOD_WP_ATTRIBS_NONE = 0,
		DOD_WP_ATTRIBS_MGNEST = 0x1, // machine gunner's will nest and deploy their MG here
		DOD_WP_ATTRIBS_PRONE = 0x2, // bots using this waypoint will go prone
		DOD_WP_ATTRIBS_POINT_OWNERS_ONLY = 0x4, // this waypoint can only be used by the team that owns the control point
		DOD_WP_ATTRIBS_POINT_ATTACKERS_ONLY = 0x8, // this waypoint can only be used by the attacking team.
		DOD_WP_ATTRIBS_USE_RIFLEGREN = 0x10, // rifleman bots will use their rifle grenades here
	};

	inline static constexpr std::array<std::pair<DoDWPAttributes, std::string_view>, 5> s_dodwpattribsmap = {{
		{ DOD_WP_ATTRIBS_MGNEST, "MGNEST" },
		{ DOD_WP_ATTRIBS_PRONE, "PRONE" },
		{ DOD_WP_ATTRIBS_POINT_OWNERS_ONLY, "POINT_OWNERS_ONLY" },
		{ DOD_WP_ATTRIBS_POINT_ATTACKERS_ONLY, "POINT_ATTACKERS_ONLY" },
		{ DOD_WP_ATTRIBS_USE_RIFLEGREN, "USE_RIFLEGREN" },
	}};

	static DoDWPAttributes StringToDoDWPAttribute(const char* str);
	static const char* DoDWPAttributeToString(DoDWPAttributes attrib)
	{
		for (auto& pair : s_dodwpattribsmap)
		{
			if (pair.first == attrib)
			{
				return pair.second.data();
			}
		}

		return "INVALID";
	}

	// Assigns a control point to this waypoint
	void AssignToControlPoint(int index)
	{
		m_CPIndex = index;
	}
	int GetAssignedControlPoint() const { return m_CPIndex; }
	void SetDoDWaypointAttributes(DoDWPAttributes attribute)
	{
		m_dodattributes |= attribute;
	}
	void ClearDoDWaypointAttributes(DoDWPAttributes attribute)
	{
		m_dodattributes &= ~attribute;
	}
	bool HasDoDWaypointAttributes(DoDWPAttributes attributes) const
	{
		return (m_dodattributes & attributes) != 0;
	}
	void WipeDoDWaypointAttributes()
	{
		m_dodattributes = 0;
	}

protected:
	void DrawModText() const override;

private:
	int m_dodattributes; // DoD specific waypoint attributes
	int m_CPIndex; // Assigned control point index
};


#endif // !__NAVBOT_DODS_NAV_WAYPOINT_H_
