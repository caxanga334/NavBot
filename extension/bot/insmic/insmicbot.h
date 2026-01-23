#ifndef __NAVBOT_INSMIC_BOT_H_
#define __NAVBOT_INSMIC_BOT_H_

#include <bot/basebot.h>
#include <mods/insmic/insmic_shareddefs.h>

class CInsMICBot : public CBaseBot
{
public:
	CInsMICBot(edict_t* edict);

	// Returns the bot's insurgency team
	insmic::InsMICTeam GetMyInsurgencyTeam() const
	{
		return static_cast<insmic::InsMICTeam>(GetCurrentTeamIndex());
	}

	bool HasJoinedGame() override;
	void TryJoinGame() override;

private:

};


#endif // !__NAVBOT_INSMIC_BOT_H_
