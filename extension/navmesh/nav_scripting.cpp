#include NAVBOT_PCH_FILE
#include <array>
#include <string_view>
#include <memory>
#include <cinttypes>
#include <algorithm>
#include <extension.h>
#include <util/entprops.h>
#include <sdkports/debugoverlay_shared.h>
#include <sdkports/sdk_traces.h>
#include "nav.h"
#include "nav_scripting.h"

// undef some stuff from mathlib that causes issues when using STL

#undef min
#undef max
#undef clamp

void navscripting::EntityLink::Save(std::fstream& filestream, uint32_t version)
{
	bool hasentity = !m_classname.empty();
	filestream.write(reinterpret_cast<char*>(&hasentity), sizeof(bool));

	if (hasentity)
	{
		std::uint64_t strlength = static_cast<uint64_t>(m_classname.size()) + 1U;
		filestream.write(reinterpret_cast<char*>(&strlength), sizeof(std::uint64_t));
		filestream.write(m_classname.c_str(), strlength);
		filestream.write(reinterpret_cast<char*>(&m_position), sizeof(Vector));

		bool hastargetname = !m_targetname.empty();
		filestream.write(reinterpret_cast<char*>(&hastargetname), sizeof(bool));
		
		if (hastargetname)
		{
			strlength = static_cast<uint64_t>(m_targetname.size()) + 1U;
			filestream.write(reinterpret_cast<char*>(&strlength), sizeof(std::uint64_t));
			filestream.write(m_targetname.c_str(), strlength);
		}
	}
}

void navscripting::EntityLink::Load(std::fstream& filestream, uint32_t version)
{
	bool hasentity = false;
	filestream.read(reinterpret_cast<char*>(&hasentity), sizeof(bool));

	if (hasentity)
	{
		std::uint64_t strlength = 0;
		filestream.read(reinterpret_cast<char*>(&strlength), sizeof(std::uint64_t));
		std::unique_ptr<char[]> classname = std::make_unique<char[]>(strlength + 2U);
		filestream.read(classname.get(), strlength);
		filestream.read(reinterpret_cast<char*>(&m_position), sizeof(Vector));
		m_classname.assign(classname.get());

		bool hastargetname = false;
		filestream.read(reinterpret_cast<char*>(&hastargetname), sizeof(bool));

		if (hastargetname)
		{
			strlength = 0;
			filestream.read(reinterpret_cast<char*>(&strlength), sizeof(std::uint64_t));
			std::unique_ptr<char[]> targetname = std::make_unique<char[]>(strlength + 2U);
			filestream.read(targetname.get(), strlength);
			m_targetname.assign(targetname.get());
		}
	}
}

void navscripting::EntityLink::PostLoad()
{
}

void navscripting::EntityLink::DebugDraw() const
{
	if (m_hEntity.IsValid())
	{
		CBaseEntity* pEnt = m_hEntity.Get();

		if (pEnt)
		{
			NDebugOverlay::EntityBounds(pEnt, 0, 255, 255, 128, NDEBUG_PERSIST_FOR_ONE_TICK);
		}
	}
}

void navscripting::EntityLink::LinkToEntity(CBaseEntity* entity)
{
	if (!entity)
	{
		m_hEntity = nullptr;
		m_classname.clear();
		m_targetname.clear();
		m_position.Init(0.0f, 0.0f, 0.0f);
		return;
	}

	m_hEntity = entity;
	const char* classname = entityprops::GetEntityClassname(entity);
	
	if (classname && classname[0] != '\0')
	{
		m_classname.assign(classname);
	}
	else
	{
		int ref = gamehelpers->EntityToBCompatRef(entity);
		Warning("[NAVBOT]: navscripting::EntityLink::LinkToEntity -- Failed to get classname for entity #%i\n", ref);
	}

	const char* targetname = entityprops::GetEntityTargetname(entity);

	if (targetname && targetname[0] != '\0')
	{
		m_targetname.assign(targetname);
	}

	m_position = UtilHelpers::getWorldSpaceCenter(entity);
}

void navscripting::EntityLink::FindLinkedEntity()
{
	m_hEntity.Term();

	if (m_classname.empty())
	{
		return;
	}

	if (!m_targetname.empty())
	{
		int index = UtilHelpers::FindEntityByTargetname(-1, m_targetname.c_str());

		if (index != INVALID_EHANDLE_INDEX)
		{
			CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(index);

			if (pEntity)
			{
				SetActiveEntity(pEntity);
			}
		}
		else
		{
			smutils->LogError(myself, "FindLinkedEntity: No entity of targetname \"%s\"!", m_targetname.c_str());
		}
	}

	// either search by targetname failed or the entity lacks one
	if (!m_hEntity.IsValid())
	{
		CBaseEntity* best = nullptr;
		float nearest = 99999999.0f;
		const Vector& start = m_position;
		auto functor = [&nearest, &best, &start](int index, edict_t* edict, CBaseEntity* entity) {
			if (entity)
			{
				Vector end = UtilHelpers::getWorldSpaceCenter(entity);
				float distance = (start - end).Length();

				if (distance < nearest)
				{
					best = entity;
					distance = nearest;
				}
			}

			return true;
		};

		UtilHelpers::ForEachEntityOfClassname(m_classname.c_str(), functor);

		if (!best)
		{
			return;
		}

		SetActiveEntity(best);
	}

	if (!m_hEntity.IsValid() && !m_classname.empty())
	{
		smutils->LogError(myself, "Nav: Entity Link failed to find entity! classname: \"%s\" targetname \"%s\"", m_classname.c_str(), m_targetname.c_str());
	}
}

const char* navscripting::ToggleCondition::TCTypeToString(navscripting::ToggleCondition::TCTypes type)
{
	using namespace std::literals::string_view_literals;

	constexpr std::array names = {
		"TYPE_NOT_SET"sv,
		"TYPE_ENTITY_EXISTS"sv,
		"TYPE_ENTITY_ALIVE"sv,
		"TYPE_ENTITY_ENABLED"sv,
		"TYPE_ENTITY_LOCKED"sv,
		"TYPE_ENTITY_TEAM"sv,
		"TYPE_ENTITY_TOGGLE_STATE"sv,
		"TYPE_ENTITY_DISTANCE"sv,
		"TYPE_ENTITY_VISIBLE"sv,
		"TYPE_TRACEHULL_SOLIDWORLD"sv,
		"TYPE_BASETOGGLE_POSITION"sv,
	};

	static_assert(names.size() == static_cast<std::size_t>(navscripting::ToggleCondition::TCTypes::MAX_TOGGLE_TYPES), "Toggle Type name array and enum size mismatch!");

	return names[static_cast<std::size_t>(type)].data();
}

void navscripting::ToggleCondition::Save(std::fstream& filestream, uint32_t version)
{
	m_targetEnt.Save(filestream, version);
	filestream.write(reinterpret_cast<char*>(&m_toggle_type), sizeof(TCTypes));
	filestream.write(reinterpret_cast<char*>(&m_iData), sizeof(int));
	filestream.write(reinterpret_cast<char*>(&m_flData), sizeof(float));
	filestream.write(reinterpret_cast<char*>(&m_vecData), sizeof(Vector));
	filestream.write(reinterpret_cast<char*>(&m_inverted), sizeof(bool));
}

void navscripting::ToggleCondition::Load(std::fstream& filestream, uint32_t version)
{
	m_targetEnt.Load(filestream, version);
	filestream.read(reinterpret_cast<char*>(&m_toggle_type), sizeof(TCTypes));
	filestream.read(reinterpret_cast<char*>(&m_iData), sizeof(int));
	filestream.read(reinterpret_cast<char*>(&m_flData), sizeof(float));
	filestream.read(reinterpret_cast<char*>(&m_vecData), sizeof(Vector));
	filestream.read(reinterpret_cast<char*>(&m_inverted), sizeof(bool));
}

void navscripting::ToggleCondition::PostLoad()
{
	m_targetEnt.PostLoad();
}

bool navscripting::ToggleCondition::RunTestCondition() const
{
	bool result = false;

	switch (m_toggle_type)
	{
	case navscripting::ToggleCondition::TYPE_NOT_SET:
		return true;
	case navscripting::ToggleCondition::TYPE_ENTITY_EXISTS:
		result = TestCondition_EntityExists();
		break;
	case navscripting::ToggleCondition::TYPE_ENTITY_ALIVE:
		result = TestCondition_EntityAlive();
		break;
	case navscripting::ToggleCondition::TYPE_ENTITY_ENABLED:
		result = TestCondition_EntityEnabled();
		break;
	case navscripting::ToggleCondition::TYPE_ENTITY_LOCKED:
		result = TestCondition_EntityLocked();
		break;
	case navscripting::ToggleCondition::TYPE_ENTITY_TEAM:
		result = TestCondition_EntityTeam();
		break;
	case navscripting::ToggleCondition::TYPE_ENTITY_TOGGLE_STATE:
		result = TestCondition_EntityToggleState();
		break;
	case navscripting::ToggleCondition::TYPE_ENTITY_DISTANCE:
		result = TestCondition_EntityDistance();
		break;
	case navscripting::ToggleCondition::TYPE_ENTITY_VISIBLE:
		result = TestCondition_EntityVisible();
		break;
	case navscripting::ToggleCondition::TYPE_TRACEHULL_SOLIDWORLD:
		result = TestCondition_TraceHullSolidWorld();
		break;
	case navscripting::ToggleCondition::TYPE_BASETOGGLE_POSITION:
		result = TestCondition_BaseToggle_Position();
		break;
	case navscripting::ToggleCondition::MAX_TOGGLE_TYPES:
		[[fallthrough]];
	default:
#ifdef EXT_DEBUG
		smutils->LogError(myself, "navscripting::ToggleCondition::RunTestCondition() running default: for type %i", static_cast<int>(m_toggle_type));
#endif // EXT_DEBUG
		return false;
	}

	if (m_inverted)
	{
		return !result;
	}

	return result;
}

void navscripting::ToggleCondition::DebugScreenOverlay(const float x, const float y, float duration, int r, int g, int b, int a) const
{
	NDebugOverlay::ScreenText(x, y, "--- TOGGLE CONDITION ---", r, g, b, a, duration);

	NDebugOverlay::ScreenText(x, y + 0.04f, r, g, b, a, duration, "Type: %s int: %i float: %3.2f Inverted: %s", 
		GetConditionName(), m_iData, m_flData, m_inverted ? "YES" : "NO");

	NDebugOverlay::ScreenText(x, y + 0.08f, r, g, b, a, duration, "classname: %s targetname: %s", 
		m_targetEnt.GetSavedClassname().c_str(), m_targetEnt.GetSavedTargetname().c_str());
}

void navscripting::ToggleCondition::OnTestConditionChanged() const
{
	CBaseEntity* entity = m_targetEnt.GetEntity();

	if (!entity)
	{
		return;
	}

	int index = gamehelpers->EntityToBCompatRef(entity);

	if (m_toggle_type == TCTypes::TYPE_ENTITY_ENABLED)
	{
		if (!entprops->HasEntProp(index, Prop_Data, "m_bDisabled"))
		{
			rootconsole->ConsolePrint("nav scripting: Toggle type set to entity enabled but target entity doesn't have \"m_bDisabled\" property!");
		}
	}
	else if (m_toggle_type == TCTypes::TYPE_ENTITY_LOCKED)
	{
		if (!entprops->HasEntProp(index, Prop_Data, "m_bLocked"))
		{
			rootconsole->ConsolePrint("nav scripting: Toggle type set to entity locked but target entity doesn't have \"m_bLocked\" property!");
		}
	}
	else if (m_toggle_type == TCTypes::TYPE_ENTITY_TEAM)
	{
		if (!entprops->HasEntProp(index, Prop_Data, "m_iTeamNum"))
		{
			rootconsole->ConsolePrint("nav scripting: Toggle type set to entity team but target entity doesn't have \"m_iTeamNum\" property!");
		}
	}
	else if (m_toggle_type == TCTypes::TYPE_ENTITY_TOGGLE_STATE)
	{
		if (!entprops->HasEntProp(index, Prop_Data, "m_toggle_state"))
		{
			rootconsole->ConsolePrint("nav scripting: Toggle type set to entity toggle state but target entity doesn't have \"m_toggle_state\" property!");
		}
	}
	else if (m_toggle_type == TCTypes::TYPE_TRACEHULL_SOLIDWORLD)
	{
		if (m_flData < 1.0f)
		{
			rootconsole->ConsolePrint("nav scripting: Toggle type set to trace hull solid world but hull size (float data) is less than 1!");
		}
	}
	else if (m_toggle_type == TCTypes::TYPE_BASETOGGLE_POSITION)
	{
		if (!entprops->HasEntProp(entity, Prop_Data, "m_vecPosition1") || !entprops->HasEntProp(entity, Prop_Data, "m_vecPosition2"))
		{
			rootconsole->ConsolePrint("nav scripting: Toggle type set to base toggle position but the target entity isn't an instance of \"CBaseToggle\"!");
		}
	}
}

bool navscripting::ToggleCondition::TestCondition_EntityAlive() const
{
	CBaseEntity* pEntity = m_targetEnt.GetEntity();

	if (pEntity)
	{
		return UtilHelpers::IsEntityAlive(pEntity);
	}

	return false;
}

bool navscripting::ToggleCondition::TestCondition_EntityEnabled() const
{
	CBaseEntity* pEntity = m_targetEnt.GetEntity();

	if (pEntity)
	{
		int index = gamehelpers->EntityToBCompatRef(pEntity);

		bool disabled = false;
		entprops->GetEntPropBool(index, Prop_Data, "m_bDisabled", disabled);

		return !disabled; // true when disabled, invert so it returns false when it's disabled
	}

	return false;
}

bool navscripting::ToggleCondition::TestCondition_EntityLocked() const
{
	CBaseEntity* pEntity = m_targetEnt.GetEntity();

	if (pEntity)
	{
		int index = gamehelpers->EntityToBCompatRef(pEntity);

		bool locked = false;
		entprops->GetEntPropBool(index, Prop_Data, "m_bLocked", locked);

		return !locked; // return false if locked
	}

	return false;
}

bool navscripting::ToggleCondition::TestCondition_EntityTeam() const
{
	CBaseEntity* pEntity = m_targetEnt.GetEntity();

	if (pEntity)
	{
		int index = gamehelpers->EntityToBCompatRef(pEntity);

		int team = NAV_TEAM_ANY;
		entprops->GetEntProp(index, Prop_Data, "m_iTeamNum", team);
		return team == m_iData;
	}

	return false;
}

bool navscripting::ToggleCondition::TestCondition_EntityToggleState() const
{
	CBaseEntity* pEntity = m_targetEnt.GetEntity();

	if (pEntity)
	{
		int index = gamehelpers->EntityToBCompatRef(pEntity);

		int toggle_state = -1;
		entprops->GetEntProp(index, Prop_Data, "m_toggle_state", toggle_state);
		return toggle_state == m_iData;
	}

	return false;
}

bool navscripting::ToggleCondition::TestCondition_EntityDistance() const
{
	CBaseEntity* pEntity = m_targetEnt.GetEntity();

	if (pEntity)
	{
		// Needs to be world space center since a brush entity origin will most likely be at 0,0,0
		const Vector origin = UtilHelpers::getWorldSpaceCenter(pEntity);
		float range = (m_vecData - origin).Length();

		return range < m_flData;
	}

	return false;
}

bool navscripting::ToggleCondition::TestCondition_EntityVisible() const
{
	CBaseEntity* pEntity = m_targetEnt.GetEntity();

	if (pEntity)
	{
		int index = gamehelpers->EntityToBCompatRef(pEntity);

		int effects = 0;
		entprops->GetEntProp(index, Prop_Data, "m_fEffects", effects);

		if ((effects & EF_NODRAW) != 0)
		{
			return false; // nodraw set, entity is not visible
		}

		return true; // entity is visible
	}

	return false;
}

bool navscripting::ToggleCondition::TestCondition_TraceHullSolidWorld() const
{
	CTraceFilterWorldAndPropsOnly filter;

	// minimum value of 1 for size
	float size = std::max(m_flData, 1.0f);
	Vector mins(-size, -size, -size);
	Vector maxs(size, size, size);
	trace_t tr;
	trace::hull(m_vecData, m_vecData, mins, maxs, MASK_SOLID, &filter, tr);

	if (m_inverted)
	{
		return !tr.DidHit();
	}

	return tr.DidHit();
}

bool navscripting::ToggleCondition::TestCondition_BaseToggle_Position() const
{
	CBaseEntity* pEntity = m_targetEnt.GetEntity();

	if (pEntity)
	{
		Vector position1;
		Vector position2;
		Vector local = UtilHelpers::getEntityLocalOrigin(pEntity);

		entprops->GetEntPropVector(pEntity, Prop_Data, "m_vecPosition1", position1);
		entprops->GetEntPropVector(pEntity, Prop_Data, "m_vecPosition2", position2);

		float flTravelDist = (position1 - position2).Length();
		float flCurDist = (position1 - local).Length();
		float position = flCurDist / flTravelDist;

		if (m_inverted)
		{
			return m_flData <= position;
		}

		return m_flData >= position;
	}

	return false;
}
