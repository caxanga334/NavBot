#ifndef SMNAV_ENTITIES_BASE_ENTITY_H_
#define SMNAV_ENTITIES_BASE_ENTITY_H_
#pragma once

#include <string>
#include <const.h>

// Helper classes to access entity data

struct edict_t;
struct matrix3x4_t;
class CBaseEntity;

namespace entities
{
	// 'H' prefix for Helper Base Entity, to avoid conflicts

	class HBaseEntity
	{
	public:
		HBaseEntity(edict_t* entity);
		HBaseEntity(CBaseEntity* entity);

		void AssignNewEntity(CBaseEntity* entity);

		// returns an edict index or reference for non networked entities
		inline int GetIndex() const { return m_index; }
		bool GetEntity(CBaseEntity** entity, edict_t** edict) const;
		CBaseEntity* GetBaseEntity() const { return m_pEntity; }
		edict_t* GetEdict() const { return m_edict; }
		const char* GetClassname() const;
		bool IsBoundsDefinedInEntitySpace() const;
		Vector GetAbsOrigin() const;
		void SetAbsOrigin(const Vector& origin) const;
		Vector EyePosition() const;
		Vector WorldAlignMins() const;
		Vector WorldAlignMaxs() const;
		Vector OBBCenter() const;
		Vector WorldSpaceCenter() const;
		QAngle GetAbsAngles() const;
		void SetAbsAngles(const QAngle& angles) const;
		void SetAbsAngles(const Vector& angles) const;
		QAngle GetCollisionAngles() const;
		Vector GetAbsVelocity() const;
		void SetAbsVelocity(const Vector& velocity) const;
		Vector GetViewOffset() const;
		void CalcNearestPoint(const Vector& worldPos, Vector& out) const;
		size_t GetTargetName(char* result, int maxsize) const;
		CBaseEntity* GetOwnerEntity() const;
		CBaseEntity* GetMoveParent() const;
		CBaseEntity* GetMoveChild() const;
		CBaseEntity* GetMovePeer() const;
		MoveType_t GetMoveType() const;
		int GetTeam() const;
		int GetEFlags() const;
		int IsEFlagSet(const int nEFlagMask) const;
		int GetSolidFlags() const;
		SolidType_t GetSolidType() const;
		matrix3x4_t CollisionToWorldTransform() const;
		void CollisionToWorldSpace(const Vector& in, Vector& out) const;
		void WorldToCollisionSpace(const Vector& in, Vector& out) const;
		int GetParentAttachment() const;
		matrix3x4_t GetParentToWorldTransform(matrix3x4_t& tempMatrix) const;
		int GetEffects() const;
		inline bool IsEffectActive(int effects) const { return (GetEffects() & effects) != 0; }
		float GetSimulationTime() const;
		void SetSimulationTime(float time) const;
		void Teleport(const Vector& origin, const QAngle* angles = nullptr, const Vector* velocity = nullptr) const;
		bool IsDisabled() const;
		RenderMode_t GetRenderMode() const;
		int GetHealth() const;
		int GetMaxHealth() const;
		int GetTakeDamage() const;
		matrix3x4_t EntityToWorldTransform() const;
		void ComputeAbsPosition(const Vector& vecLocalPosition, Vector* pAbsPosition) const;
		bool IsTransparent() const
		{
			return GetRenderMode() != RenderMode_t::kRenderNormal;
		}

	private:
		void CalcAbsolutePosition(matrix3x4_t& result) const;
		int m_index; // entity index or reference
		CBaseEntity* m_pEntity;
		edict_t* m_edict;
	};

	class HFuncBrush : public HBaseEntity
	{
	public:
		HFuncBrush(edict_t* entity) : HBaseEntity(entity) {}
		HFuncBrush(CBaseEntity* entity) : HBaseEntity(entity) {}

		enum BrushSolidities_e {
			BRUSHSOLID_TOGGLE = 0,
			BRUSHSOLID_NEVER = 1,
			BRUSHSOLID_ALWAYS = 2,
		};

		BrushSolidities_e GetSolidity() const;
	};

	/**
	 * @brief EHANDLE made to store helper entities.
	 * @tparam EntClass Helper entity class to store.
	 */
	template <typename EntClass>
	class EntityHandle
	{
	public:
		EntityHandle(CBaseEntity* entity) : EntClass(entity) {}

		// returns the stored entity. NULL if the entity was removed.
		EntClass* Get() const
		{
			CBaseEntity* ent = m_handle.Get();

			if (ent)
			{
				return &m_entity;
			}

			return nullptr;
		}

		// Assigns a new entity.
		void Set(EntClass* entity)
		{
			m_handle.Set(entity->GetBaseEntity());
			m_entity.AssignNewEntity(entity->GetBaseEntity());
		}

		// Assigns a new entity.
		void SetEntity(CBaseEntity* entity)
		{
			m_handle.Set(entity);
			m_entity.AssignNewEntity(entity);
		}

	private:
		CHandle<CBaseEntity> m_handle;
		EntClass m_entity;
	};
}

inline bool entities::HBaseEntity::IsBoundsDefinedInEntitySpace() const
{
	return ((GetSolidFlags() & FSOLID_FORCE_WORLD_ALIGNED) == 0) && (GetSolidType() != SOLID_BBOX) && (GetSolidType() != SOLID_NONE);
}

#endif // !SMNAV_ENTITIES_BASE_ENTITY_H_

