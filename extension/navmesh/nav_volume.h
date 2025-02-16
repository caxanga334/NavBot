#include <cinttypes>
#include <algorithm>
#include <fstream>
#include <vector>
#include <variant>
#include <array>
#include <string>
#include <sdkports/sdk_ehandle.h>
#include <sdkports/sdk_timers.h>
#include "nav.h"
#include "nav_scripting.h"

class CNavVolume
{
public:
	CNavVolume();
	virtual ~CNavVolume();

	// Condition type for determining if the volume is blocked or not
	enum ConditionType : int
	{
		CONDITION_NONE = 0, // No condition
		CONDITION_SOLID_WORLD, // blocked if there is a solid obstruction (world and props only)
		CONDITION_ENTITY_EXISTS, // blocked if the target entity exists
		CONDITION_ENTITY_ENABLED, // blocked if the target entity is enabled
		CONDITION_ENTITY_LOCKED, // blocked if the target entity is locked
		CONDITION_DOOR_CLOSED, // blocked if the target door is closed
		CONDITION_ENTITY_TEAM, // blocked if the target entity is not owned by the team
		CONDITION_ENTITY_DISTANCE, // blocked if the distance from the volume origin to the entity is less than the data value

		MAX_CONDITION_TYPES
	};

	static constexpr float UPDATE_INTERVAL = 2.0f;
	static constexpr float MAX_DRAW_RANGE = 1500.0f;
	static unsigned int s_nextID;
	static constexpr float BASE_SCREENX = 0.52f;
	static constexpr float BASE_SCREENY = 0.56f;

	unsigned int GetID() const { return m_id; }
	void SetOrigin(const Vector& origin);
	const Vector& GetOrigin() const;
	void SetBounds(const Vector& mins, const Vector& maxs);
	void GetBounds(Vector& mins, Vector& maxs) const;
	void SetTeam(int team)
	{
		if (team < 0 || team >= NAV_TEAMS_ARRAY_SIZE)
		{
			m_teamIndex = NAV_TEAM_ANY;
		}
		else
		{
			m_teamIndex = team;
		}
	}
	int GetTeam() const { return m_teamIndex; }

	void SetTargetEntity(CBaseEntity* entity) { m_toggle_condition.SetTargetEntity(entity); }
	CBaseEntity* GetTargetEntity() const { return m_toggle_condition.GetTargetEntity(); }

	void SetInvertedConditionCheck(bool value)
	{
		if (value)
		{
			if (!IsConditionCheckInverted())
			{
				m_toggle_condition.ToggleInvertedCondition();
			}
		}
		else
		{
			if (IsConditionCheckInverted())
			{
				m_toggle_condition.ToggleInvertedCondition();
			}
		}
	}
	bool IsConditionCheckInverted() const { return m_toggle_condition.IsTestConditionInverted(); }

	bool IntersectsWith(const CNavVolume* other) const;
	void SearchForNavAreas();
	void SearchTargetEntity() { m_toggle_condition.SearchForEntities(); }
	
	virtual void Update();
	virtual void OnRoundRestart()
	{
		m_scanTimer.Start(1.0f);
		m_toggle_condition.OnRoundRestart();
	}
	virtual void Save(std::fstream& filestream, uint32_t version);
	virtual NavErrorType Load(std::fstream& filestream, uint32_t version, uint32_t subVersion);
	virtual NavErrorType PostLoad(void);
	virtual void Draw() const; // draw this volume
	void DrawAreas() const;
	virtual void ScreenText() const; // screen text for this volume, only called on the selected volume
	virtual bool IsBlocked(int teamID) const;

	void SetToggleData(float data) { m_toggle_condition.SetFloatData(data); }
	void SetToggleData(int data) { m_toggle_condition.SetIntegerData(data); }
	void SetToggleData(const Vector& data) { m_toggle_condition.SetVectorData(data); }
	void SetToggleData(CBaseEntity* data) { m_toggle_condition.SetTargetEntity(data); }
	void SetToggleData(navscripting::ToggleCondition::TCTypes data) { m_toggle_condition.SetToggleConditionType(data); }
	bool IsToggleConditionInverted() const { return m_toggle_condition.IsTestConditionInverted(); }
	void InvertToggleCondition() { m_toggle_condition.ToggleInvertedCondition(); }
	void ClearToggleData() { m_toggle_condition.clear(); }

protected:
	void UpdateBlockedStatus(int teamID, bool blocked)
	{
		if (teamID < 0 || teamID >= static_cast<int>(m_blockedCache.size()))
		{
			std::fill(m_blockedCache.begin(), m_blockedCache.end(), blocked);
		}
		else
		{
			m_blockedCache[teamID] = blocked;
		}
	}

private:
	friend class CNavMesh;

	unsigned int m_id;
	Vector m_origin; // box origins
	Vector m_mins; // box mins
	Vector m_maxs; // box maxs
	Vector m_calculatedMins; // calculated origin + mins
	Vector m_calculatedMaxs; // calculated origin + maxs
	std::vector<CNavArea*> m_areas; // vector of nav areas inside this volume
	int m_teamIndex; // team index
	std::array<bool, NAV_TEAMS_ARRAY_SIZE> m_blockedCache;
	navscripting::ToggleCondition m_toggle_condition;
	CountdownTimer m_scanTimer;

	class FindAreasInVolume
	{
	public:
		FindAreasInVolume(CNavVolume* v)
		{
			this->volume = v;
		}

		bool operator()(CNavArea* area);

	private:
		CNavVolume* volume;
	};

	enum EntitySearchMethod : std::uint8_t
	{
		SEARCH_TARGETNAME = 0U, // Search entity by target name
		SEARCH_NEAREST, // Search entity by classname

		MAX_SEARCH_METHODS
	};
};

extern ConVar sm_nav_volume_edit;