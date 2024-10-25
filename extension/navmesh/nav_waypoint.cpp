#include <filesystem>
#include <fstream>

#include <extension.h>
#include <bot/basebot.h>
#include <sdkports/debugoverlay_shared.h>
#include "nav_mesh.h"
#include "nav_area.h"
#include "nav_waypoint.h"

WaypointID CWaypoint::g_NextWaypointID = 0;

void WaypointConnect::ConvertFromIDtoPointer()
{
	if (std::holds_alternative<WaypointID>(connection))
	{
		auto wpt = TheNavMesh->GetWaypointOfID<CWaypoint>(std::get<WaypointID>(connection));

		if (wpt.has_value())
		{
			connection = wpt->get();
		}
	}
}

CWaypoint::CWaypoint()
{
	m_ID = g_NextWaypointID; // Assing the next ID to this instance
	g_NextWaypointID++; // Increment ID

	for (auto& angle : m_aimAngles)
	{
		angle.Init(0.0f, 0.0f, 0.0f);
	}

	m_flags = 0;
	m_numAimAngles = 0;
	m_teamNum = NAV_TEAM_ANY;
	m_user.Term();
}

CWaypoint::~CWaypoint()
{
}

void CWaypoint::Reset()
{
	m_user = nullptr;
	m_expireUserTimer.Invalidate();
}

void CWaypoint::Update()
{
	if (m_expireUserTimer.HasStarted())
	{
		// Bot that was using this waypoint is no longer valid, probably kicked from the server.
		if (m_user.Get() == nullptr)
		{
			m_user.Term();
			m_expireUserTimer.Invalidate();
			OnStopUse();
		}

		// Use timer expired
		if (m_expireUserTimer.IsElapsed())
		{
			m_user = nullptr;
			m_expireUserTimer.Invalidate();
			OnStopUse();
		}
	}
}

bool CWaypoint::CanBeUsedByBot(CBaseBot* bot) const
{
	return m_expireUserTimer.HasStarted() == false;
}

void CWaypoint::Save(std::fstream& filestream, uint32_t version)
{
	filestream.write(reinterpret_cast<char*>(&m_ID), sizeof(WaypointID));
	filestream.write(reinterpret_cast<char*>(&m_origin), sizeof(Vector));

	for (auto& angle : m_aimAngles)
	{
		filestream.write(reinterpret_cast<char*>(&angle), sizeof(QAngle));
	}

	filestream.write(reinterpret_cast<char*>(&m_numAimAngles), sizeof(int));
	filestream.write(reinterpret_cast<char*>(&m_flags), sizeof(int));
	filestream.write(reinterpret_cast<char*>(&m_teamNum), sizeof(int));
	filestream.write(reinterpret_cast<char*>(&m_radius), sizeof(float));

	{
		uint64_t size = static_cast<uint64_t>(m_connections.size());
		filestream.write(reinterpret_cast<char*>(&size), sizeof(uint64_t));

		for (auto& connect : m_connections)
		{
			WaypointID id = connect.GetOther()->GetID();
			filestream.write(reinterpret_cast<char*>(&id), sizeof(WaypointID));
		}
	}
}

NavErrorType CWaypoint::Load(std::fstream& filestream, uint32_t version, uint32_t subVersion)
{
	if (!filestream.good())
	{
		return NAV_CORRUPT_DATA;
	}

	filestream.read(reinterpret_cast<char*>(&m_ID), sizeof(WaypointID));
	filestream.read(reinterpret_cast<char*>(&m_origin), sizeof(Vector));

	for (std::size_t i = 0; i < CWaypoint::MAX_AIM_ANGLES; i++)
	{
		QAngle angle;
		filestream.read(reinterpret_cast<char*>(&angle), sizeof(QAngle));
		m_aimAngles[i] = angle;
	}

	filestream.read(reinterpret_cast<char*>(&m_numAimAngles), sizeof(int));
	filestream.read(reinterpret_cast<char*>(&m_flags), sizeof(int));
	filestream.read(reinterpret_cast<char*>(&m_teamNum), sizeof(int));
	filestream.read(reinterpret_cast<char*>(&m_radius), sizeof(float));

	{
		uint64_t size = 0;
		filestream.read(reinterpret_cast<char*>(&size), sizeof(uint64_t));

		for (uint64_t i = 0; i < size; i++)
		{
			WaypointID id = 0;
			filestream.read(reinterpret_cast<char*>(&id), sizeof(WaypointID));

			auto& connect = m_connections.emplace_back();
			connect.connection = id;
		}
	}

	if (!filestream.good())
	{
		return NAV_CORRUPT_DATA;
	}

	return NAV_OK;
}

NavErrorType CWaypoint::PostLoad()
{
	for (auto& connect : m_connections)
	{
		connect.ConvertFromIDtoPointer();
	}

	return NAV_OK;
}

void CWaypoint::Draw() const
{
	int r = 0;
	int g = 0;
	int b = 255;

	if (TheNavMesh->GetSelectedWaypoint().get() == this)
	{
		r = 255;
		g = 255;
	}

	NDebugOverlay::Line(m_origin, m_origin + Vector(0.0f, 0.0f, CWaypoint::WAYPOINT_HEIGHT), r, g, b, false, NDEBUG_PERSIST_FOR_ONE_TICK);

	for (int i = 0; i < m_numAimAngles; i++)
	{
		const QAngle& angle = m_aimAngles[i];
		Vector forward;

		AngleVectors(angle, &forward);
		forward.NormalizeInPlace();
		Vector start = m_origin + Vector(0.0f, 0.0f, CWaypoint::WAYPOINT_AIM_HEIGHT);
		Vector end = start + (forward * CWaypoint::WAYPOINT_AIM_LENGTH);

		NDebugOverlay::Line(start, end, 0, 0, 255, false, NDEBUG_PERSIST_FOR_ONE_TICK);
	}

	if (m_radius >= 0.98f)
	{
		Vector mins{ -m_radius, m_radius, 0.0f };
		Vector maxs{ m_radius, m_radius, CWaypoint::WAYPOINT_HEIGHT };

		NDebugOverlay::Box(m_origin, mins, maxs, r, g, b, 150, NDEBUG_PERSIST_FOR_ONE_TICK);
	}

	constexpr std::size_t size = 512;
	std::unique_ptr<char[]> text = std::make_unique<char[]>(size);

	ke::SafeSprintf(text.get(), size, "Waypoint #%i\nTeam #%i", m_ID, m_teamNum);

	NDebugOverlay::Text(m_origin + Vector(0.0f, 0.0f, CWaypoint::WAYPOINT_TEXT_HEIGHT), text.get(), false, NDEBUG_PERSIST_FOR_ONE_TICK);
}

void CWaypoint::Use(CBaseBot* user, const float duration) const
{
	m_user = user->GetEntity();
	m_expireUserTimer.Start(duration);
	OnUse(user);
}

bool CWaypoint::IsBeingUsed() const
{
	return m_expireUserTimer.HasStarted() && !m_expireUserTimer.IsElapsed();
}

void CWaypoint::StopUsing() const
{
	m_user = nullptr;
	m_expireUserTimer.Invalidate();
	OnStopUse();
}

const QAngle& CWaypoint::GetAngle(std::size_t index) const
{
	return m_aimAngles.at(index);
}

void CWaypoint::ConnectTo(const CWaypoint* other)
{
	if (this == other)
	{
		Warning("CWaypoint::ConnectTo can't add connection to self! \n");
		return;
	}

	for (auto& connect : m_connections)
	{
		if (connect.GetOther() == other)
		{
			Warning("CWaypoint::ConnectTo error. Waypoint %i is already connected to %i", this->GetID(), other->GetID());
			return; // Already connected
		}
	}

	m_connections.emplace_back(const_cast<CWaypoint*>(other));
}

void CWaypoint::DisconnectFrom(const CWaypoint* other)
{
	m_connections.erase(std::remove_if(m_connections.begin(), m_connections.end(), [&other](const WaypointConnect& object) {
		return object.GetOther() == other;
	}));
}

bool CWaypoint::IsConnectedTo(const CWaypoint* other) const
{
	for (auto& connect : m_connections)
	{
		if (connect.GetOther() == other)
		{
			return true;
		}
	}

	return false;
}

bool CWaypoint::IsConnectedTo(WaypointID id) const
{
	for (auto& connect : m_connections)
	{
		if (connect.GetOther()->GetID() == id)
		{
			return true;
		}
	}

	return false;
}

void CWaypoint::NotifyWaypointDestruction(const CWaypoint* other)
{
	DisconnectFrom(other);
}
