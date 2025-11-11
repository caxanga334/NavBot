#ifndef NAVBOT_PREDICTION_LIB_H_
#define NAVBOT_PREDICTION_LIB_H_
#pragma once

class CBotWeapon;
class CBaseExtPlayer;
class CBaseBot;
class WeaponAttackFunctionInfo;

namespace pred
{
	Vector SimpleProjectileLead(const Vector& targetPosition, const Vector& targetVelocity, const float projectileSpeed, const float rangeBetween);
	float SimpleGravityCompensation(const float time, const float proj_gravity);

	inline const float GetProjectileTravelTime(const float projectileSpeed, const float rangeBetween)
	{
		return rangeBetween / projectileSpeed;
	}

	float GetGrenadeZ(const float rangeBetween, const float projectileSpeed);

	float GravityComp(const float rangeBetween, const float projGravity, const float elevationRate);

	Vector IterativeProjectileLead(const Vector& shooterPosition, const Vector& initialTargetPosition, const Vector& targetVelocity, const float projectileSpeed, const int maxIterations);

	Vector IterativeEnginePredictedProjectileLead(CBaseEntity* target, const Vector& shooterPosition, const Vector& initialTargetPosition, const Vector& targetVelocity, const float projectileSpeed, const int maxIterations);

	Vector IterativeBallisticLeadAgaistPlayer(const Vector& shooterPosition, const Vector& initialTargetPosition, const Vector& targetVelocity, const WeaponAttackFunctionInfo* attackInfo, const int maxIterations);

	struct ProjectileData_t
	{
		void FillFromAttackInfo(const WeaponAttackFunctionInfo* attackInfo);

		float speed;
		float gravity;
		float ballistic_start_range;
		float ballistic_end_range;
		float ballistic_min_rate;
		float ballistic_max_rate;
	};

	Vector IterativeBallisticLead(const Vector& shooterPosition, const Vector& initialTargetPosition, const Vector& targetVelocity, const ProjectileData_t& data, const int maxIterations);

	struct PredictionEntityData
	{
		PredictionEntityData();

		int entindex;
		CBaseEntity* entity;
		ICollideable* collider;
		Vector basevel;
		Vector absvel;
		Vector angvel;
		QAngle angles;
		Vector pos;
		float gravity;
		Vector mins;
		Vector maxs;
		MoveType_t moveType;
		float elasticity;
		float friction;
		CBaseEntity* groundent;
		CBaseEntity* moveparent;
		int fFlags;
		unsigned int mask;
		bool isplayer;

		void UpdateData(CBaseEntity* entity);
		void SetGroundEntity(CBaseEntity* ground)
		{
			this->groundent = ground;

			if (ground)
			{
				AddFlags(FL_ONGROUND);
			}
			else
			{
				RemoveFlags(FL_ONGROUND);
			}
		}
		void AddFlags(int flags) { this->fFlags |= flags; }
		void RemoveFlags(int flags) { this->fFlags &= ~flags; }
		bool HasFlags(int flags) const { return (this->fFlags & flags) != 0; }
	};

	class CEnginePrediction
	{
	public:
		CEnginePrediction();

		void PredictEntity(CBaseEntity* entity, float time);
		/**
		 * @brief Predicts an entity movement until they hit ground.
		 * @param entity Entity to simulate.
		 * @param time How many seconds in the future to simulate.
		 */
		void PredictUntilGround(CBaseEntity* entity, float time);

		void SetTraceMask(unsigned int mask) { m_entdata.mask = mask; }
		unsigned int GetTraceMask() const { return m_entdata.mask; }
		const PredictionEntityData& GetPredictionData() const { return m_entdata; }

	private:
		PredictionEntityData m_entdata;
		void RunPrediction(CBaseEntity* entity);
	};
}

namespace sdkphysics
{
	void PhysicsStep(pred::PredictionEntityData* entity);
	void PhysicsToss(pred::PredictionEntityData* entity);
	void PhysicsCheckVelocity(pred::PredictionEntityData* entity);
	void PhysicsAddGravityMove(pred::PredictionEntityData* entity, Vector& move);
	float GetActualGravity(pred::PredictionEntityData* entity);
	void SimulateAngles(pred::PredictionEntityData* entity, float frametime);
	void PhysicsPushEntity(pred::PredictionEntityData* entity, Vector& move, trace_t& tr);
	void PhysicsCheckSweep(pred::PredictionEntityData* entity, Vector& vecAbsStart, Vector vecAbsDelta, trace_t& tr);
	void PerformFlyCollisionResolution(pred::PredictionEntityData* entity, trace_t& trace, Vector& move);
	void ResolveFlyCollisionSlide(pred::PredictionEntityData* entity, trace_t& trace, Vector& vecVelocity);
	int PhysicsClipVelocity(pred::PredictionEntityData* entity, const Vector& in, const Vector& normal, Vector& out, float overbounce);
	bool PhysicsCheckWater(pred::PredictionEntityData* entity);
	void PhysicsStepRunTimestep(pred::PredictionEntityData* entity, float timestep);
	void PhysicsAddHalfGravity(pred::PredictionEntityData* entity, float timestep);
	int PhysicsTryMove(pred::PredictionEntityData* entity, float flTime, trace_t* steptrace);
	void PhysicsStepRecheckGround(pred::PredictionEntityData* entity);
	bool IsStandable(CBaseEntity* entity);
	bool IsBSPModel(CBaseEntity* entity);
}

#endif // !NAVBOT_PREDICTION_LIB_H_
