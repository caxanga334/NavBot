#include NAVBOT_PCH_FILE
#include "timers.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

void CTimerSystem::Think()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CTimerSystem::Think", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (m_inLevelTransition) { return; }

	std::lock_guard lock(m_mutex);

	if (m_timers.empty()) { return; }

	const float now = gpGlobals->curtime;

	for (auto iter = m_timers.begin(); iter != m_timers.end(); )
	{
		auto& [key, timer] = *iter;

		if (timer->ShouldFire(now))
		{
			// if the callback returns false, the timer is destroyed
			if (!timer->FireCallback())
			{
				iter = m_timers.erase(iter);
				continue;
			}

			if (!timer->IsRepeatingTimer())
			{
				iter = m_timers.erase(iter);
				continue;
			}

			float nextFire = now + timer->GetInterval();
			timer->SetNextFire(nextFire);
		}

		iter++;
	}
}

void CTimerSystem::OnMapStart()
{
	m_inLevelTransition = false;
}

void CTimerSystem::OnMapEnd()
{
	m_inLevelTransition = true;

	std::lock_guard lock(m_mutex);

	if (m_timers.empty()) { return; }

	std::erase_if(m_timers, [](const auto& pair) {
		if (pair.second->HasFlags(CTimerSystem::TimerFlags::TIMERFLAG_KEEP_BETWEEN_MAPS))
		{
			return false;
		}

		return true;
	});
}

void CTimerSystem::InitTimer(CTimer* timer) const
{
	timer->SetNextFire(gpGlobals->curtime + timer->GetInterval());
}
