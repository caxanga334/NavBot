#ifndef __NAVBOT_CSS_MOD_H_
#define __NAVBOT_CSS_MOD_H_

#include "css_shareddefs.h"
#include "css_lib.h"
#include "css_buy_profile.h"
#include "../basemod.h"

class CConCommandArgs;
class CCSSBot;

class CCSSModSettings : public CModSettings
{
public:
	CCSSModSettings() :
		CModSettings()
	{
		m_eco_limit = 2000;
		m_team_eco_limit = 1500;
		m_timeleft_hurry = 40.0f;
		SetDefendRate(64); // increase default defend chance
	}

	int GetEcoLimit() const { return m_eco_limit; }
	int GetTeamEcoLimit() const { return m_team_eco_limit; }
	float GetTimeLeftToHurry() const { return m_timeleft_hurry; }

	void SetEcoLimit(int value) { m_eco_limit = value; }
	void SetTeamEcoLimit(int value) { m_team_eco_limit = value; }
	void SetTimeLeftToHurry(float value) { m_timeleft_hurry = value; }

protected:
	SourceMod::SMCResult ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value) override;

private:
	int m_eco_limit; // if a bot money is less than this, do a full eco
	int m_team_eco_limit; // if the team average money is less than this, do a full eco
	float m_timeleft_hurry; // if the time left in the round is less than this, hurry things up

};

class CCounterStrikeSourceMod : public CBaseMod
{
public:
	CCounterStrikeSourceMod();

	void FireGameEvent(IGameEvent* event) override;
	CBaseBot* AllocateBot(edict_t* edict) override;
	void OnRoundStart() override;
	void OnRoundEnd() override;
	CNavMesh* NavMeshFactory() override;
	const CCSSModSettings* GetCSSModSettings() const { return static_cast<const CCSSModSettings*>(GetModSettings()); }
	const counterstrikesource::BuyManager& GetBuyManager() const { return m_buymanager; }
	/**
	 * @brief Runs the buy logic for the given bot.
	 * @param bot Bot that will be buying weapons.
	 */
	void BuyWeapons(CCSSBot* bot) const;
	// Returns true if the c4 is planted and active.
	bool IsBombActive() const { return m_bombactive; }
	// Returns true if the bomb location is known by the cts.
	bool IsTheBombKnownByCTs() const { return m_bombisknown; }
	// Marks the bomb position was known by the CTs.
	void MarkBombAsKnown() { m_bombisknown = true; }
	// Gets the planted bomb entity. NULL if none.
	CBaseEntity* GetActiveBombEntity() const { return m_c4.Get(); }
	// CSS mod access
	static CCounterStrikeSourceMod* GetCSSMod();
protected:
	void RegisterModCommands() override;
	void OnPostInit() override;
	CModSettings* CreateModSettings() const override { return new CCSSModSettings; }

private:
	void OnFlashbangDetonated(int userid, const Vector& pos);
	void OnBombPlanted(int userid);
	// resets per round stuff
	void RoundReset();

	counterstrikesource::BuyManager m_buymanager;
	std::array<int, static_cast<std::size_t>(counterstrikesource::CSSTeam::MAX_CSS_TEAMS)> m_teammoney; // average team money
	bool m_bombactive;
	bool m_bombisknown;
	CHandle<CBaseEntity> m_c4;

	static void OnReloadBuyManagerCommand(const CConCommandArgs& args);
};


#endif // !__NAVBOT_CSS_MOD_H_
