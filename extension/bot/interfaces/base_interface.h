#ifndef SMNAV_BOT_BASE_INTERFACE_H_
#define SMNAV_BOT_BASE_INTERFACE_H_
#pragma once

class CBaseBot;

#include "event_listener.h"

/**
 * @brief Base class for bot interfaces. A system that allows to expand the bot capabilities
*/
class IBotInterface : public IEventListener
{
public:
	IBotInterface(CBaseBot* bot);
	virtual ~IBotInterface();

	virtual CBaseBot* GetBot() const { return m_bot; }

	virtual void OnDifficultyProfileChanged() {}

	// Reset the interface to it's initial state
	virtual void Reset() = 0;
	// Called at intervals
	virtual void Update() = 0;
	// Called every server frame
	virtual void Frame() = 0;

private:
	CBaseBot* m_bot; // The bot that this interface belongs to
};

inline IBotInterface::~IBotInterface()
{
}

#endif // !SMNAV_BOT_BASE_INTERFACE_H_
