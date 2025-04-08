#ifndef SMNAV_BOT_EVENT_LISTENER_H_
#define SMNAV_BOT_EVENT_LISTENER_H_
#pragma once

#include <vector>

class CBaseBot;
class CPath;
class CBaseEntity;
class Vector;
class CTakeDamageInfo;
struct edict_t;
class CNavArea;

// Interface for receiving events
class IEventListener
{
public:

	virtual ~IEventListener() {}

	enum MovementFailureType
	{
		FAIL_NO_PATH = 0, // no path exists
		FAIL_STUCK, // bot got stuck
		FAIL_FELL_OFF_PATH // bot fell off the path
	};

	enum SoundType
	{
		SOUND_INVALID = -1,
		SOUND_GENERIC = 0, // generic sounds
		SOUND_WEAPON, // weapon sound (except firing)
		SOUND_GUNFIRE, // gun fire
		SOUND_PLAYER, // player sounds
		SOUND_FOOTSTEP, // NPC/player footstep
		SOUND_VOICE, // NPC/player voice
		SOUND_WORLD, // world emitted sounds
		SOUND_DOOR, // door opening/closing
		SOUND_TURRET, // turret/sentry gun sound
		SOUND_DANGER, // something dangerous

		MAX_SOUND_TYPES
	};

	// Gets a vector containing all event listeners
	virtual std::vector<IEventListener*>* GetListenerVector() { return nullptr; }

	virtual void OnDebugMoveToHostCommand();
	virtual void OnNavAreaChanged(CNavArea* oldArea, CNavArea* newArea);
	virtual void OnStuck(); // bot is stuck
	virtual void OnUnstuck(); // bot was stuck and is no longer stuck
	virtual void OnMoveToFailure(CPath* path, MovementFailureType reason);
	virtual void OnMoveToSuccess(CPath* path);
	virtual void OnContact(CBaseEntity* pOther); // Something touched the bot
	virtual void OnIgnited(const CTakeDamageInfo& info); // The bot is on fire and/or taking fire damage
	virtual void OnInjured(const CTakeDamageInfo& info); // when the bot takes damage
	virtual void OnKilled(const CTakeDamageInfo& info); // when the bot is killed
	virtual void OnOtherKilled(CBaseEntity* pVictim, const CTakeDamageInfo& info); // when another player gets killed
	virtual void OnSight(CBaseEntity* subject); // when the bot spots an entity
	virtual void OnLostSight(CBaseEntity* subject); // when the bot loses sight of an entity
	virtual void OnSound(CBaseEntity* source, const Vector& position, SoundType type, const float maxRadius); // when the bot hears an entity
	virtual void OnRoundStateChanged(); // When the round state changes (IE: round start,end, freeze time end, setup time end, etc...)
	virtual void OnFlagTaken(CBaseEntity* player); // CTF: Flag was stolen
	virtual void OnFlagDropped(CBaseEntity* player); // CTF: Flag was dropped
	virtual void OnControlPointCaptured(CBaseEntity* point); // When a control point is captured
	virtual void OnControlPointLost(CBaseEntity* point); // When a control point is lost
	virtual void OnControlPointContested(CBaseEntity* point); // When a control point is under siege
	virtual void OnWeaponEquip(CBaseEntity* weapon); // When a weapon is equipped (invoked by a CBasePlayer::Weapon_Equip post hook)
	virtual void OnVoiceCommand(CBaseEntity* subject, int command); // When a player uses voice commands (mod specific)
};

inline void IEventListener::OnDebugMoveToHostCommand()
{
	auto vec = GetListenerVector();

	if (vec)
	{
		for (auto listener : *vec)
		{
			listener->OnDebugMoveToHostCommand();
		}
	}
}

inline void IEventListener::OnNavAreaChanged(CNavArea* oldArea, CNavArea* newArea)
{
	auto vec = GetListenerVector();

	if (vec)
	{
		for (auto listener : *vec)
		{
			listener->OnNavAreaChanged(oldArea, newArea);
		}
	}
}

inline void IEventListener::OnStuck()
{
	auto vec = GetListenerVector();

	if (vec)
	{
		for (auto listener : *vec)
		{
			listener->OnStuck();
		}
	}
}

inline void IEventListener::OnUnstuck()
{
	auto vec = GetListenerVector();

	if (vec)
	{
		for (auto listener : *vec)
		{
			listener->OnUnstuck();
		}
	}
}

inline void IEventListener::OnMoveToFailure(CPath* path, MovementFailureType reason)
{
	auto vec = GetListenerVector();

	if (vec)
	{
		for (auto listener : *vec)
		{
			listener->OnMoveToFailure(path, reason);
		}
	}
}

inline void IEventListener::OnMoveToSuccess(CPath* path)
{
	auto vec = GetListenerVector();

	if (vec)
	{
		for (auto listener : *vec)
		{
			listener->OnMoveToSuccess(path);
		}
	}
}

inline void IEventListener::OnContact(CBaseEntity* pOther)
{
	auto vec = GetListenerVector();

	if (vec)
	{
		for (auto listener : *vec)
		{
			listener->OnContact(pOther);
		}
	}
}

inline void IEventListener::OnIgnited(const CTakeDamageInfo& info)
{
	auto vec = GetListenerVector();

	if (vec)
	{
		for (auto listener : *vec)
		{
			listener->OnIgnited(info);
		}
	}
}

inline void IEventListener::OnInjured(const CTakeDamageInfo& info)
{
	auto vec = GetListenerVector();

	if (vec)
	{
		for (auto listener : *vec)
		{
			listener->OnInjured(info);
		}
	}
}

inline void IEventListener::OnKilled(const CTakeDamageInfo& info)
{
	auto vec = GetListenerVector();

	if (vec)
	{
		for (auto listener : *vec)
		{
			listener->OnKilled(info);
		}
	}
}

inline void IEventListener::OnOtherKilled(CBaseEntity* pVictim, const CTakeDamageInfo& info)
{
	auto vec = GetListenerVector();

	if (vec)
	{
		for (auto listener : *vec)
		{
			listener->OnOtherKilled(pVictim, info);
		}
	}
}

inline void IEventListener::OnSight(CBaseEntity* subject)
{
	auto vec = GetListenerVector();

	if (vec)
	{
		for (auto listener : *vec)
		{
			listener->OnSight(subject);
		}
	}
}

inline void IEventListener::OnLostSight(CBaseEntity* subject)
{
	auto vec = GetListenerVector();

	if (vec)
	{
		for (auto listener : *vec)
		{
			listener->OnLostSight(subject);
		}
	}
}

inline void IEventListener::OnSound(CBaseEntity* source, const Vector& position, SoundType type, const float maxRadius)
{
	auto vec = GetListenerVector();

	if (vec)
	{
		for (auto listener : *vec)
		{
			listener->OnSound(source, position, type, maxRadius);
		}
	}
}

inline void IEventListener::OnRoundStateChanged()
{
	auto vec = GetListenerVector();

	if (vec)
	{
		for (auto listener : *vec)
		{
			listener->OnRoundStateChanged();
		}
	}
}

inline void IEventListener::OnFlagTaken(CBaseEntity* player)
{
	auto vec = GetListenerVector();

	if (vec)
	{
		for (auto listener : *vec)
		{
			listener->OnFlagTaken(player);
		}
	}
}

inline void IEventListener::OnFlagDropped(CBaseEntity* player)
{
	auto vec = GetListenerVector();

	if (vec)
	{
		for (auto listener : *vec)
		{
			listener->OnFlagDropped(player);
		}
	}
}

inline void IEventListener::OnControlPointCaptured(CBaseEntity* point)
{
	auto vec = GetListenerVector();

	if (vec)
	{
		for (auto listener : *vec)
		{
			listener->OnControlPointCaptured(point);
		}
	}
}

inline void IEventListener::OnControlPointLost(CBaseEntity* point)
{
	auto vec = GetListenerVector();

	if (vec)
	{
		for (auto listener : *vec)
		{
			listener->OnControlPointLost(point);
		}
	}
}

inline void IEventListener::OnControlPointContested(CBaseEntity* point)
{
	auto vec = GetListenerVector();

	if (vec)
	{
		for (auto listener : *vec)
		{
			listener->OnControlPointContested(point);
		}
	}
}

inline void IEventListener::OnWeaponEquip(CBaseEntity* weapon)
{
	auto vec = GetListenerVector();

	if (vec)
	{
		for (auto listener : *vec)
		{
			listener->OnWeaponEquip(weapon);
		}
	}
}

inline void IEventListener::OnVoiceCommand(CBaseEntity* subject, int command)
{
	auto vec = GetListenerVector();

	if (vec)
	{
		for (auto listener : *vec)
		{
			listener->OnVoiceCommand(subject, command);
		}
	}
}


#endif // !SMNAV_BOT_EVENT_LISTENER_H_

