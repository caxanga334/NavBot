#ifndef EXT_BASE_BOT_H_
#define EXT_BASE_BOT_H_

#include <list>

#include <extplayer.h>
#include <bot/interfaces/playercontrol.h>
#include <bot/interfaces/movement.h>
#include <bot/interfaces/sensor.h>

// Interval between calls to Update()
constexpr auto BOT_UPDATE_INTERVAL = 0.12f;

// TO-DO: Add a convar to control update interval

class IBotController;
class IBotInterface;

class CBaseBot : public CBaseExtPlayer
{
public:
	CBaseBot(edict_t* edict);
	virtual ~CBaseBot();

	// Called by the manager for all players every server frame.
	// Overriden to call bot functions
	virtual void PlayerThink() override final;

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

	IBotController* GetController() const { return m_controller; }

	void RegisterInterface(IBotInterface* iface);
	void BuildUserCommand(const int buttons);
	inline CBotCmd* GetUserCommand() { return &m_cmd; }
	inline void SetViewAngles(QAngle& angle) { m_viewangles = angle; }
	virtual IPlayerController* GetControlInterface();
	virtual IMovement* GetMovementInterface();
	virtual ISensor* GetSensorInterface();

	inline const std::list<IBotInterface*>& GetRegisteredInterfaces() const { return m_interfaces; }
private:
	int m_nextupdatetime;
	IBotController* m_controller;
	std::list<IBotInterface*> m_interfaces;
	CBotCmd m_cmd; // User command to send
	QAngle m_viewangles; // The bot eye angles
	int m_weaponselect;
	IPlayerController* m_basecontrol; // Base controller interface
	IMovement* m_basemover; // Base movement interface
	ISensor* m_basesensor; // Base vision and hearing interface
};

#endif // !EXT_BASE_BOT_H_
