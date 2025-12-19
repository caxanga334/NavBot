#ifndef __NAVBOT_BOT_PATH_PROCESSOR_INTERFACE_H_
#define __NAVBOT_BOT_PATH_PROCESSOR_INTERFACE_H_

#include "base_interface.h"

class CMeshNavigator;
class BotPathSegment;

/**
 * @brief Path Processor. Interface responsible for processing and storing information about the bot's path.
 */
class IPathProcessor : public IBotInterface
{
public:
	IPathProcessor(CBaseBot* bot);
	
	void Reset() override;
	void Update() override;
	void Frame() override;
	// Returns true if the path status has changed and behavior should be notified
	bool ShouldFireEvents() const { return m_fireEvent; }

protected:
	// Sets if the event should be fired
	void SetFireEvent(bool state) { m_fireEvent = state; }

private:
	bool m_fireEvent; // true if the path status has changed, this is used to known when to fire events to the behavior system

};

#endif // !__NAVBOT_BOT_PATH_PROCESSOR_INTERFACE_H_
