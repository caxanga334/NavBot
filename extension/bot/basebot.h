#ifndef EXT_BASE_BOT_H_
#define EXT_BASE_BOT_H_

#include <extplayer.h>

class CBaseBot : public CBaseExtPlayer
{
public:
	CBaseBot(edict_t* edict);
	virtual ~CBaseBot();

	// Called by the manager for all players every server frame.
	// Overriden to call bot functions
	virtual void PlayerThink() override;

	// true if this is a bot managed by this extension
	virtual bool IsExtensionBot() override { return true; }
	// Pointer to the extension bot class
	virtual CBaseBot* MyBotPointer() override;

	// Reset the bot to it's initial state
	virtual void Reset();
	// Function called at intervals to run the AI 
	virtual void Update();
	// Function called every server frame to run the AI
	virtual void Frame();

private:
	int m_nextupdatetime;
};

#endif // !EXT_BASE_BOT_H_
