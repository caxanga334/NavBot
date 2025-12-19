#ifndef NAVBOT_CHASE_MESH_NAVIGATOR_H_
#define NAVBOT_CHASE_MESH_NAVIGATOR_H_

#include <entities/baseentity.h>
#include <sdkports/sdk_ehandle.h>
#include <sdkports/debugoverlay_shared.h>
#include "meshnavigator.h"

/**
 * @brief Navigator designed for chasing a moving target.
 */
class CChaseNavigator : public CMeshNavigator
{
public:
	enum SubjectLeadType : unsigned int
	{
		LEAD_SUBJECT = 0U, // The path will try to lead the target entity
		DONT_LEAD_SUBJECT, // The path won't try to lead the target entity, it will just move to the entity position itself
	};

	/**
	 * @brief A navigator designed for chasing entities.
	 * @param leadType Subject lead type.
	 * @param leadRadius Lead How far ahead of the subject the bot should try to move to.
	 */
	CChaseNavigator(SubjectLeadType leadType = LEAD_SUBJECT, const float leadRadius = 512.0f, const float lifeTime = -1.0f);
	~CChaseNavigator() override;

	void Invalidate() override;

	/**
	 * @brief Moves the bot along the path.
	 * @param bot Bot that will use this path.
	 * @param subject Subject the bot will chase.
	 * @param costFunc Path cost functor.
	 * @param predictedSubjectPosition Optional: If set the navigator won't calculate a predicted subject position and will use this instead.
	 */
	template <typename CF>
	void Update(CBaseBot* bot, CBaseEntity* subject, CF& costFunc, Vector* predictedSubjectPosition = nullptr);

	Vector PredictSubjectPosition(CBaseBot* bot, CBaseEntity* subject) const;

	// How far ahead of the subject the bot should try to move to.
	void SetLeadRadius(float radius) { m_leadRadius = radius; }
	float GetLeadRadius() const { return m_leadRadius; }
	void SetPathLifeTimeDuration(float lt) { m_lifetimeduration = lt; }
	float GetPathLifeTimeDuration() const { return m_lifetimeduration; }

protected:
	bool IsRepathNeeded(CBaseBot* bot, CBaseEntity* subject);

private:
	SubjectLeadType m_leadtype;
	CHandle<CBaseEntity> m_subject; // subject from the last valid path (tracks target entity changes)
	CountdownTimer m_failTimer; // path failed timer
	CountdownTimer m_lifeTimer; // maximum path life
	CountdownTimer m_throttleTimer; // prevent quick repaths
	float m_leadRadius;
	float m_lifetimeduration;
	float m_maxpathlength;

	void Update(CBaseBot* bot) override;

	template <typename CF>
	void RefreshPath(CBaseBot* bot, CBaseEntity* subject, CF& costFunc, Vector* predictedSubjectPosition = nullptr);
};

template<typename CF>
inline void CChaseNavigator::Update(CBaseBot* bot, CBaseEntity* subject, CF& costFunc, Vector* predictedSubjectPosition)
{
	RefreshPath(bot, subject, costFunc, predictedSubjectPosition);
	CMeshNavigator::Update(bot);
}

template<typename CF>
inline void CChaseNavigator::RefreshPath(CBaseBot* bot, CBaseEntity* subject, CF& costFunc, Vector* predictedSubjectPosition)
{
	auto mover = bot->GetMovementInterface();

	if (IsValid() && mover->IsOnLadder())
	{
		m_throttleTimer.Start(0.5f); // don't repath while using ladders
		return;
	}

	if (subject == nullptr)
	{
		return;
	}

	if (!m_failTimer.IsElapsed())
	{
		return;
	}

	if (subject != m_subject.Get())
	{
		// new chase target, refresh path

		if (bot->IsDebugging(BOTDEBUG_PATH))
		{
			bot->DebugPrintToConsole(255, 165, 0, "%s CChaseNavigator target subject changed from %p to %p!\n", bot->GetDebugIdentifier(), m_subject.Get(), subject);
		}

		Invalidate();

		m_failTimer.Invalidate();
	}

	if (IsValid() && !m_throttleTimer.IsElapsed())
	{
		// don't repath too frequently while the current path is valid
		return;
	}

	if (!IsValid() || IsRepathNeeded(bot, subject))
	{
		entities::HBaseEntity sbjEntity(subject);
		bool foundpath = false;
		Vector pathGoal = sbjEntity.GetAbsOrigin();

		if (m_leadtype == LEAD_SUBJECT)
		{
			// if a position was given, use it, otherwise calculate it.
			pathGoal = predictedSubjectPosition != nullptr ? *predictedSubjectPosition : PredictSubjectPosition(bot, subject);
			foundpath = this->ComputePathToPosition(bot, pathGoal, costFunc, m_maxpathlength);
		}
		else // don't lead subject
		{
			foundpath = this->ComputePathToPosition(bot, pathGoal, costFunc, m_maxpathlength);
		}

		if (foundpath)
		{
			if (bot->IsDebugging(BOTDEBUG_PATH))
			{
				bot->DebugPrintToConsole(255, 165, 0, "%s: CChaseNavigator::RefreshPath REPATH!\n", bot->GetDebugIdentifier());
			}

			m_subject = subject; // remember the subject of the last valid path
			m_throttleTimer.Start(0.5f); // don't repath frequently (unless the path becomes invalid)

			if (m_lifetimeduration > 0.9f)
			{
				m_lifeTimer.Start(m_lifetimeduration);
			}
			else
			{
				m_lifeTimer.Invalidate();
			}
		}
		else
		{
			Invalidate();
			Vector subjectPos = sbjEntity.GetAbsOrigin();

			// path to subject failed - try again later, time is based on distance.
			float time = (bot->GetRangeTo(subjectPos) * 0.005f);
			time = std::min(time, 3.0f); // allow a maximum fail time of 3 seconds.

			m_failTimer.Start(time);
			bot->OnMoveToFailure(this, IEventListener::MovementFailureType::FAIL_NO_PATH);

			if (bot->IsDebugging(BOTDEBUG_PATH))
			{
				bot->DebugPrintToConsole(255, 165, 0, "%s: CChaseNavigator::RefreshPath REPATH FAILED!\n", bot->GetDebugIdentifier());
				NDebugOverlay::EntityBounds(subject, 255, 0, 0, 150, 2.0f);
				Vector start = bot->GetAbsOrigin();
				NDebugOverlay::HorzArrow(start, pathGoal, 8.0f, 255, 0, 0, 255, true, 2.0f);
			}
		}
	}
}

#endif // !NAVBOT_CHASE_MESH_NAVIGATOR_H_
