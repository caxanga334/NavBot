#ifndef NAVBOT_BOT_DIFFICULTY_PROFILE_H_
#define NAVBOT_BOT_DIFFICULTY_PROFILE_H_

#include <vector>
#include <memory>
#include <unordered_map>
#include <type_traits>
#include <cmath>

#include <ITextParsers.h>

// Bot difficulty profile
class DifficultyProfile
{
public:
	DifficultyProfile() :
		skill_level(-1), aimspeed(1200.0f), fov(90), maxvisionrange(2048), maxhearingrange(512),
		minrecognitiontime(0.3f) 
	{
		custom_data.reserve(4);
	}

	DifficultyProfile(const DifficultyProfile& other)
	{
		*this = other;
	}

	inline DifficultyProfile& operator=(const DifficultyProfile& other)
	{
		this->skill_level = other.skill_level;
		this->aimspeed = other.aimspeed;
		this->fov = other.fov;
		this->maxvisionrange = other.maxvisionrange;
		this->maxhearingrange = other.maxhearingrange;
		this->minrecognitiontime = other.minrecognitiontime;
		return *this;
	}

	// user defined skill profiles starts from 0 and are always a positive skill level
	inline bool IsDefaultProfile() const { return skill_level < 0; }

	inline const int GetSkillLevel() const { return skill_level; }
	inline const float GetAimSpeed() const { return aimspeed; }
	inline const int GetFOV() const { return fov; }
	inline const int GetMaxVisionRange() const { return maxvisionrange; }
	inline const int GetMaxHearingRange() const { return maxhearingrange; }
	inline const float GetMinRecognitionTime() const { return minrecognitiontime; }

	inline void SetSkillLevel(const int skill) { skill_level = skill; }
	inline void SetAimSpeed(const float speed) { aimspeed = speed; }
	inline void SetFOV(const int v) { fov = v; }
	inline void SetMaxVisionRange(const int range) { maxvisionrange = range; }
	inline void SetMaxHearingRange(const int range) { maxhearingrange = range; }
	inline void SetMinRecognitionTime(const float time) { minrecognitiontime = time; }

	inline void SaveCustomData(std::string key, float data)
	{
		custom_data[key] = data;
	}

	inline bool ContainsCustomData(std::string key) const
	{
		return custom_data.count(key) > 0U;
	}

	/**
	 * @brief Retrieves a custom data value from the difficulty profile.
	 * @tparam T Data type to convert to.
	 * 
	 * A static assertion will fail if the data type is not supported.
	 * @param key Custom data key name
	 * @param defaultValue Default value to return if the key doesn't exists.
	 * @return Key value or default if not found
	 */
	template <typename T>
	inline T GetCustomData(std::string key, T defaultValue) const
	{
		if constexpr (std::is_same<T, bool>::value || std::is_same<T, const bool>::value)
		{
			auto it = custom_data.find(key);

			if (it != custom_data.end())
			{
				if (it->second > -0.5f && it->second < 0.5f)
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			else // not found
			{
				return defaultValue;
			}
		}
		else if constexpr (std::is_same<T, int>::value || std::is_same<T, const int>::value)
		{
			auto it = custom_data.find(key);

			if (it != custom_data.end())
			{
				return static_cast<int>(std::roundf(it->second));
				
			}
			else // not found
			{
				return defaultValue;
			}
		}
		else if constexpr (std::is_same<T, float>::value || std::is_same<T, const float>::value)
		{
			auto it = custom_data.find(key);

			if (it != custom_data.end())
			{
				return it->second;

			}
			else // not found
			{
				return defaultValue;
			}
		}
		else
		{
			static_assert(false, "GetCustomData unsupported data type!");
		}
	}

private:
	int skill_level; // the skill level this profile represents
	float aimspeed; // Aiming speed cap
	int fov; // field of view in degrees
	int maxvisionrange; // maximum distance the bot is able to see
	int maxhearingrange; // maximum distace the bot is able to hear
	float minrecognitiontime; // minimum time for the bot to recognize an entity
	std::unordered_map<std::string, float> custom_data; // allow mods to have custom data on profile without changing this class
};

// Bot difficulty profile manager
class CDifficultyManager : public SourceMod::ITextListener_SMC
{
public:
	CDifficultyManager()
	{
		m_profiles.reserve(32);
		m_parser_depth = 0;
		m_current = nullptr;
	}

	~CDifficultyManager();

	void LoadProfiles();

	/**
	 * @brief Given a skill level, gets a random difficulty profile for that skill level
	 * @param level Skill level
	 * @return Difficulty profile
	*/
	std::shared_ptr<DifficultyProfile> GetProfileForSkillLevel(const int level) const;

	/**
	 * @brief Called when starting parsing.
	 */
	void ReadSMC_ParseStart() override;

	/**
	 * @brief Called when ending parsing.
	 *
	 * @param halted			True if abnormally halted, false otherwise.
	 * @param failed			True if parsing failed, false otherwise.
	 */
	void ReadSMC_ParseEnd(bool halted, bool failed) override;

	/**
	 * @brief Called when entering a new section
	 *
	 * @param states		Parsing states.
	 * @param name			Name of section, with the colon omitted.
	 * @return				SMCResult directive.
	 */
	SourceMod::SMCResult ReadSMC_NewSection(const SourceMod::SMCStates* states, const char* name) override;

	/**
	 * @brief Called when encountering a key/value pair in a section.
	 *
	 * @param states		Parsing states.
	 * @param key			Key string.
	 * @param value			Value string.  If no quotes were specified, this will be NULL,
	 *						and key will contain the entire string.
	 * @return				SMCResult directive.
	 */
	SourceMod::SMCResult ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value) override;

	/**
	 * @brief Called when leaving the current section.
	 *
	 * @param states		Parsing states.
	 * @return				SMCResult directive.
	 */
	SourceMod::SMCResult ReadSMC_LeavingSection(const SourceMod::SMCStates* states) override;

private:
	std::vector<std::shared_ptr<DifficultyProfile>> m_profiles;
	DifficultyProfile* m_current; // current profile being parsed
	int m_parser_depth;
};

#endif // !NAVBOT_BOT_DIFFICULTY_PROFILE_H_
