#ifndef __NAVBOT_SENSOR_INTERFACE_UTILITY_H_
#define __NAVBOT_SENSOR_INTERFACE_UTILITY_H_

class ISensor;
class CBaseEntity;

namespace sensorutils
{
	/**
	 * @brief Utility class for overrinding a bot's primary threat.
	 * The destructor guarantees that the override is cleared.
	 * @tparam T Bot class.
	 */
	template <typename T>
	class PrimaryThreatOverride
	{
	public:
		PrimaryThreatOverride() :
			m_me(nullptr), m_sensor(nullptr)
		{
		}

		PrimaryThreatOverride(T* bot, CBaseEntity* threat) :
			m_me(bot), m_sensor(bot->GetSensorInterface())
		{
			bot->GetSensorInterface()->SetPrimaryThreatOverride(threat);
		}

		~PrimaryThreatOverride()
		{
			if (m_me && m_sensor)
			{
				m_sensor->SetPrimaryThreatOverride(nullptr);
			}
		}

		bool IsSet() const { return m_sensor != nullptr; }

		/**
		 * @brief Assigns a primary threat override.
		 * @param bot Bot to override the primary treat.
		 * @param threat Threat to set.
		 */
		void Set(T* bot, CBaseEntity* threat)
		{
			m_me = bot;
			m_sensor = bot->GetSensorInterface();
			bot->GetSensorInterface()->SetPrimaryThreatOverride(threat);
		}
		/**
		 * @brief Clears the primary threat override.
		 */
		void Clear()
		{
			if (m_me && m_sensor)
			{
				m_sensor->SetPrimaryThreatOverride(nullptr);
			}

			m_me = nullptr;
			m_sensor = nullptr;
		}

	private:
		T* m_me;
		ISensor* m_sensor;
	};
}



#endif // !__NAVBOT_SENSOR_INTERFACE_UTILITY_H_
