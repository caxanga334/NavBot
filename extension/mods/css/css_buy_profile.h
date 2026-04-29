#ifndef __NAVBOT_MOD_CSS_BUY_PROFILE_H_
#define __NAVBOT_MOD_CSS_BUY_PROFILE_H_

#include <array>
#include <vector>
#include <string>
#include <string_view>
#include <ITextParsers.h>

namespace counterstrikesource
{
	class BuyProfile
	{
	public:
		static constexpr std::size_t MAX_ENTRIES = 6U;

		BuyProfile()
		{
			m_minmoneyfornades = 2000;
			m_minmoneyforsecupgrade = 9000;
			m_defuserchance = 33;
			m_hechance = 33;
			m_flashchance = 25;
			m_smokechance = 15;
		}

		void AddPrimary(const char* weapon)
		{
			for (auto& entry : m_primaries)
			{
				if (entry.empty())
				{
					entry.assign(weapon);
					return;
				}
			}
		}

		void AddSecondary(const char* weapon)
		{
			for (auto& entry : m_secondaries)
			{
				if (entry.empty())
				{
					entry.assign(weapon);
					return;
				}
			}
		}

		bool ParseOption(const char* key, const char* value);

		std::string m_name; // profile name (for debugging)
		std::array<std::string, MAX_ENTRIES> m_primaries; // list primaries to try buying
		std::array<std::string, MAX_ENTRIES> m_secondaries; // list of secondaries to try buying
		int m_minmoneyfornades; // minimum money to consider buying grenades
		int m_minmoneyforsecupgrade; // minimum money to consider upgrading the secondary weapon
		int m_defuserchance; // chance to buy a defuser as CT
		int m_hechance; // chance to buy a he grenade
		int m_flashchance; // chance to buy a flashbang
		int m_smokechance; // chance to buy a smoke grenade
	};

	// buy option name, price
	using PriceEntry = std::pair<std::string, int>;
	// buy option name, team number
	using TeamRestrictEntry = std::pair<std::string, int>;

	class BuyManager : public SourceMod::ITextListener_SMC
	{
	public:
		BuyManager();

		bool ParseConfigFile();
		const BuyProfile* GetRandomBuyProfile() const;
		int LookupPrice(const char* name) const;
		bool IsAvailableToTeam(const char* name, const counterstrikesource::CSSTeam team) const;

	private:
		std::vector<BuyProfile> m_buyprofiles;
		std::vector<PriceEntry> m_prices;
		std::vector<TeamRestrictEntry> m_teamrestrictions;
		BuyProfile* m_current_profile;
		int m_parser_section;

		void ReadSMC_ParseStart() override;
		SourceMod::SMCResult ReadSMC_NewSection(const SourceMod::SMCStates* states, const char* name) override;
		SourceMod::SMCResult ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value) override;
		SourceMod::SMCResult ReadSMC_LeavingSection(const SourceMod::SMCStates* states) override;

		BuyProfile* AddNewProfile(const char* name);
		// returns false if duplicated
		bool AddPrice(const char* name, int price);
		bool AddTeamRestricion(const char* key, const char* value);
		int ParseTeamName(const char* name);

		static constexpr std::string_view CONFIG_FILE_NAME = "buy_profiles.cfg";
		static constexpr std::string_view USER_CONFIG_FILE_NAME = "buy_profiles.custom.cfg";
		static constexpr int SECTION_INIT = 0;
		static constexpr int SECTION_PROFILES = 1;
		static constexpr int SECTION_PRICES = 2;
		static constexpr int SECTION_PROFILE = 3;
		static constexpr int SECTION_PRIMARIES = 4;
		static constexpr int SECTION_SECONDARIES = 5;
		static constexpr int SECTION_OPTIONS = 6;
		static constexpr int SECTION_TEAM_RESTRICTIONS = 7;
	};
}

#endif // !__NAVBOT_MOD_CSS_BUY_PROFILE_H_