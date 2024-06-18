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

	// Distance to consider the goal reached
	inline virtual void SetGoalTolerance(const float tolerance) { m_goalTolerance = tolerance; }
	inline virtual const float GetGoalTolerance() const { return m_goalTolerance; }
	// Maximum distance to skip path segments and walk in a straight line (if not blocked). Allows smooth movement
	inline virtual void SetSkipAheadDistance(const float distance) { m_skipAheadDistance = distance; }
	inline virtual const float GetSkipAheadDistance() { return m_skipAheadDistance; }

protected:
	// true while the bot is using ladders
	virtual bool LadderUpdate(CBaseBot* bot);
	virtual bool Climbing(CBaseBot* bot, const CBasePathSegment* segment, const Vector& forward, const Vector& right, const float goalRange);
	virtual bool JumpOverGaps(CBaseBot* bot, const CBasePathSegment* segment, const Vector& forward, const Vector& right, const float goalRange);
	virtual Vector Avoid(CBaseBot* bot, const Vector& goalPos, const Vector& forward, const Vector& left);
	virtual edict_t* FindBlocker(CBaseBot* bot);

private:
	CBaseBot* m_me; // bot that is using this navigator
	const CBasePathSegment* m_goal; // the segment the bot is currently trying to reach
	CountdownTimer m_waitTimer; // timer for the bot to wait for stuff (lifts, doors, obstacles, ...)
	CountdownTimer m_avoidTimer; // timer for collision avoidance
	CBaseHandle m_blocker; // Entity blocking my path
	bool m_didAvoidCheck;
	float m_goalTolerance;
	float m_skipAheadDistance;

	bool IsAtGoal(CBaseBot* bot);
	bool CheckProgress(CBaseBot* bot);
	const CBasePathSegment* CheckSkipPath(CBaseBot* bot, const CBasePathSegment* from) const;
};

#endif // !SMNAV_BOT_NAV_MESH_NAVIGATOR_H_
