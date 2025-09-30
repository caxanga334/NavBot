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
	// If skill_level is this value, this is a 'default' profile.
	static constexpr int SKILL_LEVEL_DEFAULT_PROFILE = -1;
	// If skill_level is this value, this is a randomly generated profile.
	static constexpr int SKILL_LEVEL_RANDOM_PROFILE = -2;

	DifficultyProfile()
	{
		skill_level = -1;
		game_awareness = 40;
		weapon_skill = 60;
		aimspeed = 800.0f;
		fov = 90;
		maxvisionrange = 2048;
		maxhearingrange = 750;
		minrecognitiontime = 0.350f;
		predict_projectiles = false;
		allow_headshots = false;
		can_dodge = false;
		aim_tracking_interval = 0.25f;
		aggressiveness = 20;
		teamwork = 20;
		ability_use_interval = 1.0f;
		health_critical_percent = 0.2f;
		health_low_percent = 0.5f;
		numerical_disadvantage_retreat_threshold = 5;
	}

	virtual ~DifficultyProfile() = default;
	// Randomizes the difficulty profile values
	virtual void RandomizeProfileValues();
	// Invoked for each profile after parsing the config file to validate values
	virtual void ValidateProfileValues();

	// user defined skill profiles starts from 0 and are always a positive skill level
	inline bool IsDefaultProfile() const { return skill_level == SKILL_LEVEL_DEFAULT_PROFILE; }
	inline bool IsRandomizedProfile() const { return skill_level == SKILL_LEVEL_RANDOM_PROFILE; }

	inline const int GetSkillLevel() const { return skill_level; }
	// Bot game awareness skill. Range: 0 - 100
	inline const int GetGameAwareness() const { return game_awareness; }
	// General purpose weapon usage skill. Range: 0 - 100
	inline const int GetWeaponSkill() const { return weapon_skill; }
	inline const float GetAimSpeed() const { return aimspeed; }
	inline const int GetFOV() const { return fov; }
	inline const int GetMaxVisionRange() const { return maxvisionrange; }
	inline const int GetMaxHearingRange() const { return maxhearingrange; }
	inline const float GetMinRecognitionTime() const { return minrecognitiontime; }
	inline const bool ShouldPredictProjectiles() const { return predict_projectiles; }
	inline const bool IsAllowedToHeadshot() const { return allow_headshots; }
	inline const bool IsAllowedToDodge() const { return can_dodge; }
	inline const float GetAimTrackingInterval() const { return aim_tracking_interval; }
	inline const int GetAggressiveness() const { return aggressiveness; }
	inline const int GetTeamwork() const { return teamwork; }
	inline const float GetAbilityUsageInterval() const { return ability_use_interval; }
	inline const float GetHealthCriticalThreshold() const { return health_critical_percent; }
	inline const float GetHealthLowThreshold() const { return health_low_percent; }
	inline const int GetRetreatFromNumericalDisadvantageThreshold() const { return numerical_disadvantage_retreat_threshold; }

	inline void SetSkillLevel(const int skill) { skill_level = skill; }
	inline void SetGameAwareness(const int awareness) { game_awareness = awareness; }
	inline void SetWeaponSkill(const int skill) { weapon_skill = skill; }
	inline void SetAimSpeed(const float speed) { aimspeed = speed; }
	inline void SetFOV(const int v) { fov = v; }
	inline void SetMaxVisionRange(const int range) { maxvisionrange = range; }
	inline void SetMaxHearingRange(const int range) { maxhearingrange = range; }
	inline void SetMinRecognitionTime(const float time) { minrecognitiontime = time; }
	inline void SetPredictProjectiles(const bool v) { predict_projectiles = v; }
	inline void SetAllowHeadshots(const bool v) { allow_headshots = v; }
	inline void SetIsAllowedToDodge(const bool v) { can_dodge = v; }
	inline void SetAimTrackingInterval(const float v) { aim_tracking_interval = v; }
	inline void SetAggressiveness(const int v) { aggressiveness = v; }
	inline void SetTeamwork(const int v) { teamwork = v; }
	inline void SetAbilityUsageInterval(const float v) { ability_use_interval = v; }
	inline void SetHealthCriticalThreshold(const float v) { health_critical_percent = v; }
	inline void SetHealthLowThreshold(const float v) { health_low_percent = v; }
	inline void SetRetreatFromNumericalDisadvantageThreshold(const int v) { numerical_disadvantage_retreat_threshold = v; }

private:
	int skill_level; // the skill level this profile represents
	int game_awareness; // general purpose game awareness skill
	int weapon_skill; // general purpose weapon skill
	float aimspeed; // Aiming speed cap
	int fov; // field of view in degrees
	int maxvisionrange; // maximum distance the bot is able to see
	int maxhearingrange; // maximum distace the bot is able to hear
	float minrecognitiontime; // minimum time for the bot to recognize an entity
	bool predict_projectiles; // if true, bot will lead their targets with projectile weapons
	bool allow_headshots; // Allow going for headshots
	bool can_dodge; // Is allowed to dodge attacks?
	float aim_tracking_interval; // Interval between aim target position updates
	int aggressiveness; // How aggressive will this bot play
	int teamwork; // How likely this bot will cooperate with teammates
	float ability_use_interval; // Interval between secondary abilities usage
	float health_critical_percent; // If the bot HP % is less than this, the bot health is critical
	float health_low_percent; // If the bot HP % is less than this, the bot health is low
	int numerical_disadvantage_retreat_threshold; // If the numerical disadvantage (number of enemies - number of allies) is greater than this, the bot will retreat
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
	void ForEveryProfile(F& functor)
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
