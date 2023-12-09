#ifndef SMNAV_BOT_DIFFICULTY_PROFILE_H_
#define SMNAV_BOT_DIFFICULTY_PROFILE_H_
#pragma once

#include <vector>

#include <ITextParsers.h>

// Bot difficulty profile
class DifficultyProfile
{
public:
	DifficultyProfile() :
		skill_level(-1), aimspeed(25.0f), fov(90), maxvisionrange(2048), maxhearingrange(512) {}

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
		return *this;
	}

	// user defined skill profiles starts from 0 and are always a positive skill level
	inline bool IsDefaultProfile() const { return skill_level < 0; }

	inline const int GetSkillLevel() const { return skill_level; }
	inline const float GetAimSpeed() const { return aimspeed; }
	inline const int GetFOV() const { return fov; }
	inline const int GetMaxVisionRange() const { return maxvisionrange; }
	inline const int GetMaxHearingRange() const { return maxhearingrange; }

	inline void SetSkillLevel(const int skill) { skill_level = skill; }
	inline void SetAimSpeed(const float speed) { aimspeed = speed; }
	inline void SetFOV(const int v) { fov = v; }
	inline void SetMaxVisionRange(const int range) { maxvisionrange = range; }
	inline void SetMaxHearingRange(const int range) { maxhearingrange = range; }

private:
	int skill_level; // the skill level this profile represents
	float aimspeed; // how fast the bot aim approaches a given look at vector
	int fov; // field of view in degrees
	int maxvisionrange; // maximum distance the bot is able to see
	int maxhearingrange; // maximum distace the bot is able to hear
};

// Bot difficulty profile manager
class CDifficultyManager : public SourceMod::ITextListener_SMC
{
public:
	CDifficultyManager() :
		m_data(),
		m_newprofile(false),
		m_parser_header(false)
	{
		m_profiles.reserve(64);
	}

	~CDifficultyManager();

	void LoadProfiles();

	/**
	 * @brief Given a skill level, gets a random difficulty profile for that skill level
	 * @param level Skill level
	 * @return Difficulty profile
	*/
	DifficultyProfile GetProfileForSkillLevel(const int level) const;

	/**
	 * @brief Called when starting parsing.
	 */
	virtual void ReadSMC_ParseStart() override;

	/**
	 * @brief Called when ending parsing.
	 *
	 * @param halted			True if abnormally halted, false otherwise.
	 * @param failed			True if parsing failed, false otherwise.
	 */
	virtual void ReadSMC_ParseEnd(bool halted, bool failed) override;

	/**
	 * @brief Called when entering a new section
	 *
	 * @param states		Parsing states.
	 * @param name			Name of section, with the colon omitted.
	 * @return				SMCResult directive.
	 */
	virtual SourceMod::SMCResult ReadSMC_NewSection(const SourceMod::SMCStates* states, const char* name) override;

	/**
	 * @brief Called when encountering a key/value pair in a section.
	 *
	 * @param states		Parsing states.
	 * @param key			Key string.
	 * @param value			Value string.  If no quotes were specified, this will be NULL,
	 *						and key will contain the entire string.
	 * @return				SMCResult directive.
	 */
	virtual SourceMod::SMCResult ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value) override;

	/**
	 * @brief Called when leaving the current section.
	 *
	 * @param states		Parsing states.
	 * @return				SMCResult directive.
	 */
	virtual SourceMod::SMCResult ReadSMC_LeavingSection(const SourceMod::SMCStates* states) override;

private:
	struct ProfileData
	{
		int skill_level; // the skill level this profile represents
		float aimspeed; // how fast the bot aim approaches a given look at vector
		int fov; // field of view in degrees
		int maxvisionrange; // maximum distance the bot is able to see
		int maxhearingrange; // maximum distace the bot is able to hear

		// Initialize the profile data
		inline void OnNew()
		{
			skill_level = 0;
			aimspeed = 15;
			fov = 75;
			maxvisionrange = 1500;
			maxhearingrange = 750;
		}
	};

	DifficultyProfile m_default;
	std::vector<DifficultyProfile> m_profiles;
	bool m_newprofile; // reading a new profile
	bool m_parser_header;
	ProfileData m_data; // data read from the configuration file
};

#endif // !SMNAV_BOT_DIFFICULTY_PROFILE_H_
