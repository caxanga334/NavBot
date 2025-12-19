#ifndef __NAVBOT_DODS_BOT_PATH_PROCESSOR_INTERFACE_H_
#define __NAVBOT_DODS_BOT_PATH_PROCESSOR_INTERFACE_H_

#include <bot/interfaces/pathprocessor.h>

class CDoDSBot;

class CDoDSBotPathProcessor : public IPathProcessor
{
public:
	CDoDSBotPathProcessor(CDoDSBot* bot);

	void Reset() override;
	void Update() override;
	// Nav area is blocked because it needs a bomb to open an obstacle
	void OnAreaBlockedByBomb(const CNavArea* area);
	bool NeedsBombToClearPathObstacle() const { return m_bombArea != nullptr; }
	const CNavArea* GetBombArea() const { return m_bombArea; }
	void ClearBombArea() { m_bombArea = nullptr; }

private:
	const CNavArea* m_bombArea;
	CountdownTimer m_delayedUpdate;
};


#endif // !__NAVBOT_DODS_BOT_PATH_PROCESSOR_INTERFACE_H_
