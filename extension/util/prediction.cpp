#include NAVBOT_PCH_FILE
#include <algorithm>

#include <extension.h>
#include <manager.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <entities/baseentity.h>
#include <bot/basebot.h>
#include <model_types.h>
#include "prediction.h"

#if SOURCE_ENGINE == SE_EPISODEONE
#include <sdkports/sdk_convarref_ep1.h>
#endif

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

#ifdef EXT_DEBUG
static ConVar cvar_draw_prediction("sm_navbot_debug_draw_prediction", "0", FCVAR_CHEAT | FCVAR_GAMEDLL, "If enabled, draws the player's predicted position.");
#endif // EXT_DEBUG


#undef min
#undef max
#undef clamp

Vector pred::SimpleProjectileLead(const Vector& targetPosition, const Vector& targetVelocity, const float projectileSpeed, const float rangeBetween)
{
	float time = GetProjectileTravelTime(projectileSpeed, rangeBetween);

	// time = time + (gpGlobals->interval_per_tick * 2.0f); // add two ticks ahead

	Vector result = targetPosition + (targetVelocity * time);
	return result;
}

float pred::SimpleGravityCompensation(const float time, const float proj_gravity)
{
	return (CExtManager::GetSvGravityValue() / 2.0f) * (time * time) * proj_gravity;
}

float pred::GetGrenadeZ(const float rangeBetween, const float projectileSpeed)
{
	constexpr auto PROJECTILE_SPEED_CONST = 0.707f;

	const float time = GetProjectileTravelTime(projectileSpeed * PROJECTILE_SPEED_CONST, rangeBetween);
	return std::min(0.0f, ((powf(2.0f, time) - 1.0f) * (CExtManager::GetSvGravityValue() * 0.1f)));
}

float pred::GravityComp(const float rangeBetween, const float projGravity, const float elevationRate)
{
	constexpr auto STEP_DIVIDER = 50.0f;
	float steps = rangeBetween / STEP_DIVIDER;
	float z = (CExtManager::GetSvGravityValue() * projGravity) * (elevationRate * steps);

	return z;
}

Vector pred::IterativeProjectileLead(const Vector& shooterPosition, const Vector& initialTargetPosition, const Vector& targetVelocity, const float projectileSpeed, const int maxIterations)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("pred::IterativeProjectileLead", "NavBot");
#endif // EXT_VPROF_ENABLED

	Vector targetPos = initialTargetPosition;
	Vector targetDir = targetPos - shooterPosition;
	float range = targetDir.NormalizeInPlace();
	float time = GetProjectileTravelTime(projectileSpeed, range);

	for (int i = 0; i < maxIterations; i++)
	{
		Vector predictedPosition = initialTargetPosition + (targetVelocity * time);

		targetDir = predictedPosition - shooterPosition;
		range = targetDir.NormalizeInPlace();
		time = GetProjectileTravelTime(projectileSpeed, range);

		targetPos = predictedPosition;
	}

	return targetPos;
}

Vector pred::IterativeEnginePredictedProjectileLead(CBaseEntity* target, const Vector& shooterPosition, const Vector& initialTargetPosition, const Vector& targetVelocity, const float projectileSpeed, const int maxIterations)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("pred::IterativeEnginePredictedProjectileLead", "NavBot");
#endif // EXT_VPROF_ENABLED

	static CEnginePrediction enginepred;
	enginepred.SetTraceMask(MASK_PLAYERSOLID);
	Vector targetPos = initialTargetPosition;
	Vector targetDir = targetPos - shooterPosition;
	float range = targetDir.NormalizeInPlace();
	float time = GetProjectileTravelTime(projectileSpeed, range);

	for (int i = 0; i < maxIterations; i++)
	{
		enginepred.PredictUntilGround(target, time);
		Vector predictedPosition = enginepred.GetPredictionData().pos;

		targetDir = predictedPosition - shooterPosition;
		range = targetDir.NormalizeInPlace();
		time = GetProjectileTravelTime(projectileSpeed, range);

		targetPos = predictedPosition;
	}

	return targetPos;
}

Vector pred::IterativeBallisticLeadAgaistPlayer(const Vector& shooterPosition, const Vector& initialTargetPosition, const Vector& targetVelocity, const WeaponAttackFunctionInfo* attackInfo, const int maxIterations)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("pred::IterativeBallisticLeadAgaistPlayer", "NavBot");
#endif // EXT_VPROF_ENABLED

	const float projectileSpeed = attackInfo->GetProjectileSpeed();
	Vector targetPos = initialTargetPosition;
	Vector targetDir = targetPos - shooterPosition;
	float range = targetDir.NormalizeInPlace();
	float time = GetProjectileTravelTime(projectileSpeed, range);

	for (int i = 0; i < maxIterations; i++)
	{
		Vector predictedPosition = initialTargetPosition + (targetVelocity * time);

		targetDir = predictedPosition - shooterPosition;
		range = targetDir.NormalizeInPlace();
		time = GetProjectileTravelTime(projectileSpeed, range);

		const float elevation_rate = RemapValClamped(range, attackInfo->GetBallisticElevationStartRange(), 
			attackInfo->GetBallisticElevationEndRange(), 
			attackInfo->GetBallisticElevationMinRate(), attackInfo->GetBallisticElevationMaxRate());

		const float z = GravityComp(range, attackInfo->GetGravity(), elevation_rate);

		predictedPosition.z += z;

		targetPos = predictedPosition;
	}

	return targetPos;
}

void pred::ProjectileData_t::FillFromAttackInfo(const WeaponAttackFunctionInfo* attackInfo)
{
	this->speed = attackInfo->GetProjectileSpeed();
	this->gravity = attackInfo->GetGravity();
	this->ballistic_start_range = attackInfo->GetBallisticElevationStartRange();
	this->ballistic_end_range = attackInfo->GetBallisticElevationEndRange();
	this->ballistic_min_rate = attackInfo->GetBallisticElevationMinRate();
	this->ballistic_max_rate = attackInfo->GetBallisticElevationMaxRate();
}

Vector pred::IterativeBallisticLead(const Vector& shooterPosition, const Vector& initialTargetPosition, const Vector& targetVelocity, const ProjectileData_t& data, const int maxIterations)
{
	const float projectileSpeed = data.speed;
	Vector targetPos = initialTargetPosition;
	Vector targetDir = targetPos - shooterPosition;
	float range = targetDir.NormalizeInPlace();
	float time = GetProjectileTravelTime(projectileSpeed, range);

	for (int i = 0; i < maxIterations; i++)
	{
		Vector predictedPosition = initialTargetPosition + (targetVelocity * time);

		targetDir = predictedPosition - shooterPosition;
		range = targetDir.NormalizeInPlace();
		time = GetProjectileTravelTime(projectileSpeed, range);

		const float elevation_rate = RemapValClamped(range, data.ballistic_start_range, data.ballistic_end_range,
			data.ballistic_min_rate, data.ballistic_max_rate);

		const float z = GravityComp(range, data.gravity, elevation_rate);

		predictedPosition.z += z;

		targetPos = predictedPosition;
	}

	return targetPos;
}

pred::PredictionEntityData::PredictionEntityData()
{
	std::memset(this, 0, sizeof(PredictionEntityData));
}

void pred::PredictionEntityData::UpdateData(CBaseEntity* entity)
{
	IServerUnknown* unk = reinterpret_cast<IServerUnknown*>(entity);
	this->entindex = unk->GetRefEHandle().GetEntryIndex();
	this->entity = entity;
	this->collider = unk->GetCollideable();
	entprops->GetEntPropVector(entity, Prop_Data, "m_vecBaseVelocity", this->basevel);
	entprops->GetEntPropVector(entity, Prop_Data, "m_vecAbsVelocity", this->absvel);
	entprops->GetEntPropVector(entity, Prop_Data, "m_vecAngVelocity", this->angvel);
	Vector angs{ 0.0f, 0.0f, 0.0f };
	entprops->GetEntPropVector(entity, Prop_Data, "m_angAbsRotation", angs);
	this->angles.x = angs.x;
	this->angles.y = angs.y;
	this->angles.z = angs.z;
	this->pos = this->collider->GetCollisionOrigin();
	entprops->GetEntPropVector(entity, Prop_Data, "m_vecMins", this->mins);
	entprops->GetEntPropVector(entity, Prop_Data, "m_vecMaxs", this->maxs);
	this->gravity = UtilHelpers::GetEntityGravity(this->entindex);
	this->moveType = UtilHelpers::GetEntityMoveType(this->entindex);
	entprops->GetEntPropFloat(entity, Prop_Data, "m_flElasticity", this->elasticity);
	entprops->GetEntPropFloat(entity, Prop_Data, "m_flFriction", this->friction);
	entprops->GetEntPropEnt(entity, Prop_Data, "m_hGroundEntity", nullptr, &this->groundent);
	entprops->GetEntPropEnt(entity, Prop_Data, "m_hMoveParent", nullptr, &this->moveparent);
	entprops->GetEntProp(entity, Prop_Data, "m_fFlags", this->fFlags);
	this->isplayer = this->HasFlags(FL_CLIENT);
}


pred::CEnginePrediction::CEnginePrediction() :
	m_entdata()
{
}

void pred::CEnginePrediction::PredictEntity(CBaseEntity* entity, float time)
{
	int steps = static_cast<int>(std::round(time / gpGlobals->interval_per_tick));
	steps = std::max(steps, 1);
	m_entdata.UpdateData(entity);
	
	for (int i = 0; i < steps; i++)
	{
		RunPrediction(entity);
	}
}

void pred::CEnginePrediction::PredictUntilGround(CBaseEntity* entity, float time)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CEnginePrediction::PredictUntilGround", "NavBot");
#endif // EXT_VPROF_ENABLED

	int steps = static_cast<int>(std::round(time / gpGlobals->interval_per_tick));
	steps = std::max(steps, 1);
	m_entdata.UpdateData(entity);

	for (int i = 0; i < steps; i++)
	{
		RunPrediction(entity);

		if (m_entdata.HasFlags(FL_ONGROUND))
		{
			return;
		}
	}
}

void pred::CEnginePrediction::RunPrediction(CBaseEntity* entity)
{
	sdkphysics::PhysicsToss(&m_entdata);
	// sdkphysics::PhysicsStep(&m_entdata);

#ifdef EXT_DEBUG
	if (cvar_draw_prediction.GetInt() != 0)
	{
		NDebugOverlay::Box(m_entdata.pos, m_entdata.mins, m_entdata.maxs, 255, 255, 0, 30, NDEBUG_PERSIST_FOR_ONE_TICK);
	}
#endif // EXT_DEBUG
}

void sdkphysics::PhysicsStep(pred::PredictionEntityData* entity)
{
	// Now run step simulator
	// StepSimulationThink(entity, dt);
	PhysicsStepRunTimestep(entity, gpGlobals->frametime);

	// TO-DO
	// PhysicsCheckWaterTransition();

	/*
	if (VPhysicsGetObject())
	{
		if (!VectorCompare(oldOrigin, GetAbsOrigin()))
		{
			VPhysicsGetObject()->UpdateShadow(GetAbsOrigin(), vec3_angle, (GetFlags() & FL_FLY) ? true : false, dt);
		}
	}
	PhysicsRelinkChildren(dt);
	*/
}

void sdkphysics::PhysicsToss(pred::PredictionEntityData* entity)
{
	PhysicsCheckVelocity(entity);

	Vector move{ 0.0f, 0.0f, 0.0f };
	PhysicsAddGravityMove(entity, move);

	SimulateAngles(entity, gpGlobals->frametime);

	trace_t trace;
	PhysicsPushEntity(entity, move, trace);

	PhysicsCheckVelocity(entity);

	if (trace.fraction != 1.0f)  // we hit something mid air
	{
		PerformFlyCollisionResolution(entity, trace, move);
	}
}

void sdkphysics::PhysicsCheckVelocity(pred::PredictionEntityData* entity)
{
	static ConVarRef sv_maxvelocity{ "sv_maxvelocity" };
	Vector origin = entity->pos;
	Vector vecAbsVelocity = entity->absvel;
	bool bReset = false;

	for (int i = 0; i < 3; i++)
	{
		if (vecAbsVelocity[i] > sv_maxvelocity.GetFloat())
		{
			// PrintToServer("Got a velocity too high");
			vecAbsVelocity[i] = sv_maxvelocity.GetFloat();
			bReset = true;
		}
		else if (vecAbsVelocity[i] < -sv_maxvelocity.GetFloat())
		{
			// PrintToServer("Got a velocity too low");
			vecAbsVelocity[i] = -sv_maxvelocity.GetFloat();
			bReset = true;
		}
	}

	if (bReset)
	{
		entity->pos = origin;
		entity->absvel = vecAbsVelocity;
	}
}

void sdkphysics::PhysicsAddGravityMove(pred::PredictionEntityData* entity, Vector& move)
{
	Vector vecAbsVelocity = entity->absvel;

	move.x = (vecAbsVelocity[0] + entity->basevel.x) * gpGlobals->frametime;
	move.y = (vecAbsVelocity[1] + entity->basevel.y) * gpGlobals->frametime;

	if ((entity->fFlags & FL_ONGROUND) != 0)
	{
		move.z = entity->basevel.z * gpGlobals->frametime;
		return;
	}

	// linear acceleration due to gravity
	float newZVelocity = vecAbsVelocity[2] - GetActualGravity(entity) * gpGlobals->frametime;

	move.z = ((vecAbsVelocity[2] + newZVelocity) / 2.0 + entity->basevel.z) * gpGlobals->frametime;

	Vector vecBaseVelocity = entity->basevel;
	vecBaseVelocity.z = 0.0f;
	entity->basevel = vecBaseVelocity;

	vecAbsVelocity.z = newZVelocity;
	entity->absvel = vecAbsVelocity;

	// Bound velocity
	PhysicsCheckVelocity(entity);
}

float sdkphysics::GetActualGravity(pred::PredictionEntityData* entity)
{
	float ent_gravity = entity->gravity;
	if (ent_gravity == 0.0f)
	{
		ent_gravity = 1.0f;
	}

	return ent_gravity * CExtManager::GetSvGravityValue();
}

void sdkphysics::SimulateAngles(pred::PredictionEntityData* entity, float frametime)
{
	Vector angles;
	Vector tmp{ entity->angles.x, entity->angles.y, entity->angles.z };
	VectorMA(tmp, frametime, entity->angvel, angles);
	entity->angles.x = angles.x;
	entity->angles.y = angles.y;
	entity->angles.z = angles.z;
}

void sdkphysics::PhysicsPushEntity(pred::PredictionEntityData* entity, Vector& move, trace_t& tr)
{
	PhysicsCheckSweep(entity, entity->pos, move, tr);

	if (tr.fraction)
	{
		entity->pos = tr.endpos;
	}
}

void sdkphysics::PhysicsCheckSweep(pred::PredictionEntityData* entity, Vector& vecAbsStart, Vector vecAbsDelta, trace_t& tr)
{
	unsigned int mask = entity->mask;
	mask &= ~CONTENTS_MONSTER;

	trace::CTraceFilterSimple filter{ entity->entity, entity->collider->GetCollisionGroup() };
	Vector vecAbsEnd;
	VectorAdd(vecAbsStart, vecAbsDelta, vecAbsEnd);

	trace::hull(vecAbsStart, vecAbsEnd, entity->mins, entity->maxs, mask, &filter, tr);
}

void sdkphysics::PerformFlyCollisionResolution(pred::PredictionEntityData* entity, trace_t& trace, Vector& move)
{
	if (entity->moveType == MOVETYPE_FLYGRAVITY)
	{
		return; // TO-DO: players don't take this path
	}
	else
	{
		ResolveFlyCollisionSlide(entity, trace, move);
	}
}

void sdkphysics::ResolveFlyCollisionSlide(pred::PredictionEntityData* entity, trace_t& trace, Vector& vecVelocity)
{
	// Get the impact surface's friction.
	float flSurfaceFriction;
	physprops->GetPhysicsProperties(trace.surface.surfaceProps, NULL, NULL, &flSurfaceFriction, NULL);

	// A backoff of 1.0 is a slide.
	float flBackOff = 1.0f;
	Vector vecAbsVelocity;
	PhysicsClipVelocity(entity, entity->absvel, trace.plane.normal, vecAbsVelocity, flBackOff);

	if (trace.plane.normal.z <= 0.7)			// Floor
	{
		entity->absvel = vecAbsVelocity;
		return;
	}

	// Stop if on ground.
	// Get the total velocity (player + conveyors, etc.)
	VectorAdd(vecAbsVelocity, entity->basevel, vecVelocity);
	float flSpeedSqr = DotProduct(vecVelocity, vecVelocity);

	// Verify that we have an entity.
	CBaseEntity* pEntity = trace.m_pEnt;

	// Are we on the ground?
	if (vecVelocity.z < (GetActualGravity(entity) * gpGlobals->frametime))
	{
		vecAbsVelocity.z = 0.0f;

		// Recompute speedsqr based on the new absvel
		VectorAdd(vecAbsVelocity, entity->basevel, vecVelocity);
		flSpeedSqr = DotProduct(vecVelocity, vecVelocity);
	}
	entity->absvel = vecAbsVelocity;

	if (flSpeedSqr < (30 * 30))
	{
		if (IsStandable(pEntity))
		{
			entity->SetGroundEntity(pEntity);
		}

		// Reset velocities.
		entity->absvel = vec3_origin;
		entity->angvel = vec3_origin;
	}
	else
	{
		vecAbsVelocity += entity->basevel;
		vecAbsVelocity *= (1.0f - trace.fraction) * gpGlobals->frametime * flSurfaceFriction;
		PhysicsPushEntity(entity, vecAbsVelocity, trace);
	}
}

int sdkphysics::PhysicsClipVelocity(pred::PredictionEntityData* entity, const Vector& in, const Vector& normal, Vector& out, float overbounce)
{
	constexpr float STOP_EPSILON = 0.1f;
	float	backoff;
	float	change;
	float angle;
	int		i, blocked;

	blocked = 0;

	angle = normal[2];

	if (angle > 0)
	{
		blocked |= 1;		// floor
	}
	if (!angle)
	{
		blocked |= 2;		// step
	}

	backoff = DotProduct(in, normal) * overbounce;

	for (i = 0; i < 3; i++)
	{
		change = normal[i] * backoff;
		out[i] = in[i] - change;
		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
		{
			out[i] = 0;
		}
	}

	return blocked;
}

bool sdkphysics::PhysicsCheckWater(pred::PredictionEntityData* entity)
{
	int waterlevel = static_cast<int>(entityprops::GetEntityWaterLevel(entity->entity));

	if (entity->moveparent != nullptr)
		return waterlevel > 1;

	int cont = 0;
	entprops->GetEntProp(entity->entity, Prop_Data, "m_nWaterType", cont);

	// If we're not in water + don't have a current, we're done
	if ((cont & (MASK_WATER | MASK_CURRENT)) != (MASK_WATER | MASK_CURRENT))
		return waterlevel > 1;

	// Compute current direction
	Vector v(0, 0, 0);
	if (cont & CONTENTS_CURRENT_0)
	{
		v[0] += 1;
	}
	if (cont & CONTENTS_CURRENT_90)
	{
		v[1] += 1;
	}
	if (cont & CONTENTS_CURRENT_180)
	{
		v[0] -= 1;
	}
	if (cont & CONTENTS_CURRENT_270)
	{
		v[1] -= 1;
	}
	if (cont & CONTENTS_CURRENT_UP)
	{
		v[2] += 1;
	}
	if (cont & CONTENTS_CURRENT_DOWN)
	{
		v[2] -= 1;
	}

	// The deeper we are, the stronger the current.
	Vector newBaseVelocity;
	VectorMA(entity->basevel, 50.0 * waterlevel, v, newBaseVelocity);
	entity->basevel = newBaseVelocity;

	return waterlevel > 1;
}

void sdkphysics::PhysicsStepRunTimestep(pred::PredictionEntityData* entity, float timestep)
{
	static ConVarRef sv_friction{ "sv_friction" };
	static ConVarRef sv_stopspeed{ "sv_stopspeed" };
	bool	wasonground;
	bool	inwater;
	float	speed, newspeed, control;
	float	friction;

	PhysicsCheckVelocity(entity);

	wasonground = (entity->fFlags & FL_ONGROUND) ? true : false;

	// add gravity except:
	//   flying monsters
	//   swimming monsters who are in the water
	inwater = PhysicsCheckWater(entity);

	bool isfalling = false;

	if (!wasonground)
	{
		if (!(entity->fFlags & FL_FLY))
		{
			if (!((entity->fFlags & FL_SWIM) && (entityprops::GetEntityWaterLevel(entity->entity) > 0)))
			{

				if (!inwater)
				{
					PhysicsAddHalfGravity(entity, timestep);
					isfalling = true;
				}
			}
		}
	}

	if (!(entity->fFlags & FL_STEPMOVEMENT) &&
		(!VectorCompare(entity->absvel, vec3_origin) ||
			!VectorCompare(entity->basevel, vec3_origin)))
	{
		Vector vecAbsVelocity = entity->absvel;

		entity->SetGroundEntity(nullptr);
		// apply friction
		// let dead monsters who aren't completely onground slide
		if (wasonground)
		{
			speed = VectorLength(vecAbsVelocity);
			if (speed)
			{
				friction = sv_friction.GetFloat() * entity->friction;

				control = speed < sv_stopspeed.GetFloat() ? sv_stopspeed.GetFloat() : speed;
				newspeed = speed - timestep * control * friction;

				if (newspeed < 0)
					newspeed = 0;
				newspeed /= speed;

				vecAbsVelocity[0] *= newspeed;
				vecAbsVelocity[1] *= newspeed;
			}
		}

		vecAbsVelocity += entity->basevel;
		entity->absvel = vecAbsVelocity;

		// Apply angular velocity
		SimulateAngles(entity, timestep);

		PhysicsCheckVelocity(entity);

		PhysicsTryMove(entity, timestep, nullptr);

		PhysicsCheckVelocity(entity);

		vecAbsVelocity = entity->absvel;
		vecAbsVelocity -= entity->basevel;
		entity->absvel = vecAbsVelocity;

		PhysicsCheckVelocity(entity);

		if (!(entity->fFlags & FL_ONGROUND))
		{
			PhysicsStepRecheckGround(entity);
		}

		// PhysicsTouchTriggers();
	}

	if (!(entity->fFlags & FL_ONGROUND) && isfalling)
	{
		PhysicsAddHalfGravity(entity, timestep);
	}
}

void sdkphysics::PhysicsAddHalfGravity(pred::PredictionEntityData* entity, float timestep)
{
	float	ent_gravity;

	if (entity->gravity)
	{
		ent_gravity = entity->gravity;
	}
	else
	{
		ent_gravity = 1.0;
	}

	// Add 1/2 of the total gravitational effects over this timestep
	Vector vecAbsVelocity = entity->absvel;
	vecAbsVelocity[2] -= (0.5 * ent_gravity * CExtManager::GetSvGravityValue() * timestep);
	vecAbsVelocity[2] += entity->basevel.z * gpGlobals->frametime;
	entity->absvel = vecAbsVelocity;

	Vector vecNewBaseVelocity = entity->basevel;
	vecNewBaseVelocity[2] = 0;
	entity->basevel = vecNewBaseVelocity;

	// Bound velocity
	PhysicsCheckVelocity(entity);
}

int sdkphysics::PhysicsTryMove(pred::PredictionEntityData* entity, float flTime, trace_t* steptrace)
{
	constexpr int MAX_CLIP_PLANES = 5;
	static ConVarRef sv_bounce{ "sv_bounce" };
	int			bumpcount, numbumps;
	Vector		dir;
	float		d;
	int			numplanes;
	Vector		planes[MAX_CLIP_PLANES];
	Vector		primal_velocity, original_velocity, new_velocity;
	int			i, j;
	trace_t		trace;
	Vector		end;
	float		time_left;
	int			blocked;
	trace::CTraceFilterSimple filter{ entity->entity, entity->collider->GetCollisionGroup() };

	unsigned int mask = entity->mask;

	new_velocity.Init();

	numbumps = 4;

	Vector vecAbsVelocity = entity->absvel;

	blocked = 0;
	VectorCopy(vecAbsVelocity, original_velocity);
	VectorCopy(vecAbsVelocity, primal_velocity);
	numplanes = 0;

	time_left = flTime;

	for (bumpcount = 0; bumpcount < numbumps; bumpcount++)
	{
		if (vecAbsVelocity == vec3_origin)
			break;

		VectorMA(entity->pos, time_left, vecAbsVelocity, end);

		trace::hull(entity->pos, end, entity->mins, entity->maxs, mask, &filter, trace);

		if (trace.startsolid)
		{	// entity is trapped in another solid
			entity->absvel = vec3_origin;
			return 4;
		}

		if (trace.fraction > 0)
		{	// actually covered some distance
			entity->pos = trace.endpos;
			VectorCopy(vecAbsVelocity, original_velocity);
			numplanes = 0;
		}

		if (trace.fraction == 1)
			break;		// moved the entire distance

		if (!trace.m_pEnt)
		{
			entity->absvel = vecAbsVelocity;
			// Warning("PhysicsTryMove: !trace.u.ent");
			// Assert(0);
			return 4;
		}

		if (trace.plane.normal[2] > 0.7)
		{
			blocked |= 1;		// floor
			if (IsStandable(trace.m_pEnt))
			{
				// keep track of time when changing ground entity
				//if (GetGroundEntity() != trace.m_pEnt)
				//{
				//	SetGroundChangeTime(gpGlobals->curtime + (flTime - (1 - trace.fraction) * time_left));
				//}

				entity->groundent = trace.m_pEnt;
			}
		}
		if (!trace.plane.normal[2])
		{
			blocked |= 2;		// step
			
			if (steptrace != nullptr)
			{
				*steptrace = trace;	// save for player extrafriction
			}
		}

		// run the impact function
		// PhysicsImpact(trace.m_pEnt, trace);
		// Removed by the impact function
		// if (IsMarkedForDeletion() || IsEdictFree())
			// break;

		time_left -= time_left * trace.fraction;

		// clipped to another plane
		if (numplanes >= MAX_CLIP_PLANES)
		{	// this shouldn't really happen
			entity->absvel = vec3_origin;
			return blocked;
		}

		VectorCopy(trace.plane.normal, planes[numplanes]);
		numplanes++;

		// modify original_velocity so it parallels all of the clip planes
		if (entity->moveType == MOVETYPE_WALK && (!(entity->fFlags & FL_ONGROUND) || entity->friction != 1.0f))	// relfect player velocity
		{
			for (i = 0; i < numplanes; i++)
			{
				if (planes[i][2] > 0.7)
				{// floor or slope
					PhysicsClipVelocity(entity, original_velocity, planes[i], new_velocity, 1.0f);
					VectorCopy(new_velocity, original_velocity);
				}
				else
				{
					PhysicsClipVelocity(entity, original_velocity, planes[i], new_velocity, 1.0 + sv_bounce.GetFloat() * (1.0f - entity->friction));
				}
			}

			VectorCopy(new_velocity, vecAbsVelocity);
			VectorCopy(new_velocity, original_velocity);
		}
		else
		{
			for (i = 0; i < numplanes; i++)
			{
				PhysicsClipVelocity(entity, original_velocity, planes[i], new_velocity, 1.0f);
				for (j = 0; j < numplanes; j++)
					if (j != i)
					{
						if (DotProduct(new_velocity, planes[j]) < 0)
							break;	// not ok
					}
				if (j == numplanes)
					break;
			}

			if (i != numplanes)
			{
				// go along this plane
				VectorCopy(new_velocity, vecAbsVelocity);
			}
			else
			{
				// go along the crease
				if (numplanes != 2)
				{
					//				Msg( "clip velocity, numplanes == %i\n",numplanes);
					entity->absvel = vecAbsVelocity;
					return blocked;
				}
				CrossProduct(planes[0], planes[1], dir);
				d = DotProduct(dir, vecAbsVelocity);
				VectorScale(dir, d, vecAbsVelocity);
			}

			//
			// if original velocity is against the original velocity, stop dead
			// to avoid tiny oscillations in sloping corners
			//
			if (DotProduct(vecAbsVelocity, primal_velocity) <= 0)
			{
				entity->absvel = vec3_origin;
				return blocked;
			}
		}
	}

	entity->absvel = vecAbsVelocity;
	return blocked;
}

void sdkphysics::PhysicsStepRecheckGround(pred::PredictionEntityData* entity)
{
	unsigned int mask = entity->mask;
	// determine if it's on solid ground at all
	Vector	mins, maxs, point;
	int		x, y;
	trace_t trace;

	VectorAdd(entity->pos, entity->collider->OBBMins(), mins);
	VectorAdd(entity->pos, entity->collider->OBBMaxs(), maxs);
	point[2] = mins[2] - 1;
	for (x = 0; x <= 1; x++)
	{
		for (y = 0; y <= 1; y++)
		{
			point[0] = x ? maxs[0] : mins[0];
			point[1] = y ? maxs[1] : mins[1];

			trace::line(point, point, mask, entity->entity, COLLISION_GROUP_NONE, trace);

			/*
			if (pCollision && IsNPC())
			{
				UTIL_TraceLineFilterEntity(this, point, point, mask, COLLISION_GROUP_NONE, &trace);
			}
			else
			{
				UTIL_TraceLine(point, point, mask, this, COLLISION_GROUP_NONE, &trace);
			}
			*/

			if (trace.startsolid)
			{
				entity->groundent = trace.m_pEnt;
				return;
			}
		}
	}
}

bool sdkphysics::IsStandable(CBaseEntity* entity)
{
	ICollideable* collider = reinterpret_cast<IServerUnknown*>(entity)->GetCollideable();

	if ((collider->GetSolidFlags() & FSOLID_NOT_STANDABLE) != 0)
		return false;

	switch (collider->GetSolid())
	{
	case SOLID_BSP:
		[[fallthrough]];
	case SOLID_VPHYSICS:
		[[fallthrough]];
	case SOLID_BBOX:
		return true;
	default:
		break;
	}

	return IsBSPModel(entity);
}

bool sdkphysics::IsBSPModel(CBaseEntity* entity)
{
	ICollideable* collider = reinterpret_cast<IServerUnknown*>(entity)->GetCollideable();

	if (collider->GetSolid() == SOLID_BSP)
		return true;

	const model_t* pModel = modelinfo->GetModel(reinterpret_cast<IServerEntity*>(entity)->GetModelIndex());

	if (collider->GetSolid() == SOLID_VPHYSICS && modelinfo->GetModelType(pModel) == static_cast<int>(modtype_t::mod_brush))
		return true;

	return false;
}
