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

// Interface for receiving events
class IEventListener
{
public:

	enum MovementFailureType
	{
		FAIL_NO_PATH = 0, // no path exists
		FAIL_STUCK, // bot got stuck
		FAIL_FELL_OFF_PATH // bot fell off the path
	};

	enum SoundType
	{
		SOUND_GENERIC = 0,
		SOUND_WEAPON,
		SOUND_PLAYER,
		SOUND_WORLD,

		MAX_SOUND_TYPES
	};

	// Gets a vector containing all event listeners
	virtual std::vector<IEventListener*>* GetListenerVector() { return nullptr; }

	virtual void OnTestEventPropagation();
	virtual void OnStuck(); // bot is stuck
	virtual void OnUnstuck(); // bot was stuck and is no longer stuck
	virtual void OnMoveToFailure(CPath* path, MovementFailureType reason);
	virtual void OnMoveToSuccess(CPath* path);
	virtual void OnContact(CBaseEntity* pOther); // Something touched the bot
	virtual void OnIgnited(const CTakeDamageInfo& info); // The bot is on fire and/or taking fire damage
	virtual void OnInjured(const CTakeDamageInfo& info); // when the bot takes damage
	virtual void OnKilled(const CTakeDamageInfo& info); // when the bot is killed
	virtual void OnOtherKilled(CBaseEntity* pVictim, const CTakeDamageInfo& info); // when another player gets killed
	virtual void OnSight(edict_t* subject); // when the bot spots an entity
	virtual void OnLostSight(edict_t* subject); // when the bot loses sight of an entity
	virtual void OnSound(edict_t* source, const Vector& position, SoundType type, const int volume); // when the bot hears an entity
};

inline void IEventListener::OnTestEventPropagation()
{
	auto vec = GetListenerVector();

	if (vec)
	{
		for (auto listener : *vec)
		{
			listener->OnTestEventPropagation();
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

inline void IEventListener::OnSight(edict_t* subject)
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

inline void IEventListener::OnLostSight(edict_t* subject)
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

inline void IEventListener::OnSound(edict_t* source, const Vector& position, SoundType type, const int volume)
{
	auto vec = GetListenerVector();

	if (vec)
	{
		for (auto listener : *vec)
		{
			listener->OnSound(source, position, type, volume);
		}
	}
}


#endif // !SMNAV_BOT_EVENT_LISTENER_H_

