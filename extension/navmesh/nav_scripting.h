#ifndef NAV_MESH_SCRIPTING_H_
#define NAV_MESH_SCRIPTING_H_

#include <string>
#include <fstream>
#include <sdkports/sdk_ehandle.h>

/**
 * @brief Namespace for navigation mesh scripting utils
 */
namespace navscripting
{
	/**
	 * @brief Links to a game entity with save/load support.
	 */
	class EntityLink
	{
	public:
		EntityLink() :
			m_hEntity(nullptr), m_position(0.0f, 0.0f, 0.0f)
		{
			m_classname.reserve(64);
			m_targetname.reserve(64);
		}

		~EntityLink()
		{
		}

		void Save(std::fstream& filestream, uint32_t version);
		void Load(std::fstream& filestream, uint32_t version);
		void PostLoad();
		void OnRoundRestart()
		{
			FindLinkedEntity();
		}
		void DebugDraw() const;

		// Links to an entity or NULL to clear the linked entity.
		void LinkToEntity(CBaseEntity* entity);
		// Searches for the linked entity.
		void FindLinkedEntity();

		CBaseEntity* GetEntity() const { return m_hEntity.Get(); }
		const Vector& GetSavedEntityPosition() const { return m_position; }
		void clear()
		{
			m_hEntity.Term();
			m_classname.clear();
			m_targetname.clear();
			m_position = vec3_origin;
		}

		// returns true if an entity has been linked. Does not means the entity will be non-NULL.
		bool HasSavedEntity() const { return !m_classname.empty(); }
		const std::string& GetSavedClassname() const { return m_classname; }
		const std::string& GetSavedTargetname() const { return m_targetname; }

	private:
		CHandle<CBaseEntity> m_hEntity; // entity handle
		std::string m_classname; // entity classname for searching
		std::string m_targetname; // entity targetname for searching
		Vector m_position; // entity position for searching

		// Sets the current entity without linking
		void SetActiveEntity(CBaseEntity* entity) { m_hEntity = entity; }
	};
	
	/**
	 * @brief Handles a toggle condition in scripting context.
	 */
	class ToggleCondition
	{
	public:
		
		/**
		 * @brief Toggle Condition Types
		 */
		enum TCTypes : int
		{
			TYPE_NOT_SET = 0,
			TYPE_ENTITY_EXISTS,
			TYPE_ENTITY_ALIVE,
			TYPE_ENTITY_ENABLED,
			TYPE_ENTITY_LOCKED,
			TYPE_ENTITY_TEAM,
			TYPE_ENTITY_TOGGLE_STATE,
			TYPE_ENTITY_DISTANCE,
			TYPE_ENTITY_VISIBLE,
			TYPE_TRACEHULL_SOLIDWORLD,

			MAX_TOGGLE_TYPES
		};

		static const char* TCTypeToString(navscripting::ToggleCondition::TCTypes type);

		ToggleCondition() :
			m_vecData(0.0f, 0.0f, 0.0f)
		{
			m_toggle_type = TCTypes::TYPE_NOT_SET;
			m_iData = 0;
			m_flData = 0.0f;
			m_inverted = false;
		}

		~ToggleCondition()
		{
		}

		void Save(std::fstream& filestream, uint32_t version);
		void Load(std::fstream& filestream, uint32_t version);
		void PostLoad();
		void OnRoundRestart()
		{
			m_targetEnt.FindLinkedEntity();
		}

		void SearchForEntities() { m_targetEnt.FindLinkedEntity(); }
		void SetTargetEntity(CBaseEntity* entity) { m_targetEnt.LinkToEntity(entity); }
		void SetIntegerData(int value) { m_iData = value; }
		void SetFloatData(float value) { m_flData = value; }
		void SetVectorData(const Vector& value) { m_vecData = value; }
		void SetToggleConditionType(TCTypes type) { m_toggle_type = type; OnTestConditionChanged(); }
		void ToggleInvertedCondition() { m_inverted = !m_inverted; }
		CBaseEntity* GetTargetEntity() const { return m_targetEnt.GetEntity(); }
		int GetIntegerData() const { return m_iData; }
		float GetFloatData() const { return m_flData; }
		const Vector& GetVectorData() const { return m_vecData; }
		bool IsTestConditionInverted() const { return m_inverted; }
		TCTypes GetToggleConditionType() const { return m_toggle_type; }
		bool HasTargetEntityBeenSet() const { return !m_targetEnt.HasSavedEntity(); }
		/**
		 * @brief Runs the test condition.
		 * @return The test condition results.
		 */
		bool RunTestCondition() const;
		const EntityLink& GetEntityLink() const { return m_targetEnt; }
		const char* GetConditionName() const { return TCTypeToString(m_toggle_type); }
		void DebugScreenOverlay(const float x, const float y, float duration, int r = 255, int g = 255, int b = 0, int a = 255) const;

		void clear()
		{
			m_targetEnt.clear();
			m_toggle_type = TCTypes::TYPE_NOT_SET;
			m_iData = 0;
			m_flData = 0.0f;
			m_vecData = vec3_origin;
			m_inverted = false;
		}

	private:
		EntityLink m_targetEnt;
		TCTypes m_toggle_type;
		int m_iData; // int data
		float m_flData; // float data
		Vector m_vecData; // Vector data
		bool m_inverted;

		void OnTestConditionChanged() const;

		bool TestCondition_EntityExists() const { return m_targetEnt.GetEntity() != nullptr; }
		bool TestCondition_EntityAlive() const;
		bool TestCondition_EntityEnabled() const;
		bool TestCondition_EntityLocked() const;
		bool TestCondition_EntityTeam() const;
		bool TestCondition_EntityToggleState() const;
		bool TestCondition_EntityDistance() const;
		bool TestCondition_EntityVisible() const;
		bool TestCondition_TraceHullSolidWorld() const;
	};

}

#endif // !NAV_MESH_SCRIPTING_H_
