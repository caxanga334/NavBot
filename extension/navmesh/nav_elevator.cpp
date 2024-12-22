#include <extension.h>
#include <sdkports/debugoverlay_shared.h>
#include <util/entprops.h>
#include <entities/baseentity.h>
#include "nav_mesh.h"
#include "nav_area.h"
#include "nav_elevator.h"

CNavElevator::CNavElevator()
{
	m_id = CNavElevator::s_nextID;
	CNavElevator::s_nextID++;

	m_team = NAV_TEAM_ANY;
	m_type = ElevatorType::UNDEFINED;
	m_minFloorDistance = 48.0f;
	m_toggle_state = nullptr;
	m_bLocked = nullptr;
	m_doorSpeed = nullptr;
}

CNavElevator::~CNavElevator()
{
	for (auto& floor : m_floors)
	{
		floor.GetArea()->NotifyElevatorDestruction(this);
	}
}

void CNavElevator::Update()
{
	CBaseEntity* elevator = m_elevator.handle.Get();

	if (elevator == nullptr)
	{
		return;
	}

	DetectCurrentFloor();
}

void CNavElevator::OnRoundRestart()
{
	m_elevator.SearchForEntity();

	if (m_elevator.handle.Get() != nullptr)
	{
		UpdateEntityProps();
	}

	for (auto& floor : m_floors)
	{
		if (!floor.use_button.classname.empty() && floor.use_button.handle.Get() == nullptr)
		{
			floor.use_button.SearchForEntity();
		}

		if (!floor.call_button.classname.empty() && floor.call_button.handle.Get() == nullptr)
		{
			floor.call_button.SearchForEntity();
		}
	}
}

bool CNavElevator::IsBlocked(int teamID) const
{
	if (m_elevator.handle.Get() == nullptr)
	{
		return true; // blocked for everybody, elevator is gone
	}

	if (m_team != NAV_TEAM_ANY && m_team != teamID)
	{
		return true; // elevator areas are blocked for non owning team
	}

	return false;
}

void CNavElevator::Draw() const
{
	if (TheNavMesh->GetSelectedElevator().get() == this)
	{
		CBaseEntity* elev = m_elevator.handle.Get();

		if (elev != nullptr)
		{
			NDebugOverlay::EntityBounds(elev, 0, 128, 0, 128, NDEBUG_PERSIST_FOR_ONE_TICK);
			Vector center = UtilHelpers::getWorldSpaceCenter(elev);
			NDebugOverlay::Cross3D(center, 32.0f, 255, 0, 255, true, NDEBUG_PERSIST_FOR_ONE_TICK);
		}

		for (auto& floor : m_floors)
		{
			floor.GetArea()->DrawFilled(238, 130, 238, 128, NDEBUG_PERSIST_FOR_ONE_TICK);

			CBaseEntity* callButton = floor.call_button.handle.Get();
			Vector pos;

			if (callButton)
			{
				NDebugOverlay::EntityBounds(callButton, 0, 255, 255, 200, NDEBUG_PERSIST_FOR_ONE_TICK);
				pos = UtilHelpers::getWorldSpaceCenter(callButton);
				NDebugOverlay::Text(pos, "Call Button", false, NDEBUG_PERSIST_FOR_ONE_TICK);
			}

			CBaseEntity* useButton = floor.use_button.handle.Get();

			if (useButton)
			{
				NDebugOverlay::EntityBounds(useButton, 255, 255, 0, 200, NDEBUG_PERSIST_FOR_ONE_TICK);
				pos = UtilHelpers::getWorldSpaceCenter(useButton);
				NDebugOverlay::Text(pos, "Use Button", false, NDEBUG_PERSIST_FOR_ONE_TICK);
			}

			if (floor.is_here)
			{
				NDebugOverlay::Text(floor.GetArea()->GetCenter(), "Elevator is here!", false, NDEBUG_PERSIST_FOR_ONE_TICK);
			}

			if (!floor.wait_position.IsZero(0.5f))
			{
				NDebugOverlay::Box(floor.wait_position, waitpos_min, waitpos_max, 0, 71, 171, 200, NDEBUG_PERSIST_FOR_ONE_TICK);
				NDebugOverlay::Text(floor.wait_position, "Wait Position", false, NDEBUG_PERSIST_FOR_ONE_TICK);
			}

			if (!floor.floor_position.IsZero(0.5f))
			{
				NDebugOverlay::Sphere(floor.floor_position, 16.0f, 0, 128, 0, true, NDEBUG_PERSIST_FOR_ONE_TICK);
			}
		}
	}

	NDebugOverlay::Text(m_elevator.position, false, NDEBUG_PERSIST_FOR_ONE_TICK, "Nav Elevator #%i", m_id);
}

void CNavElevator::ScreenText() const
{
	NDebugOverlay::ScreenText(SCREENTEXT_BASE_X, SCREENTEXT_BASE_Y, 255, 255, 0, 255, NDEBUG_PERSIST_FOR_ONE_TICK, "Selected Nav Elevator #%i Team Num %i Type %i Floor Detection Distance: %3.2f", m_id, m_team, static_cast<int>(m_type), m_minFloorDistance);
	NDebugOverlay::ScreenText(SCREENTEXT_BASE_X, SCREENTEXT_BASE_Y + 0.04f, 255, 255, 0, 255, NDEBUG_PERSIST_FOR_ONE_TICK, "Entity: %s <%i>", 
		m_elevator.classname.c_str(), m_elevator.handle.GetEntryIndex());
}

void CNavElevator::Save(std::fstream& filestream, uint32_t version)
{
	filestream.write(reinterpret_cast<char*>(&m_id), sizeof(unsigned int));
	filestream.write(reinterpret_cast<char*>(&m_team), sizeof(int));
	filestream.write(reinterpret_cast<char*>(&m_type), sizeof(ElevatorType));
	filestream.write(reinterpret_cast<char*>(&m_minFloorDistance), sizeof(float));
	m_elevator.Save(filestream, version);

	std::uint64_t floorcount = static_cast<std::uint64_t>(m_floors.size());
	filestream.write(reinterpret_cast<char*>(&floorcount), sizeof(std::uint64_t));

	if (m_type == ElevatorType::DOOR || m_type == ElevatorType::MOVELINEAR)
	{
		if (m_floors.size() > 2)
		{
			Warning("Nav Elevator #%i is func_door/func_movelinear type but has more than two floors!\n", m_id);
		}
	}

	for (auto& floor : m_floors)
	{
		floor.Save(filestream, version);

		if (floor.wait_position.IsZero(0.5f))
		{
			Warning("Nav Elevator #%i without wait position set!\n", m_id);
		}

		if (m_type == ElevatorType::DOOR || m_type == ElevatorType::MOVELINEAR)
		{
			if (floor.toggle_state < 0)
			{
				Warning("Nav Elevator #%i is func_door/func_movelinear type but contains floors without a toggle_state set!\n", m_id);
			}
		}
	}
}

NavErrorType CNavElevator::Load(std::fstream& filestream, uint32_t version, uint32_t subVersion)
{
	filestream.read(reinterpret_cast<char*>(&m_id), sizeof(unsigned int));
	filestream.read(reinterpret_cast<char*>(&m_team), sizeof(int));
	filestream.read(reinterpret_cast<char*>(&m_type), sizeof(ElevatorType));
	filestream.read(reinterpret_cast<char*>(&m_minFloorDistance), sizeof(float));
	m_elevator.Load(filestream, version, subVersion);

	std::uint64_t floorcount = 0;
	filestream.read(reinterpret_cast<char*>(&floorcount), sizeof(std::uint64_t));

	for (std::uint64_t i = 0; i < floorcount; i++)
	{
		auto& floor = m_floors.emplace_back();
		floor.Load(filestream, version, subVersion);
	}

	if (filestream.good())
	{
		return NAV_OK;
	}

	return NAV_CORRUPT_DATA;
}

NavErrorType CNavElevator::PostLoad()
{
	m_elevator.PostLoad();

	for (auto& floor : m_floors)
	{
		floor.PostLoad();
		floor.GetArea()->SetElevator(this, &floor);
	}

	UpdateEntityProps();

	return NAV_OK;
}

const CNavElevator::ElevatorFloor* CNavElevator::GetFloorForArea(const CNavArea* area) const
{
	for (auto& floor : m_floors)
	{
		if (floor.GetArea() == area)
		{
			return &floor;
		}
	}

	return nullptr;
}

const CNavElevator::ElevatorFloor* CNavElevator::GetStoppedFloor() const
{
	for (auto& floor : m_floors)
	{
		if (floor.is_here)
		{
			return &floor;
		}
	}

	return nullptr;
}

float CNavElevator::GetLengthBetweenFloors(const CNavArea* area1, const CNavArea* area2) const
{
	const CNavElevator::ElevatorFloor* floor1 = GetFloorForArea(area1);
	const CNavElevator::ElevatorFloor* floor2 = GetFloorForArea(area2);

	if (floor1 == nullptr || floor2 == nullptr)
	{
		return -1.0f;
	}

	return (floor1->GetArea()->GetCenter() - floor2->GetArea()->GetCenter()).Length();
}

float CNavElevator::GetSpeed() const
{
	if ((m_type == ElevatorType::DOOR || m_type == ElevatorType::MOVELINEAR) && m_elevator.handle.Get() != nullptr)
	{
		return *m_doorSpeed;
	}

	return 100.0f;
}

bool CNavElevator::IsReachableViaElevator(const CNavArea* from, const CNavArea* to) const
{
	const CNavElevator::ElevatorFloor* floor1 = GetFloorForArea(from);
	const CNavElevator::ElevatorFloor* floor2 = GetFloorForArea(to);

	if (floor1 == nullptr || floor2 == nullptr)
	{
		return false;
	}

	if (!floor1->is_here && floor1->call_button.classname.empty())
	{
		return false; // elevator is not at the 'from' floor and there is no call button.
	}

	return true;
}

void CNavElevator::AssignElevatorEntity(CBaseEntity* elevator)
{
	m_elevator.AssignEntity(elevator);
	DetectType();
	UpdateEntityProps();
}

void CNavElevator::UpdateElevatorInitialPosition()
{
	CBaseEntity* elev = m_elevator.handle.Get();

	if (elev == nullptr)
	{
		return;
	}

	m_elevator.position = UtilHelpers::getWorldSpaceCenter(elev);
}

void CNavElevator::DetectType()
{
	CBaseEntity* elevator = m_elevator.handle.Get();

	if (UtilHelpers::FClassnameIs(elevator, "func_door"))
	{
		m_type = DOOR;
	}
	else if (UtilHelpers::FClassnameIs(elevator, "func_movelinear"))
	{
		m_type = MOVELINEAR;
	}
	else if (UtilHelpers::FClassnameIs(elevator, "func_tracktrain"))
	{
		m_type = TRAIN;
	}
	else if (UtilHelpers::FClassnameIs(elevator, "func_elevator"))
	{
		m_type = ELEVATOR;
	}
	else
	{
		Warning("Unable to determine elevator type for entity %s!\n", gamehelpers->GetEntityClassname(elevator));
		m_type = ElevatorType::UNDEFINED;
	}
}

void CNavElevator::UpdateEntityProps()
{
	if (m_type == ElevatorType::DOOR)
	{
		m_toggle_state = entprops->GetPointerToEntData<int>(m_elevator.handle.Get(), Prop_Data, "m_toggle_state");
		m_bLocked = entprops->GetPointerToEntData<bool>(m_elevator.handle.Get(), Prop_Data, "m_bLocked");
		m_doorSpeed = entprops->GetPointerToEntData<float>(m_elevator.handle.Get(), Prop_Data, "m_flSpeed");
	}
	else if (m_type == ElevatorType::MOVELINEAR)
	{
		// func_movelinear doesn't have m_bLocked
		m_toggle_state = entprops->GetPointerToEntData<int>(m_elevator.handle.Get(), Prop_Data, "m_toggle_state");
		m_doorSpeed = entprops->GetPointerToEntData<float>(m_elevator.handle.Get(), Prop_Data, "m_flSpeed");
	}
}

int CNavElevator::GetCurrentToggleState() const
{
	if ((m_type == ElevatorType::DOOR || m_type == ElevatorType::MOVELINEAR) && m_elevator.handle.Get() != nullptr)
	{
		return *m_toggle_state;
	}

	return -1;
}

bool CNavElevator::CreateNewFloor(CNavArea* area)
{
	for (auto& floor : m_floors)
	{
		if (floor.GetArea() == area)
		{
			return false;
		}
	}

	auto& floor = m_floors.emplace_back(area);
	area->SetElevator(this, &floor);

	return true;
}

void CNavElevator::RemoveFloor(CNavArea* area)
{
	for (auto& floor : m_floors)
	{
		if (floor.GetArea() == area)
		{
			area->NotifyElevatorDestruction(this);
		}
	}

	m_floors.erase(std::remove_if(m_floors.begin(), m_floors.end(), [&area](const CNavElevator::ElevatorFloor& floor) {
		return floor.GetArea() == area;
	}));
}

void CNavElevator::RemoveAllFloors()
{
	for (auto& floor : m_floors)
	{
		floor.GetArea()->NotifyElevatorDestruction(this);
	}

	m_floors.clear();
}

void CNavElevator::NotifyNavAreaDestruction(CNavArea* area)
{
	if (GetFloorForArea(area) == nullptr)
	{
		return;
	}

	RemoveFloor(area);
}

void CNavElevator::SetFloorToggleState(CNavArea* area, int ts)
{
	for (auto& floor : m_floors)
	{
		if (floor.GetArea() == area)
		{
			floor.toggle_state = ts;
			return;
		}
	}
}

void CNavElevator::SetFloorCallButton(CNavArea* area, CBaseEntity* entity)
{
	for (auto& floor : m_floors)
	{
		if (floor.GetArea() == area)
		{
			floor.call_button.AssignEntity(entity);
			return;
		}
	}
}

void CNavElevator::SetFloorUseButton(CNavArea* area, CBaseEntity* entity)
{
	for (auto& floor : m_floors)
	{
		if (floor.GetArea() == area)
		{
			floor.use_button.AssignEntity(entity);
			return;
		}
	}
}

void CNavElevator::SetFloorWaitPosition(CNavArea* area, const Vector& position)
{
	for (auto& floor : m_floors)
	{
		if (floor.GetArea() == area)
		{
			floor.wait_position = position;
			return;
		}
	}
}

void CNavElevator::SetFloorPosition(CNavArea* area, const Vector& position)
{
	for (auto& floor : m_floors)
	{
		if (floor.GetArea() == area)
		{
			floor.floor_position = position;
			return;
		}
	}
}

void CNavElevator::SetFloorShootableButton(CNavArea* area)
{
	for (auto& floor : m_floors)
	{
		if (floor.GetArea() == area)
		{
			floor.shootable_button = !floor.shootable_button;
			return;
		}
	}
}

void CNavElevator::DetectCurrentFloor()
{
	// for doors and move linear entities, use toggle state
	if (m_type == ElevatorType::DOOR || m_type == ElevatorType::MOVELINEAR)
	{
		int ts = *m_toggle_state;

		for (auto& floor : m_floors)
		{
			if (floor.toggle_state == ts)
			{
				floor.is_here = true;
			}
			else
			{
				floor.is_here = false;
			}
		}
	}
	else
	{
		Vector pos = UtilHelpers::getWorldSpaceCenter(m_elevator.handle.Get());

		for (auto& floor : m_floors)
		{
			floor.is_here = false;
			float dist = (floor.floor_position - pos).Length();

			if (dist < m_minFloorDistance)
			{
				floor.is_here = true; // don't break the loop, set the other floors to false
			}
		}
	}
}

void CNavElevator::ElevatorEntity::Save(std::fstream& filestream, uint32_t version)
{
	bool hasclassname = !this->classname.empty();
	filestream.write(reinterpret_cast<char*>(&hasclassname), sizeof(bool));

	if (hasclassname)
	{
		std::uint64_t size = static_cast<std::uint64_t>(this->classname.size() + 1U);
		filestream.write(reinterpret_cast<char*>(&size), sizeof(std::uint64_t));
		filestream.write(this->classname.c_str(), size);

		bool hastargetname = !this->targetname.empty();
		filestream.write(reinterpret_cast<char*>(&hastargetname), sizeof(bool));

		if (hastargetname)
		{
			size = static_cast<std::uint64_t>(this->targetname.size() + 1U);
			filestream.write(reinterpret_cast<char*>(&size), sizeof(std::uint64_t));
			filestream.write(this->targetname.c_str(), size);
		}

		filestream.write(reinterpret_cast<char*>(&this->position), sizeof(Vector));
	}
}

void CNavElevator::ElevatorEntity::Load(std::fstream& filestream, uint32_t version, uint32_t subVersion)
{
	bool hasclassname = false;
	filestream.read(reinterpret_cast<char*>(&hasclassname), sizeof(bool));

	if (hasclassname)
	{
		std::uint64_t size = 0;
		filestream.read(reinterpret_cast<char*>(&size), sizeof(std::uint64_t));
		std::unique_ptr<char[]> clname = std::make_unique<char[]>(size + 1);
		filestream.read(clname.get(), size);
		this->classname.assign(clname.get());

		bool hastargetname = false;
		filestream.read(reinterpret_cast<char*>(&hastargetname), sizeof(bool));

		if (hastargetname)
		{
			size = 0;
			filestream.read(reinterpret_cast<char*>(&size), sizeof(std::uint64_t));

			std::unique_ptr<char[]> tgtname = std::make_unique<char[]>(size + 1);

			filestream.read(tgtname.get(), size);
			this->targetname.assign(tgtname.get());
		}

		filestream.read(reinterpret_cast<char*>(&this->position), sizeof(Vector));
	}
}

void CNavElevator::ElevatorEntity::PostLoad()
{
	SearchForEntity(false);
}

void CNavElevator::ElevatorEntity::SearchForEntity(const bool noerror)
{
	if (this->classname.empty())
	{
		this->handle.Term(); // make sure this remains invalid
		return; // no entity assigned to this
	}

	if (!this->targetname.empty())
	{
		int ref = UtilHelpers::FindNamedEntityByClassname(INVALID_EHANDLE_INDEX, this->targetname.c_str(), this->classname.c_str());
		CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(ref);
		this->handle = pEntity;
	}
	else
	{
		CBaseEntity* nearest = nullptr;
		float best = FLT_MAX;
		const Vector& start = this->position;

		UtilHelpers::ForEachEntityOfClassname(this->classname.c_str(), [&nearest, &best, &start](int index, edict_t* edict, CBaseEntity* entity) {
			if (entity != nullptr)
			{
				Vector end = UtilHelpers::getWorldSpaceCenter(entity);

				float distance = (end - start).Length();

				if (distance < best)
				{
					best = distance;
					nearest = entity;
				}
			}

			return true;
		});

		this->handle = nearest;
	}

	if (!noerror && !this->handle.IsValid())
	{
		Warning("CNavElevator::ElevatorEntity::SearchForEntity failed to find entity! <%s> <%s>\n", this->classname.c_str(), this->targetname.c_str());
	}
}

void CNavElevator::ElevatorEntity::AssignEntity(CBaseEntity* entity)
{
	if (entity == nullptr)
	{
		this->handle = nullptr;
		this->position.Init(0.0f, 0.0f, 0.0f);
		this->classname.clear();
		this->targetname.clear();
		return;
	}

	entities::HBaseEntity be(entity);

	const char* classname = gamehelpers->GetEntityClassname(entity);

	std::unique_ptr<char[]> targetname = std::make_unique<char[]>(256);

	this->handle = entity;
	this->position = UtilHelpers::getWorldSpaceCenter(entity);
	this->classname.assign(classname);

	if (be.GetTargetName(targetname.get(), 256) > 0)
	{
		this->targetname.assign(targetname.get());
	}
	else
	{
		this->targetname.clear();
	}
}

void CNavElevator::ElevatorFloor::ConvertAreaIDToPointer()
{
	unsigned int id = std::get<unsigned int>(this->floor_area);
	CNavArea* area = TheNavMesh->GetNavAreaByID(id);
	this->floor_area = area;
}

void CNavElevator::ElevatorFloor::Save(std::fstream& filestream, uint32_t version)
{
	this->use_button.Save(filestream, version);
	this->call_button.Save(filestream, version);

	unsigned int id = this->GetArea()->GetID();
	filestream.write(reinterpret_cast<char*>(&id), sizeof(unsigned int));
	filestream.write(reinterpret_cast<char*>(&this->floor_position), sizeof(Vector));
	filestream.write(reinterpret_cast<char*>(&this->wait_position), sizeof(Vector));
	filestream.write(reinterpret_cast<char*>(&this->shootable_button), sizeof(bool));
	filestream.write(reinterpret_cast<char*>(&this->toggle_state), sizeof(int));
}

void CNavElevator::ElevatorFloor::Load(std::fstream& filestream, uint32_t version, uint32_t subVersion)
{
	this->use_button.Load(filestream, version, subVersion);
	this->call_button.Load(filestream, version, subVersion);

	unsigned int id = 0;
	filestream.read(reinterpret_cast<char*>(&id), sizeof(unsigned int));
	this->floor_area = id;
	filestream.read(reinterpret_cast<char*>(&this->floor_position), sizeof(Vector));
	filestream.read(reinterpret_cast<char*>(&this->wait_position), sizeof(Vector));
	filestream.read(reinterpret_cast<char*>(&this->shootable_button), sizeof(bool));
	filestream.read(reinterpret_cast<char*>(&this->toggle_state), sizeof(int));
}

void CNavElevator::ElevatorFloor::PostLoad()
{
	this->use_button.PostLoad();
	this->call_button.PostLoad();
	this->ConvertAreaIDToPointer();
}

float CNavElevator::ElevatorFloor::GetDistanceTo(const ElevatorFloor* other) const
{
	return (this->GetArea()->GetCenter() - other->GetArea()->GetCenter()).Length();
}
