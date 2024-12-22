#ifndef NAV_ELEVATOR_H_
#define NAV_ELEVATOR_H_

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

/**
 * @brief Class for representing a nav mesh elevator
 */
class CNavElevator
{
public:
	CNavElevator();
	virtual ~CNavElevator();

	static inline unsigned int s_nextID{ 0 };
	static constexpr auto MAX_DRAW_RANGE = 1024.0f;
	static constexpr auto UPDATE_INTERVAL = 0.2f;

	enum ElevatorType : std::int32_t
	{
		UNDEFINED = 0,
		AUTO_TRIGGER, // elevator is triggered automatically when a player enters it.
		DOOR, // elevator is made using a func_door entity
		MOVELINEAR, // elevator is made using a func_movelinear entity
		TRAIN, // elevator is made using a func_tracktrain entity
		ELEVATOR, // elevator is made using a func_elevator entity (L4D / L4D2)
	
	};

	class ElevatorEntity
	{
	public:
		ElevatorEntity() :
			handle(nullptr), position(0.0f, 0.0f, 0.0f)
		{
			classname.reserve(64);
			targetname.reserve(64);
		}

		void Save(std::fstream& filestream, uint32_t version);
		void Load(std::fstream& filestream, uint32_t version, uint32_t subVersion);
		void PostLoad();
		void SearchForEntity(const bool noerror = true);
		void AssignEntity(CBaseEntity* entity);

		CHandle<CBaseEntity> handle;
		std::string classname;
		std::string targetname;
		Vector position; // position for post load search
	};

	class ElevatorFloor
	{
	public:
		ElevatorFloor()
		{
			floor_area = static_cast<unsigned int>(0U);
			is_here = false;
			shootable_button = false;
			toggle_state = -1;
		}

		ElevatorFloor(CNavArea* area)
		{
			floor_area = area;
			is_here = false;
			shootable_button = false;
			toggle_state = -1;
		}

		void ConvertAreaIDToPointer();
		CNavArea* GetArea() const { return std::get<CNavArea*>(floor_area); }

		void Save(std::fstream& filestream, uint32_t version);
		void Load(std::fstream& filestream, uint32_t version, uint32_t subVersion);
		void PostLoad();

		bool HasCallButton() const { return !this->call_button.classname.empty(); }
		bool HasUseButton() const { return !this->use_button.classname.empty(); }
		float GetDistanceTo(const ElevatorFloor* other) const;

		/* Save Data */
		ElevatorEntity call_button; // button to call the elevator if it's not on this floor
		ElevatorEntity use_button; // button to activate the elevator
		std::variant<unsigned int, CNavArea*> floor_area; // nav area where the bot must be to ride the elevator
		Vector floor_position; // position used to detect if the elevator is at this floor
		Vector wait_position; // position the bot will wait after calling the elevator
		bool shootable_button; // button must be shot with a hitscan weapon to trigger it, otherwise press the use key
		int toggle_state; // for func_door elevators

		/* Runtime Data */
		bool is_here; // elevator is at this floor
	};

	unsigned int GetID() const { return m_id; }
	// Gets the elevator save origin 
	const Vector& GetElevatorOrigin() const { return m_elevator.position; }

	virtual void Update();
	virtual void OnRoundRestart();
	virtual bool IsBlocked(int teamID) const;

	virtual void Draw() const; // draws this elevator 
	virtual void ScreenText() const; // screen text for this elevator

	virtual void Save(std::fstream& filestream, uint32_t version);
	virtual NavErrorType Load(std::fstream& filestream, uint32_t version, uint32_t subVersion);
	virtual NavErrorType PostLoad();

	const ElevatorFloor* GetFloorForArea(const CNavArea* area) const;
	const ElevatorFloor* GetStoppedFloor() const;
	float GetLengthBetweenFloors(const CNavArea* area1,const CNavArea* area2) const;
	float GetSpeed() const;
	/**
	 * @brief Given two nav areas, check if they can be reached via this elevator.
	 * @param from Start area.
	 * @param to Destination area.
	 * @return True if the elevator can be used to go from the start area to the destination area.
	 */
	bool IsReachableViaElevator(const CNavArea* from, const CNavArea* to) const;
	bool IsMultiFloorElevator() const { return m_floors.size() > 2U; }

	void AssignElevatorEntity(CBaseEntity* elevator);
	void UpdateElevatorInitialPosition();
	void DetectType();
	void UpdateEntityProps();
	void SetType(ElevatorType type) { m_type = type; }
	ElevatorType GetType() const { return m_type; }
	void SetFloorDetectionDistance(float dist)
	{
		if (dist < 32.0f)
		{
			m_minFloorDistance = 32.0f;
		}
		else
		{
			m_minFloorDistance = dist;
		}
	}
	// Gets the current toggle state, returns -1 if invalid (entity doesn't have a toggle state)
	int GetCurrentToggleState() const;

	void SetTeamNum(int teamID) { m_team = teamID; }
	int GetTeamNum() const { return m_team; }
	const std::vector<ElevatorFloor>& GetFloors() const { return m_floors; }

	bool CreateNewFloor(CNavArea* area);
	void RemoveFloor(CNavArea* area);
	void RemoveAllFloors();
	void NotifyNavAreaDestruction(CNavArea* area);

	/* Floor Edit Commands */

	void SetFloorToggleState(CNavArea* area, int ts);
	void SetFloorCallButton(CNavArea* area, CBaseEntity* entity);
	void SetFloorUseButton(CNavArea* area, CBaseEntity* entity);
	void SetFloorWaitPosition(CNavArea* area, const Vector& position);
	void SetFloorPosition(CNavArea* area, const Vector& position);
	void SetFloorShootableButton(CNavArea* area);

protected:
	static constexpr auto SCREENTEXT_BASE_X = 0.22f;
	static constexpr auto SCREENTEXT_BASE_Y = 0.22f;

	virtual void DetectCurrentFloor();

	static inline Vector waitpos_min{ -12.0f, -12.0f, 0.0f };
	static inline Vector waitpos_max{ 12.0f, 12.0f, 32.0f };

private:
	friend class CNavMesh;

	unsigned int m_id;
	int m_team; // team who can use this elevator
	ElevatorEntity m_elevator; // The elevator entity itself
	std::vector<ElevatorFloor> m_floors; // Floor connections
	ElevatorType m_type;
	float m_minFloorDistance;

	int* m_toggle_state; // Entity toggle state
	bool* m_bLocked; // Door locked?
	float* m_doorSpeed; // Door speed (m_flSpeed)
};

extern ConVar sm_nav_elevator_edit;

static_assert(sizeof(CNavElevator::ElevatorType) == sizeof(std::int32_t), "Changing the size of CNavElevator::ElevatorType will break save/load!");

#endif // !NAV_ELEVATOR_H_