#ifndef EXT_BASE_BOT_H_
#define EXT_BASE_BOT_H_

#include <list>

#include <extplayer.h>
#include <bot/interfaces/playercontrol.h>
#include <bot/interfaces/movement.h>
#include <bot/interfaces/sensor.h>
#include <bot/interfaces/event_listener.h>
#include <bot/interfaces/behavior.h>
#include <bot/interfaces/profile.h>
#include <util/UtilTrace.h>

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

	virtual bool IsRangeGreaterThan(const Vector& pos, const float range) const;
	virtual bool IsRangeGreaterThan(edict_t* edict, const float range) const;
	virtual bool IsRangeLessThan(const Vector& pos, const float range) const;
	virtual bool IsRangeLessThan(edict_t* edict, const float range) const;

	virtual bool IsAbleToBreak(edict_t* entity);

	IBotController* GetController() const { return m_controller; }

	void RegisterInterface(IBotInterface* iface);
	void BuildUserCommand(const int buttons);
	void RunUserCommand(CBotCmd* ucmd);
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
	
	/**
	 * @brief Makes the bot switch weapons via user command.
	 * Weapon switch will happen when the server receives the next user command.
	 * Set to 0 to cancel a pending switch.
	 * @param weapon_entity Entity index of the weapon the bot wants to switch to
	*/
	inline void SelectWeaponByUserCmd(int weapon_entity) { m_weaponselect = weapon_entity; }
	void SelectWeaponByClassname(const char* szclassname);
	virtual void SafeWeaponSelectByClassname(const char* szclassname);
	void SelectWeaponByCommand(const char* szclassname) const;

	inline const DifficultyProfile& GetDifficultyProfile() const { return m_profile; }
	inline void SetDifficultyProfile(DifficultyProfile profile)
	{
		m_profile = profile;

		for (auto iface : m_interfaces)
		{
			iface->OnDifficultyProfileChanged();
		}
	}

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
	DifficultyProfile m_profile;
};

#endif // !EXT_BASE_BOT_H_
