#ifndef NAVBOT_TF_NAV_MESH_H_
#define NAVBOT_TF_NAV_MESH_H_

#include <navmesh/nav_waypoint.h>

class CTFWaypoint : public CWaypoint
{
public:
	static constexpr int NO_CONTROL_POINT = -1;

	CTFWaypoint();
	~CTFWaypoint() override;

	enum TFHint : unsigned int
	{
		TFHINT_NONE = 0,
		TFHINT_GUARD, // Guard/Defend position
		TFHINT_SNIPER, // Sniper spot
		TFHINT_SENTRYGUN, // Sentry gun spot
		TFHINT_DISPENSER, // Dispenser spot
		TFHINT_TELE_ENTRANCE, // Tele entrance
		TFHINT_TELE_EXIT, // Tele exit

		MAX_TFHINT_TYPES
	};

	static const char* TFHintToString(CTFWaypoint::TFHint hint);
	static bool IsValidTFHint(long long hint)
	{
		return hint >= TFHINT_NONE && hint < MAX_TFHINT_TYPES;
	}
	
	static CTFWaypoint::TFHint StringToTFHint(const char* szName);

	void Save(std::fstream& filestream, uint32_t version) override;
	NavErrorType Load(std::fstream& filestream, uint32_t version, uint32_t subVersion) override;

	bool IsAvailableToTeam(const int teamNum) const override;

	void SetControlPointIndex(int val = CTFWaypoint::NO_CONTROL_POINT) { m_cpindex = val; }
	int GetControlPointIndex() const { return m_cpindex; }

	void SetTFHint(CTFWaypoint::TFHint hint) { m_tfhint = hint; }
	CTFWaypoint::TFHint GetTFHint() const { return m_tfhint; }
	// Checks if the given bot can 
	bool CanBuildHere(CTF2Bot* bot) const;

private:
	int m_cpindex; // Control point index assigned to this waypoint
	TFHint m_tfhint; // TF2 hint assigned to this waypoint

	void DrawModText() const override;
};

#endif // !NAVBOT_TF_NAV_MESH_H_