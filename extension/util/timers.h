#ifndef __NAVBOT_UTILS_TIMERS_H_
#define __NAVBOT_UTILS_TIMERS_H_

class CTimerSystem
{
public:
	CTimerSystem()
	{
		m_inLevelTransition = true;
	}

	~CTimerSystem()
	{}

	static void CreateTimerSystem() { CTimerSystem::s_instance = std::make_unique<CTimerSystem>(); }
	static void DestroyTimerSystem() { CTimerSystem::s_instance.reset(nullptr); }
	static CTimerSystem* Get() { return CTimerSystem::s_instance.get(); }

	// reference to a timer
	using TimerRef = std::uintptr_t;

	enum TimerFlags : std::uint8_t
	{
		TIEMRFLAG_NONE = 0,
		TIMERFLAG_REPEAT = 0x1, // Timer repeats until callbacks returns false or killed externally
		TIMERFLAG_KEEP_BETWEEN_MAPS = 0x2, // Timer isn't removed on level transition
	};

	class CTimer
	{
	public:
		template <typename F, typename T>
		CTimer(F callback, T data) :
			CTimer()
		{
			m_callback = callback;
			m_data = data;
		}

		bool HasFlags(TimerFlags flags) const
		{
			return (m_flags & flags) != 0;
		}

		void AssignFlags(TimerFlags flags) { m_flags = flags; }

		void SetInterval(float time) { m_interval = time; }
		void SetNextFire(float time) { m_nextFire = time; }
		float GetInterval() const { return m_interval; }
		bool ShouldFire(const float time) const { return time >= m_nextFire; }
		bool IsRepeatingTimer() const { return HasFlags(CTimerSystem::TimerFlags::TIMERFLAG_REPEAT); }
		// Fires the timer callback, returns the result
		bool FireCallback() const
		{
			return m_callback(m_data);
		}

	private:
		friend class CTimerSystem;
		std::function<bool(const std::any&)> m_callback;
		std::any m_data;
		float m_interval;
		float m_nextFire;
		TimerFlags m_flags;

		CTimer()
		{
			m_interval = -1.0f;
			m_nextFire = 0.0f;
			m_flags = TimerFlags::TIEMRFLAG_NONE;
		}
	};

	/**
	 * @brief Creates a new timer.
	 * @tparam F callback function bool (const std::any& data)
	 * @param callback Callback function
	 * @param interval Delay in seconds between callback calls. Negative or zero for a once timer.
	 * @param flags Optional timer behavior flags.
	 * @return Reference to timer (used to kill a timer outside it's callback)
	 */
	template <typename F>
	TimerRef CreateTimer(F callback, float interval, TimerFlags flags = TimerFlags::TIEMRFLAG_NONE)
	{
		std::lock_guard lock(m_mutex);
		std::any empty;
		std::unique_ptr<CTimer> timer = std::make_unique<CTimer>(callback, empty);
		TimerRef ref = reinterpret_cast<TimerRef>(timer.get());
		timer->SetInterval(interval);
		timer->AssignFlags(flags);
		InitTimer(timer.get());
		m_timers.insert_or_assign(ref, std::move(timer));
		return ref;
	}

	/**
	 * @brief Cretes a new timer.
	 * @tparam F callback function bool (const std::any& data)
	 * @tparam T Data type
	 * @param callback Callback function.
	 * @param data Copy constructible data to pass to the timer callback.
	 * @param interval Delay in seconds between callback calls. Negative or zero for a once timer.
	 * @param flags Optional timer behavior flags.
	 * @return Reference to timer (used to kill a timer outside it's callback)
	 */
	template <typename F, typename T>
	TimerRef CreateTimer(F callback, T data, float interval, TimerFlags flags = TimerFlags::TIEMRFLAG_NONE)
	{
		static_assert(std::is_copy_constructible_v<T>);
		std::lock_guard lock(m_mutex);
		std::unique_ptr<CTimer> timer = std::make_unique<CTimer>(callback, data);
		TimerRef ref = reinterpret_cast<TimerRef>(timer.get());
		timer->SetInterval(interval);
		timer->AssignFlags(flags);
		InitTimer(timer.get());
		m_timers.insert_or_assign(ref, std::move(timer));
		return ref;
	}

	/**
	 * @brief Stops a timer by their reference.
	 * @param ref Reference of the timer to stop.
	 */
	void KillTimer(TimerRef ref)
	{
		std::lock_guard lock(m_mutex);
		m_timers.erase(ref);
	}

	void Think();
	void OnMapStart();
	void OnMapEnd();

private:
	std::mutex m_mutex; // mutex timers
	std::unordered_map<TimerRef, std::unique_ptr<CTimer>> m_timers;
	bool m_inLevelTransition;

	void InitTimer(CTimer* timer) const;

	static inline std::unique_ptr<CTimerSystem> s_instance{ nullptr };
};



#endif // !__NAVBOT_UTILS_TIMERS_H_
