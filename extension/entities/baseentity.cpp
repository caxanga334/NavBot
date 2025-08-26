#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <entities/baseentity.h>

entities::HBaseEntity::HBaseEntity(edict_t* entity)
{
	m_index = gamehelpers->IndexOfEdict(entity);
}

entities::HBaseEntity::HBaseEntity(CBaseEntity* entity)
{
	m_index = gamehelpers->EntityToBCompatRef(entity);
}

/**
 * @brief Gets the actual entity as a CBaseEntity or edict
 * @return false if lookup failed
*/
bool entities::HBaseEntity::GetEntity(CBaseEntity** entity, edict_t** edict) const
{
	return UtilHelpers::IndexToAThings(GetIndex(), entity, edict);
}

const char* entities::HBaseEntity::GetClassname() const
{
	CBaseEntity* entity = nullptr;
	if (!GetEntity(&entity, nullptr)) { return nullptr; }
	return gamehelpers->GetEntityClassname(entity);
}

Vector entities::HBaseEntity::GetAbsOrigin() const
{
	Vector result;

	if (entprops->GetEntPropVector(GetIndex(), Prop_Data, "m_vecOrigin", result) == true)
	{
		return result;
	}

	return vec3_origin;
}

void entities::HBaseEntity::SetAbsOrigin(const Vector& origin) const
{
	entprops->SetEntPropVector(GetIndex(), Prop_Data, "m_vecOrigin", origin);
}

Vector entities::HBaseEntity::EyePosition() const
{
	return GetAbsOrigin() + GetViewOffset();
}

Vector entities::HBaseEntity::WorldAlignMins() const
{
	Vector result;

	if (entprops->GetEntPropVector(GetIndex(), Prop_Data, "m_vecMins", result) == true)
	{
		return result;
	}

	return vec3_origin;
}

Vector entities::HBaseEntity::WorldAlignMaxs() const
{
	Vector result;

	if (entprops->GetEntPropVector(GetIndex(), Prop_Data, "m_vecMaxs", result) == true)
	{
		return result;
	}

	return vec3_origin;
}

Vector entities::HBaseEntity::OBBCenter() const
{
	Vector result;
	VectorLerp(WorldAlignMins(), WorldAlignMaxs(), 0.5f, result);
	return result;
}

Vector entities::HBaseEntity::WorldSpaceCenter() const
{
	Vector result;
	Vector center = OBBCenter();
	CollisionToWorldSpace(center, result);
	return result;
}

QAngle entities::HBaseEntity::GetAbsAngles() const
{
	Vector result;

	if (entprops->GetEntPropVector(GetIndex(), Prop_Data, "m_angRotation", result) == true)
	{
		return QAngle(result.x, result.y, result.z);
	}

	return vec3_angle;
}

void entities::HBaseEntity::SetAbsAngles(const QAngle& angles) const
{
	Vector vangles(angles.x, angles.y, angles.z);
	entprops->SetEntPropVector(GetIndex(), Prop_Data, "m_angRotation", vangles);
}

void entities::HBaseEntity::SetAbsAngles(const Vector& angles) const
{
	entprops->SetEntPropVector(GetIndex(), Prop_Data, "m_angRotation", angles);
}

QAngle entities::HBaseEntity::GetCollisionAngles() const
{
	if (IsBoundsDefinedInEntitySpace())
	{
		return GetAbsAngles();
	}

	return vec3_angle;
}

Vector entities::HBaseEntity::GetAbsVelocity() const
{
	Vector result;

	if (entprops->GetEntPropVector(GetIndex(), Prop_Data, "m_vecAbsVelocity", result) == true)
	{
		return result;
	}

	return vec3_origin;
}

void entities::HBaseEntity::SetAbsVelocity(const Vector& velocity) const
{
	entprops->SetEntPropVector(GetIndex(), Prop_Data, "m_vecAbsVelocity", velocity);
}

Vector entities::HBaseEntity::GetViewOffset() const
{
	Vector result;

	if (entprops->GetEntPropVector(GetIndex(), Prop_Data, "m_vecViewOffset", result) == true)
	{
		return result;
	}

	return vec3_origin;
}

void entities::HBaseEntity::CalcNearestPoint(const Vector& worldPos, Vector& out) const
{
	Vector localPt, localClosestPt;
	WorldToCollisionSpace(worldPos, localPt);
	UtilHelpers::CalcClosestPointOnAABB(WorldAlignMins(), WorldAlignMaxs(), localPt, localClosestPt);
	CollisionToWorldSpace(localClosestPt, out);
}

size_t entities::HBaseEntity::GetTargetName(char* result, int maxsize) const
{
	size_t size = 0;
	if (!entprops->GetEntPropString(GetIndex(), Prop_Data, "m_iName", result, maxsize, size))
	{
		ke::SafeStrcpy(result, static_cast<size_t>(maxsize), "");
	}

	return size;
}

CBaseEntity* entities::HBaseEntity::GetOwnerEntity() const
{
	int ref = INVALID_EHANDLE_INDEX;
	entprops->GetEntPropEnt(GetIndex(), Prop_Data, "m_hOwnerEntity", &ref);
	return gamehelpers->ReferenceToEntity(ref);
}

CBaseEntity* entities::HBaseEntity::GetMoveParent() const
{
	int ref = INVALID_EHANDLE_INDEX;
	entprops->GetEntPropEnt(GetIndex(), Prop_Data, "m_hMoveParent", &ref);
	return gamehelpers->ReferenceToEntity(ref);
}

CBaseEntity* entities::HBaseEntity::GetMoveChild() const
{
	int ref = INVALID_EHANDLE_INDEX;
	entprops->GetEntPropEnt(GetIndex(), Prop_Data, "m_hMoveChild", &ref);
	return gamehelpers->ReferenceToEntity(ref);
}

CBaseEntity* entities::HBaseEntity::GetMovePeer() const
{
	int ref = INVALID_EHANDLE_INDEX;
	entprops->GetEntPropEnt(GetIndex(), Prop_Data, "m_hMovePeer", &ref);
	return gamehelpers->ReferenceToEntity(ref);
}

MoveType_t entities::HBaseEntity::GetMoveType() const
{
	return UtilHelpers::GetEntityMoveType(GetIndex());
}

int entities::HBaseEntity::GetTeam() const
{
	int team = 0;
	entprops->GetEntProp(GetIndex(), Prop_Data, "m_iTeamNum", team);
	return team;
}

int entities::HBaseEntity::GetEFlags() const
{
	int result = 0;
	entprops->GetEntProp(GetIndex(), Prop_Data, "m_iEFlags", result);
	return result;
}

int entities::HBaseEntity::IsEFlagSet(const int nEFlagMask) const
{
	return (GetEFlags() & nEFlagMask) != 0;
}

int entities::HBaseEntity::GetSolidFlags() const
{
	int flags = 0;
	entprops->GetEntProp(GetIndex(), Prop_Data, "m_usSolidFlags", flags);
	return flags;
}

SolidType_t entities::HBaseEntity::GetSolidType() const
{
	int type = 0;
	entprops->GetEntProp(GetIndex(), Prop_Data, "m_nSolidType", type);
	return static_cast<SolidType_t>(type);
}

matrix3x4_t entities::HBaseEntity::CollisionToWorldTransform() const
{
	matrix3x4_t matResult;

	if (IsBoundsDefinedInEntitySpace())
	{
		CalcAbsolutePosition(matResult);
		return matResult;
	}

	SetIdentityMatrix(matResult);
	MatrixSetColumn(GetAbsOrigin(), 3, matResult);
	return matResult;
}

void entities::HBaseEntity::CollisionToWorldSpace(const Vector& in, Vector& out) const
{

	if (!IsBoundsDefinedInEntitySpace() || (GetCollisionAngles() == vec3_angle))
	{
		VectorAdd(in, GetAbsOrigin(), out);
	}
	else
	{
		VectorTransform(in, CollisionToWorldTransform(), out);
	}
}

void entities::HBaseEntity::WorldToCollisionSpace(const Vector& in, Vector& out) const
{
	if (!IsBoundsDefinedInEntitySpace() || (GetCollisionAngles() == vec3_angle))
	{
		VectorSubtract(in, GetAbsOrigin(), out);
	}
	else
	{
		VectorITransform(in, CollisionToWorldTransform(), out);
	}
}

int entities::HBaseEntity::GetParentAttachment() const
{
	int result = 0;
	entprops->GetEntProp(GetIndex(), Prop_Data, "m_iParentAttachment", result);
	return result;
}

matrix3x4_t entities::HBaseEntity::GetParentToWorldTransform(matrix3x4_t& tempMatrix) const
{
	auto moveparent = GetMoveParent();

	if (!moveparent)
	{
		SetIdentityMatrix(tempMatrix);
		return tempMatrix;
	}

	if (GetParentAttachment() != 0)
	{
		auto file = __FILE__;
		// log until cbaseanimating is implemented
		smutils->LogError(myself, "Error! Code took incomplete path at %i(%s)", __LINE__, file);
		return tempMatrix;
	}

	HBaseEntity parentEntity(moveparent);
	matrix3x4_t result;
	parentEntity.CalcAbsolutePosition(result);
	return result;
}

int entities::HBaseEntity::GetEffects() const
{
	int effects = 0;
	entprops->GetEntProp(GetIndex(), Prop_Data, "m_fEffects", effects);
	return effects;
}

float entities::HBaseEntity::GetSimulationTime() const
{
	float time = 0.0f;
	entprops->GetEntPropFloat(GetIndex(), Prop_Data, "m_flSimulationTime", time);
	return time;
}

void entities::HBaseEntity::SetSimulationTime(float time) const
{
	entprops->SetEntPropFloat(GetIndex(), Prop_Data, "m_flSimulationTime", time);
}

void entities::HBaseEntity::Teleport(const Vector& origin, const QAngle* angles, const Vector* velocity) const
{
	SetAbsOrigin(origin);

	if (angles)
	{
		SetAbsAngles(*angles);
	}

	if (velocity)
	{
		SetAbsVelocity(*velocity);
	}

	SetSimulationTime(gpGlobals->curtime);
}

bool entities::HBaseEntity::IsDisabled() const
{
	bool result = false;
	entprops->GetEntPropBool(GetIndex(), Prop_Data, "m_bDisabled", result);
	return result;
}

RenderMode_t entities::HBaseEntity::GetRenderMode() const
{
	int render = 0;
	entprops->GetEntProp(GetIndex(), Prop_Data, "m_nRenderMode", render);
	return static_cast<RenderMode_t>(render);
}

int entities::HBaseEntity::GetHealth() const
{
	int value = 0;
	entprops->GetEntProp(GetIndex(), Prop_Data, "m_iHealth", value);
	return value;
}

int entities::HBaseEntity::GetMaxHealth() const
{
	int value = 0;
	entprops->GetEntProp(GetIndex(), Prop_Data, "m_iMaxHealth", value);
	return value;
}

int entities::HBaseEntity::GetTakeDamage() const
{
	int value = 0;
	entprops->GetEntProp(GetIndex(), Prop_Data, "m_takedamage", value);
	return value;
}

matrix3x4_t entities::HBaseEntity::EntityToWorldTransform() const
{
	// the real function only calculates the absolute position if the dirty flag is set.
	// since we don't have direct access to the member the result is stored, we always make the calculations.

	matrix3x4_t result;
	CalcAbsolutePosition(result);
	return result;
}

void entities::HBaseEntity::ComputeAbsPosition(const Vector& vecLocalPosition, Vector* pAbsPosition) const
{
	CBaseEntity* pMoveParent = GetMoveParent();

	if (pMoveParent == nullptr)
	{
		*pAbsPosition = vecLocalPosition;
	}
	else
	{
		HBaseEntity mp(pMoveParent);
		matrix3x4_t matrix;
		mp.CalcAbsolutePosition(matrix);

		VectorTransform(vecLocalPosition, matrix, *pAbsPosition);
	}
}

void entities::HBaseEntity::CalcAbsolutePosition(matrix3x4_t& result) const
{
	matrix3x4_t tempmat;
	auto angles = GetAbsAngles();
	auto origin = GetAbsOrigin();
	AngleMatrix(angles, origin, tempmat);

	auto moveparent = GetMoveParent();

	if (!moveparent)
	{
		// no move parent
		result = tempmat;
		return;
	}

	// concatenate with our parent's transform
	matrix3x4_t tempmat2, scratchspace;
	ConcatTransforms(GetParentToWorldTransform(scratchspace), tempmat, tempmat2);
	MatrixCopy(tempmat2, result);
	return;
}

entities::HFuncBrush::BrushSolidities_e entities::HFuncBrush::GetSolidity() const
{
	int solidity = 0;
	entprops->GetEntProp(GetIndex(), Prop_Data, "m_iSolidity", solidity);
	return static_cast<entities::HFuncBrush::BrushSolidities_e>(solidity);
}
