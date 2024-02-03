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

Vector entities::HBaseEntity::GetAbsOrigin() const
{
	Vector result;

	if (entprops->GetEntPropVector(GetIndex(), Prop_Data, "m_vecOrigin", result) == true)
	{
		return result;
	}

	return vec3_origin;
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

void entities::HBaseEntity::CalcNearestPoint(const Vector& worldPos, Vector& out) const
{
	Vector localPt, localClosestPt;
	WorldToCollisionSpace(worldPos, localPt);
	UtilHelpers::CalcClosestPointOnAABB(WorldAlignMins(), WorldAlignMaxs(), localPt, localClosestPt);
	CollisionToWorldSpace(localClosestPt, out);
}

void entities::HBaseEntity::GetTargetName(char* result, std::size_t maxsize) const
{
	int size = 0;
	entprops->GetEntPropString(GetIndex(), Prop_Data, "m_iName", result, maxsize, size);
}

CBaseEntity* entities::HBaseEntity::GetOwnerEntity() const
{
	CBaseEntity* owner = nullptr;
	int ref = INVALID_ENT_REFERENCE;
	entprops->GetEntPropEnt(GetIndex(), Prop_Data, "m_hOwnerEntity", ref);
	UtilHelpers::IndexToAThings(ref, &owner, nullptr);
	return owner;
}

CBaseEntity* entities::HBaseEntity::GetMoveParent() const
{
	CBaseEntity* entity = nullptr;
	int ref = INVALID_ENT_REFERENCE;
	entprops->GetEntPropEnt(GetIndex(), Prop_Data, "m_hMoveParent", ref);
	UtilHelpers::IndexToAThings(ref, &entity, nullptr);
	return entity;
}

CBaseEntity* entities::HBaseEntity::GetMoveChild() const
{
	CBaseEntity* entity = nullptr;
	int ref = INVALID_ENT_REFERENCE;
	entprops->GetEntPropEnt(GetIndex(), Prop_Data, "m_hMoveChild", ref);
	UtilHelpers::IndexToAThings(ref, &entity, nullptr);
	return entity;
}

CBaseEntity* entities::HBaseEntity::GetMovePeer() const
{
	CBaseEntity* entity = nullptr;
	int ref = INVALID_ENT_REFERENCE;
	entprops->GetEntPropEnt(GetIndex(), Prop_Data, "m_hMovePeer", ref);
	UtilHelpers::IndexToAThings(ref, &entity, nullptr);
	return entity;
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





