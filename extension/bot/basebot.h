#ifndef EXT_BASE_BOT_H_
#define EXT_BASE_BOT_H_

#include <list>

#include <extplayer.h>
#include <bot/interfaces/playercontrol.h>
#include <bot/interfaces/movement.h>
#include <bot/interfaces/sensor.h>
#include <bot/interfaces/event_listener.h>
#include <bot/interfaces/behavior.h>

// Interval between calls to Update()
constexpr auto BOT_UPDATE_INTERVAL = 0.12f;

// TO-DO: Add a convar to control update interval

class IBotController;
class IBotInterface;

class CBaseBot : public CBaseExtPlayer, public IEventListener
{
public:
	CBaseBot(edict_t* edict);
	virtual ~CBaseBot();

	// Event propagation
	virtual std::vector<IEventListener*>* GetListenerVector() override;

	// Called by the manager for all players every server frame.
	// Overriden to call bot functions
	virtual void PlayerThink() override final;

	// true if this is a bot managed by this extension
	virtual bool IsExtensionBot() const override { return true; }
	// Pointer to the extension bot class
	virtual CBaseBot* MyBotPointer() override { return this; }

	// Reset the bot to it's initial state
	virtual void Reset();
	// Function called at intervals to run the AI 
	virtual void Update();
	// Function called every server frame to run the AI
	virtual void Frame();

	virtual float GetRangeTo(const Vector& pos) const;
	virtual float GetRangeTo(edict_t* edict) const;
	virtual float GetRangeToSqr(const Vector& pos) const;
	virtual float GetRangetToSqr(edict_t* edict) const;

	IBotController* GetController() const { return m_controller; }

	void RegisterInterface(IBotInterface* iface);
	void BuildUserCommand(const int buttons);
	inline CBotCmd* GetUserCommand() { return &m_cmd; }
	// Sets the view angles to be sent on the next User Command
	inline void SetViewAngles(QAngle& angle) { m_viewangles = angle; }
	virtual IPlayerController* GetControlInterface();
	virtual IMovement* GetMovementInterface();
	virtual ISensor* GetSensorInterface();
	virtual IBehavior* GetBehaviorInterface();

	inline const std::list<IBotInterface*>& GetRegisteredInterfaces() const { return m_interfaces; }

	virtual void Spawn();
	virtual void FirstSpawn();

protected:
	bool m_isfirstspawn;

private:
	
	int m_nextupdatetime;
	IBotController* m_controller;
	std::list<IBotInterface*> m_interfaces;
	std::vector<IEventListener*> m_listeners; // Event listeners vector
	CBotCmd m_cmd; // User command to send
	QAngle m_viewangles; // The bot eye angles
	int m_weaponselect;
	IPlayerController* m_basecontrol; // Base controller interface
	IMovement* m_basemover; // Base movement interface
	ISensor* m_basesensor; // Base vision and hearing interface
	IBehavior* m_basebehavior; // Base AI Behavior interface
};

#endif // !EXT_BASE_BOT_H_
