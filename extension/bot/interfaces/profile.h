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
	DifficultyProfile()
	{
		skill_level = -1;
		aimspeed = 800.0f;
		fov = 90;
		maxvisionrange = 2048;
		maxhearingrange = 750;
		minrecognitiontime = 0.350f;
		predict_projectiles = false;
		allow_headshots = false;
		aim_lockin_time = 0.3f;
		aim_moving_error = 0.2f;
		aim_minspeed_for_error = 250.0f;
	}

	// user defined skill profiles starts from 0 and are always a positive skill level
	inline bool IsDefaultProfile() const { return skill_level < 0; }

	inline const int GetSkillLevel() const { return skill_level; }
	inline const float GetAimSpeed() const { return aimspeed; }
	inline const int GetFOV() const { return fov; }
	inline const int GetMaxVisionRange() const { return maxvisionrange; }
	inline const int GetMaxHearingRange() const { return maxhearingrange; }
	inline const float GetMinRecognitionTime() const { return minrecognitiontime; }
	inline const bool ShouldPredictProjectiles() const { return predict_projectiles; }
	inline const bool IsAllowedToHeadshot() const { return allow_headshots; }
	inline const float GetAimLockInTime() const { return aim_lockin_time; }
	inline const float GetAimMovingError() const { return aim_moving_error; }
	inline const float GetAimMinSpeedForError() const { return aim_minspeed_for_error; }

	inline void SetSkillLevel(const int skill) { skill_level = skill; }
	inline void SetAimSpeed(const float speed) { aimspeed = speed; }
	inline void SetFOV(const int v) { fov = v; }
	inline void SetMaxVisionRange(const int range) { maxvisionrange = range; }
	inline void SetMaxHearingRange(const int range) { maxhearingrange = range; }
	inline void SetMinRecognitionTime(const float time) { minrecognitiontime = time; }
	inline void SetPredictProjectiles(const bool v) { predict_projectiles = v; }
	inline void SetAllowHeadshots(const bool v) { allow_headshots = v; }
	inline void SetAimLockInTime(const float time) { aim_lockin_time = time; }
	inline void SetAimMovingError(const float error) { aim_moving_error = error; }
	inline void SetAimMinSpeedForError(const float speed) { aim_minspeed_for_error = speed; }

private:
	int skill_level; // the skill level this profile represents
	float aimspeed; // Aiming speed cap
	int fov; // field of view in degrees
	int maxvisionrange; // maximum distance the bot is able to see
	int maxhearingrange; // maximum distace the bot is able to hear
	float minrecognitiontime; // minimum time for the bot to recognize an entity
	bool predict_projectiles; // if true, bot will lead their targets with projectile weapons
	bool allow_headshots; // Allow going for headshots
	float aim_lockin_time; // Time in seconds until the bot aim 'locks' into the target
	float aim_moving_error; // Aim position error for moving targets
	float aim_minspeed_for_error; // if the target speed is greater than this, apply error.
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

	virtual ~CDifficultyManager();

	void LoadProfiles();

	virtual DifficultyProfile* CreateNewProfile() const { return new DifficultyProfile; }

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

	/**
	 * @brief Runs a function on every bot difficulty profile.
	 * @tparam F Functor: void (T* profile)
	 * @tparam T Difficulty profile class to use.
	 * @param functor Function to run.
	 */
	template <typename T, typename F>
	void ForEveryProfile(F functor)
	{
		for (auto& ptr : m_profiles)
		{
			T* profile = static_cast<T*>(ptr.get());
			functor(profile);
		}
	}

private:
	std::vector<std::shared_ptr<DifficultyProfile>> m_profiles;

protected:
	int m_parser_depth;
	DifficultyProfile* m_current; // current profile being parsed
};

#endif // !NAVBOT_BOT_DIFFICULTY_PROFILE_H_
