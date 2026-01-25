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

	CBaseBot* AllocateBot(edict_t* edict) override;
	IModHelpers* AllocModHelpers() const override;
	/**
	 * @brief Selects a team and squad for a bot.
	 * @param[out] teamid Team ID to join.
	 * @param[out] encodedSquadData Encoded squad data for the client command.
	 */
	void SelectTeamAndSquadForBot(int& teamid, int& encodedSquadData) const;
	void OnMapStart() override;
	// Returns the detected game type for the current map.
	insmic::InsMICGameTypes_t GetGameType() const { return m_gametype; }
protected:
	void RegisterModCommands() override;

private:
	insmic::InsMICGameTypes_t m_gametype;

	void DetectGameType();
};

#endif // !__NAVBOT_INSMIC_MOD_H_
