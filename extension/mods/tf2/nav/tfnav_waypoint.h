#ifndef NAVBOT_TF_NAV_MESH_H_
#define NAVBOT_TF_NAV_MESH_H_

#include <navmesh/nav_waypoint.h>

class CTFWaypoint : public CWaypoint
{
public:
	static constexpr int NO_CONTROL_POINT = -1;

	CTFWaypoint();
	~CTFWaypoint() override;

	void Save(std::fstream& filestream, uint32_t version) override;
	NavErrorType Load(std::fstream& filestream, uint32_t version, uint32_t subVersion) override;

	bool IsAvailableToTeam(const int teamNum) override;

	void SetControlPointIndex(int val = CTFWaypoint::NO_CONTROL_POINT) { m_cpindex = val; }
	int GetControlPointIndex() const { return m_cpindex; }

private:
	int m_cpindex; // Control point index assigned to this waypoint

	void DrawModText() const override;
};

#endif // !NAVBOT_TF_NAV_MESH_H_