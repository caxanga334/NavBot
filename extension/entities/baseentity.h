#ifndef SMNAV_ENTITIES_BASE_ENTITY_H_
#define SMNAV_ENTITIES_BASE_ENTITY_H_
#pragma once

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

		// returns an edict index or reference for non networked entities
		inline int GetIndex() const { return m_index; }
		bool GetEntity(CBaseEntity** entity, edict_t** edict) const;
		const char* GetClassname() const;
		bool IsBoundsDefinedInEntitySpace() const;
		Vector GetAbsOrigin() const;
		void SetAbsOrigin(const Vector& origin) const;
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
		void GetTargetName(char* result, int maxsize) const;
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

	private:
		void CalcAbsolutePosition(matrix3x4_t& result) const;
		int m_index; // entity index or reference
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
}

inline bool entities::HBaseEntity::IsBoundsDefinedInEntitySpace() const
{
	return ((GetSolidFlags() & FSOLID_FORCE_WORLD_ALIGNED) == 0) && (GetSolidType() != SOLID_BBOX) && (GetSolidType() != SOLID_NONE);
}

#endif // !SMNAV_ENTITIES_BASE_ENTITY_H_

