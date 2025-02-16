#include <array>
#include <string_view>
#include <extension.h>
#include <sdkports/debugoverlay_shared.h>
#include "nav_mesh.h"
#include "nav_area.h"
#include "nav_prereq.h"

ConVar sm_nav_prerequisite_edit("sm_nav_prerequisite_edit", "0", FCVAR_GAMEDLL, "Controls the Nav Prerequisite edit mode.");

CNavPrerequisite::CNavPrerequisite() :
	m_goalPosition(0.0f, 0.0f, 0.0f), m_mins(-32.0f, -32.0f, 0.0f), m_maxs(32.0f, 32.0f, 72.0f), m_origin(0.0f, 0.0f, 0.0f)
{
	m_id = CNavPrerequisite::s_nextID;
	CNavPrerequisite::s_nextID++;
	m_task = TASK_NONE;
	m_flData = 0.0f;
	m_teamIndex = NAV_TEAM_ANY;
}

CNavPrerequisite::~CNavPrerequisite()
{
	for (auto area : m_areas)
	{
		area->NotifyPrerequisiteDestruction(this);
	}
}

const char* CNavPrerequisite::TaskIDtoString(PrerequisiteTask task)
{
	using namespace std::literals::string_view_literals;

	constexpr std::array names = {
		"TASK_NONE"sv,
		"TASK_WAIT"sv,
		"TASK_MOVE_TO_POS"sv,
		"TASK_DESTROY_ENT"sv,
		"TASK_USE_ENT"sv,
	};

	static_assert(names.size() == static_cast<std::size_t>(PrerequisiteTask::MAX_TASK_TYPES), "Prerequisite name array size and enum mismatch!");

	return names[static_cast<std::size_t>(task)].data();
}

void CNavPrerequisite::Save(std::fstream& filestream, uint32_t version)
{
	m_goalEntity.Save(filestream, version);
	m_toggle_condition.Save(filestream, version);
	filestream.write(reinterpret_cast<char*>(&m_id), sizeof(unsigned int));
	filestream.write(reinterpret_cast<char*>(&m_origin), sizeof(Vector));
	filestream.write(reinterpret_cast<char*>(&m_mins), sizeof(Vector));
	filestream.write(reinterpret_cast<char*>(&m_maxs), sizeof(Vector));
	filestream.write(reinterpret_cast<char*>(&m_task), sizeof(PrerequisiteTask));
	filestream.write(reinterpret_cast<char*>(&m_goalPosition), sizeof(Vector));
	filestream.write(reinterpret_cast<char*>(&m_flData), sizeof(float));
	filestream.write(reinterpret_cast<char*>(&m_teamIndex), sizeof(int));
}

NavErrorType CNavPrerequisite::Load(std::fstream& filestream, uint32_t version, uint32_t subVersion)
{
	m_goalEntity.Load(filestream, version);
	m_toggle_condition.Load(filestream, version);
	filestream.read(reinterpret_cast<char*>(&m_id), sizeof(unsigned int));
	filestream.read(reinterpret_cast<char*>(&m_origin), sizeof(Vector));
	filestream.read(reinterpret_cast<char*>(&m_mins), sizeof(Vector));
	filestream.read(reinterpret_cast<char*>(&m_maxs), sizeof(Vector));
	filestream.read(reinterpret_cast<char*>(&m_task), sizeof(PrerequisiteTask));
	filestream.read(reinterpret_cast<char*>(&m_goalPosition), sizeof(Vector));
	filestream.read(reinterpret_cast<char*>(&m_flData), sizeof(float));
	filestream.read(reinterpret_cast<char*>(&m_teamIndex), sizeof(int));

	if (!filestream.good())
	{
		return NAV_CORRUPT_DATA;
	}

	return NAV_OK;
}

NavErrorType CNavPrerequisite::PostLoad(void)
{
	m_goalEntity.PostLoad();
	m_toggle_condition.PostLoad();
	m_calculatedMins = (m_origin + m_mins);
	m_calculatedMaxs = (m_origin + m_maxs);
	SearchForNavAreas();

	return NAV_OK;
}

void CNavPrerequisite::OnRoundRestart()
{
	m_goalEntity.OnRoundRestart();
	m_toggle_condition.OnRoundRestart();
}

bool CNavPrerequisite::IsPointInsideVolume(const Vector& point) const
{
	return UtilHelpers::PointIsInsideAABB(point, m_calculatedMins, m_calculatedMaxs);
}

void CNavPrerequisite::Draw() const
{
	if (TheNavMesh->GetSelectedPrerequisite().get() == this)
	{
		NDebugOverlay::Box(m_origin, m_mins, m_maxs, 255, 215, 0, 100, NDEBUG_PERSIST_FOR_ONE_TICK);
		NDebugOverlay::Text(m_origin, false, NDEBUG_PERSIST_FOR_ONE_TICK, "MARKED Nav Prerequisite #%i", m_id);
		NDebugOverlay::Cross3D(m_goalPosition, 16.0f, 255, 0, 0, true, NDEBUG_PERSIST_FOR_ONE_TICK);

		CBaseEntity* goalEnt = m_goalEntity.GetEntity();

		if (goalEnt)
		{
			NDebugOverlay::EntityBounds(goalEnt, 0, 128, 0, 128, NDEBUG_PERSIST_FOR_ONE_TICK);
		}

		CBaseEntity* toggleEnt = m_toggle_condition.GetTargetEntity();

		if (toggleEnt)
		{
			NDebugOverlay::EntityBounds(toggleEnt, 0, 128, 200, 128, NDEBUG_PERSIST_FOR_ONE_TICK);
		}

		const Vector& vec = m_toggle_condition.GetVectorData();
		NDebugOverlay::Cross3D(vec, 16.0f, 0, 128.0f, 0, true, NDEBUG_PERSIST_FOR_ONE_TICK);
	}
	else
	{
		NDebugOverlay::Box(m_origin, m_mins, m_maxs, 135, 206, 250, 100, NDEBUG_PERSIST_FOR_ONE_TICK);
		NDebugOverlay::Text(m_origin, false, NDEBUG_PERSIST_FOR_ONE_TICK, "Nav Prerequisite #%i", m_id);
	}
}

void CNavPrerequisite::ScreenText() const
{
	NDebugOverlay::ScreenText(BASE_SCREENX, BASE_SCREENY, 255, 255, 0, 255, NDEBUG_PERSIST_FOR_ONE_TICK, "Selected Prerequisite #%i Team %i %s %s", 
		m_id, m_teamIndex, TaskIDtoString(m_task), IsEnabled() ? "ENABLED" : "DISABLED");

	m_toggle_condition.DebugScreenOverlay(BASE_SCREENX, BASE_SCREENY + 0.04f, NDEBUG_PERSIST_FOR_ONE_TICK);
}

void CNavPrerequisite::DrawAreas() const
{
	for (auto area : m_areas)
	{
		area->DrawFilled(255, 0, 0, 200, 10.0f);
	}
}

void CNavPrerequisite::SearchForNavAreas()
{
	for (auto area : m_areas)
	{
		area->NotifyPrerequisiteDestruction(this);
	}

	m_areas.clear();

	auto findareas = [this](CNavArea* area) {

		Vector pos = area->GetCenter();
		pos.z += navgenparams->step_height * 0.5f;

		if (this->IsPointInsideVolume(pos))
		{
			this->m_areas.push_back(area);
			area->SetPrerequisite(this);
		}

		return true;
	};

	CNavMesh::ForAllAreas<decltype(findareas)>(findareas);
}
