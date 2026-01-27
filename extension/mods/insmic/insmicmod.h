#ifndef __NAVBOT_INSMIC_MOD_H_
#define __NAVBOT_INSMIC_MOD_H_

#include <mods/basemod.h>
#include "insmic_lib.h"

class CInsMICBot;

class CInsMICMod : public CBaseMod
{
public:
	static CInsMICMod* GetInsurgencyMod();

	CInsMICMod();

	void FireGameEvent(IGameEvent* event) override;
	CBaseBot* AllocateBot(edict_t* edict) override;
	IModHelpers* AllocModHelpers() const override;
	/**
	 * @brief Selects a team and squad for a bot.
	 * @param[out] teamid Team ID to join.
	 * @param[out] encodedSquadData Encoded squad data for the client command.
	 */
	void SelectTeamAndSquadForBot(int& teamid, int& encodedSquadData) const;
	void OnMapStart() override;
	void OnRoundStart() override;
	// Returns the detected game type for the current map.
	insmic::InsMICGameTypes_t GetGameType() const { return m_gametype; }
	/**
	 * @brief Gets the given team's manager entity.
	 * @param teamid Team to get the manager entity of.
	 * @return Pointer to manager entity or NULL if not found.
	 */
	CBaseEntity* GetTeamManager(insmic::InsMICTeam teamid) const
	{
		switch (teamid)
		{
		case insmic::InsMICTeam::INSMICTEAM_USMC:
			return m_hUSMCTeamManager.Get();
		case insmic::InsMICTeam::INSMICTEAM_INSURGENTS:
			return m_hInsurgentsTeamManager.Get();
		default:
			return nullptr;
		}
	}
	/**
	 * @brief Gets a player encoded squad data.
	 * @param pPlayer Player to get the squad data of.
	 * @return Player's encoded squad data.
	 */
	int GetPlayerSquadData(CBaseEntity* pPlayer) const;
	/**
	 * @brief Returns a random objective the given team is able to capture.
	 * @param teamid Attacking team ID.
	 * @return Pointer to the objective entity a team can capture or NULL if none can be captured.
	 */
	CBaseEntity* GetObjectiveToCapture(insmic::InsMICTeam teamid) const;
	/**
	 * @brief Returns a random objective the given team is able to defend.
	 * @param teamid Defending team ID.
	 * @return Pointer to the objective entity a team can capture or NULL if none can be defended.
	 */
	CBaseEntity* GetObjectiveToDefend(insmic::InsMICTeam teamid) const;
protected:
	void RegisterModCommands() override;

private:
	insmic::InsMICGameTypes_t m_gametype;
	CHandle<CBaseEntity> m_hUSMCTeamManager;
	CHandle<CBaseEntity> m_hInsurgentsTeamManager;

	void DetectGameType();
	void FindTeamManagerEntities();
};

#endif // !__NAVBOT_INSMIC_MOD_H_
