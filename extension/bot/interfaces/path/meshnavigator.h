#ifndef SMNAV_BOT_NAV_MESH_NAVIGATOR_H_
#define SMNAV_BOT_NAV_MESH_NAVIGATOR_H_
#pragma once

#include "basepath.h"

class CBaseBot;

// Nav Mesh navigator
class CMeshNavigator : public CPath
{
public:
	CMeshNavigator();
	virtual ~CMeshNavigator();

	virtual void Invalidate() override;
	virtual void OnPathChanged(CBaseBot* bot, AIPath::ResultType result) override;

	// Moves the bot along the path
	virtual void Update(CBaseBot* bot);

	virtual const CBasePathSegment* GetGoalSegment() const override { return m_goal; }

	virtual bool IsAtGoal(CBaseBot* bot);
	// Distance to consider the goal reached
	inline virtual void SetGoalTolerance(const float tolerance) { m_goalTolerance = tolerance; }
	inline virtual const float GetGoalTolerance() const { return m_goalTolerance; }
	// Maximum distance to skip path segments and walk in a straight line (if not blocked). Allows smooth movement
	inline virtual void SetSkipAheadDistance(const float distance) { m_skipAheadDistance = distance; }
	inline virtual const float GetSkipAheadDistance() { return m_skipAheadDistance; }

private:
	const CBasePathSegment* m_goal; // the segment the bot is currently trying to reach
	CountdownTimer m_waitTimer; // timer for the bot to wait for stuff (lifts, doors, obstacles, ...)
	float m_goalTolerance;
	float m_skipAheadDistance;
};

#endif // !SMNAV_BOT_NAV_MESH_NAVIGATOR_H_
