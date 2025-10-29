#ifndef NAV_MESH_PREREQUISITE_H_
#define NAV_MESH_PREREQUISITE_H_

#include "nav.h"
#include "nav_scripting.h"

/**
 * @brief Prerequisite holds a task the bots needs to do when entering a nav area.
 * 
 * NavBot's version of func_nav_prerequisite ( https://developer.valvesoftware.com/wiki/Func_nav_prerequisite )
 */
class CNavPrerequisite
{
public:
	CNavPrerequisite();
	virtual ~CNavPrerequisite();

	enum PrerequisiteTask : int
	{
		TASK_NONE = 0,
		TASK_WAIT,
		TASK_MOVE_TO_POS,
		TASK_DESTROY_ENT,
		TASK_USE_ENT,

		MAX_TASK_TYPES
	};

	static bool IsEditing();

	static const char* TaskIDtoString(PrerequisiteTask task);

	static inline unsigned int s_nextID{ 0 };
	static constexpr auto MAX_EDIT_DRAW_DISTANCE = 1024.0f;

	virtual void Save(std::fstream& filestream, uint32_t version);
	virtual NavErrorType Load(std::fstream& filestream, uint32_t version, uint32_t subVersion);
	virtual NavErrorType PostLoad(void);
	virtual void OnRoundRestart();
	virtual bool IsEnabled() const { return m_toggle_condition.RunTestCondition(); }

	void SetFloatData(float value) { m_flData = value; }
	void SetGoalPosition(const Vector& pos) { m_goalPosition = pos; }
	void SetTeamIndex(int team)
	{
		if (team < 0)
		{
			m_teamIndex = NAV_TEAM_ANY;
		}
		else
		{
			m_teamIndex = team;
		}
	}
	void SetOrigin(const Vector& origin)
	{
		m_origin = origin;
		m_calculatedMins = (m_origin + m_mins);
		m_calculatedMaxs = (m_origin + m_maxs);
		OnSizeChanged();
	}
	void SetBounds(const Vector& mins, const Vector& maxs)
	{
		m_mins = mins;
		m_maxs = maxs;
		m_calculatedMins = (m_origin + m_mins);
		m_calculatedMaxs = (m_origin + m_maxs);
		OnSizeChanged();
	}
	void SetTask(int taskID)
	{
		if (taskID <= static_cast<int>(PrerequisiteTask::TASK_NONE) || taskID >= static_cast<int>(PrerequisiteTask::MAX_TASK_TYPES))
		{
			m_task = PrerequisiteTask::TASK_NONE;
		}
		else
		{
			m_task = static_cast<PrerequisiteTask>(taskID);
		}
	}
	void AssignTaskEntity(CBaseEntity* entity)
	{
		m_goalEntity.LinkToEntity(entity);
	}

	unsigned int GetID() const { return m_id; }
	// Gets the task data
	float GetFloatData() const { return m_flData; }
	// Gets the task goal position.
	const Vector& GetGoalPosition() const { return m_goalPosition; }
	CBaseEntity* GetTaskEntity() const { return m_goalEntity.GetEntity(); }
	int GetTeamIndex() const { return m_teamIndex; }
	bool CanBeUsedByTeam(int team) const { return m_teamIndex == NAV_TEAM_ANY || m_teamIndex == team; }
	const Vector& GetOrigin() const { return m_origin; }
	void GetBounds(Vector& mins, Vector& maxs) const
	{
		mins = m_mins;
		maxs = m_maxs;
	}
	PrerequisiteTask GetTask() const { return m_task; }
	bool IsPointInsideVolume(const Vector& point) const;
	// Called when editing is enabled render the prerequisite
	virtual void Draw() const;
	virtual void ScreenText() const;
	void DrawAreas() const;

	void SetToggleData(float data) { m_toggle_condition.SetFloatData(data); }
	void SetToggleData(int data) { m_toggle_condition.SetIntegerData(data); }
	void SetToggleData(const Vector& data) { m_toggle_condition.SetVectorData(data); }
	void SetToggleData(CBaseEntity* data) { m_toggle_condition.SetTargetEntity(data); }
	void SetToggleData(navscripting::ToggleCondition::TCTypes data) { m_toggle_condition.SetToggleConditionType(data); }
	bool IsToggleConditionInverted() const { return m_toggle_condition.IsTestConditionInverted(); }
	void InvertToggleCondition() { m_toggle_condition.ToggleInvertedCondition(); }
	void ClearToggleData() { m_toggle_condition.clear(); }

	void SearchForNavAreas();

protected:
	virtual void OnSizeChanged() {};

	navscripting::EntityLink& GetGoalEntity() { return m_goalEntity; }
	navscripting::ToggleCondition& GetToggleCondition() { return m_toggle_condition; }

	static constexpr auto BASE_SCREENY = 0.20f;
	static constexpr auto BASE_SCREENX = 0.16f;

private:
	friend class CNavMesh;

	unsigned int m_id;
	Vector m_origin; // box origins
	Vector m_mins; // box mins
	Vector m_maxs; // box maxs
	Vector m_calculatedMins; // calculated origin + mins
	Vector m_calculatedMaxs; // calculated origin + maxs
	navscripting::EntityLink m_goalEntity; // if the prerequisite goal is an entity, this is it
	navscripting::ToggleCondition m_toggle_condition; // toggle condition for determining if this prereq is enabled
	PrerequisiteTask m_task;
	Vector m_goalPosition;
	float m_flData;
	int m_teamIndex;
	std::vector<CNavArea*> m_areas; // areas inside the prerequisite bounds
};

extern ConVar sm_nav_prerequisite_edit;

#endif // !NAV_MESH_PREREQUISITE_H_
