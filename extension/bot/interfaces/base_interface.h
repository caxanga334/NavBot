#ifndef SMNAV_BOT_BASE_INTERFACE_H_
#define SMNAV_BOT_BASE_INTERFACE_H_
#pragma once

class CBaseBot;

/**
 * @brief Base class for bot interfaces. A system that allows to expand the bot capabilities
*/
class IBotInterface
{
public:
	IBotInterface(CBaseBot* bot);
	virtual ~IBotInterface();

	inline virtual CBaseBot* GetBot() { return m_bot; }

	// Reset the interface to it's initial state
	virtual void Reset() = 0;
	// Called at intervals
	virtual void Update() = 0;
	// Called every server frame
	virtual void Frame() = 0;

	void SetNext(IBotInterface* next) { m_next = next; }
	IBotInterface* GetNext() const { return m_next; }
private:
	CBaseBot* m_bot; // The bot that this interface belongs to
	IBotInterface* m_next; // Next interface in the linked list
};

inline IBotInterface::~IBotInterface()
{
	m_next = nullptr;
}

#endif // !SMNAV_BOT_BASE_INTERFACE_H_
