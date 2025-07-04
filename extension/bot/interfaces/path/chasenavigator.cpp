#include NAVBOT_PCH_FILE
#include <algorithm>
#include <extension.h>
#include <sdkports/debugoverlay_shared.h>
#include "chasenavigator.h"

#undef min
#undef max
#undef clamp

CChaseNavigator::CChaseNavigator(SubjectLeadType leadType, const float leadRadius, const float lifeTime)
{
	m_leadtype = leadType;
	m_leadRadius = leadRadius;
	m_lifetimeduration = lifeTime;
	m_maxpathlength = 0.0f;
	m_subject = nullptr;
	m_failTimer.Invalidate();
	m_lifeTimer.Invalidate();
	m_throttleTimer.Invalidate();
}

CChaseNavigator::~CChaseNavigator()
{
}

void CChaseNavigator::Invalidate()
{
	m_lifeTimer.Invalidate();
	m_throttleTimer.Invalidate();

	CMeshNavigator::Invalidate();
}

Vector CChaseNavigator::PredictSubjectPosition(CBaseBot* bot, CBaseEntity* subject) const
{
	auto mover = bot->GetMovementInterface();
	entities::HBaseEntity sbjEntity(subject);

	const Vector subjectPos = sbjEntity.GetAbsOrigin();

	Vector to = (subjectPos - bot->GetAbsOrigin());
	to.z = 0.0f;
	float flRangeSq = to.LengthSqr();

	// don't lead if subject is very far away
	float flLeadRadiusSq = GetLeadRadius();
	flLeadRadiusSq *= flLeadRadiusSq;
	if (flRangeSq > flLeadRadiusSq)
		return subjectPos;

	// Normalize in place
	float range = sqrt(flRangeSq);
	to /= (range + 0.0001f);	// avoid divide by zero

	// estimate time to reach subject, assuming maximum speed
	float leadTime = 0.5f + (range / (mover->GetDesiredSpeed() + 0.0001f));

	// estimate amount to lead the subject	
	Vector lead = leadTime * sbjEntity.GetAbsVelocity();
	lead.z = 0.0f;

	if (DotProduct(to, lead) < 0.0f)
	{
		// the subject is moving towards us - only pay attention 
		// to his perpendicular velocity for leading
		Vector2D to2D = to.AsVector2D();
		to2D.NormalizeInPlace();

		Vector2D perp(-to2D.y, to2D.x);

		float enemyGroundSpeed = lead.x * perp.x + lead.y * perp.y;

		lead.x = enemyGroundSpeed * perp.x;
		lead.y = enemyGroundSpeed * perp.y;
	}

	// compute our desired destination
	Vector pathTarget = subjectPos + lead;

	// validate this destination

	// don't lead through walls
	if (lead.LengthSqr() > 36.0f)
	{
		float fraction = 0.0f;

		if (!mover->IsPotentiallyTraversable(subjectPos, pathTarget, &fraction, true, nullptr))
		{
			// tried to lead through an unwalkable area - clip to walkable space
			pathTarget = subjectPos + fraction * (pathTarget - subjectPos);
		}
	}

	// don't lead over cliffs
	CNavArea* leadArea = nullptr;

	leadArea = TheNavMesh->GetNearestNavArea(pathTarget);

	if (leadArea == nullptr || leadArea->GetZ(pathTarget.x, pathTarget.y) < pathTarget.z - mover->GetMaxJumpHeight())
	{
		// would fall off a cliff
		return subjectPos;
	}

	return pathTarget;
}

bool CChaseNavigator::IsRepathNeeded(CBaseBot* bot, CBaseEntity* subject)
{
	entities::HBaseEntity sbjEntity(subject);

	// the closer we get, the more accurate our path needs to be
	Vector to = (sbjEntity.GetAbsOrigin() - bot->GetAbsOrigin());

	constexpr float minTolerance = 0.0f;
	constexpr float toleranceRate = 0.33f;

	float tolerance = minTolerance + toleranceRate * to.Length();

	return (sbjEntity.GetAbsOrigin() - GetEndPosition()).IsLengthGreaterThan(tolerance);
}

void CChaseNavigator::Update(CBaseBot* bot)
{
	CMeshNavigator::Update(bot);
}
