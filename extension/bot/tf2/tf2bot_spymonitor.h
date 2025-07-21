#ifndef NAVBOT_TF2BOT_SPY_MONITOR_INTERFACE_H_
#define NAVBOT_TF2BOT_SPY_MONITOR_INTERFACE_H_
#pragma once

#include <vector>
#include <util/ehandle_edict.h>
#include <sdkports/sdk_timers.h>
#include <bot/interfaces/base_interface.h>

class CTF2Bot;

class CTF2BotSpyMonitor : public IBotInterface
{
public:
	CTF2BotSpyMonitor(CBaseBot* bot);
	~CTF2BotSpyMonitor() override;

	enum SpyDetectionLevel
	{
		DETECTION_INVALID = 0,
		DETECTION_NEW, // A new spy has been spotted
		DETECTION_FOOLED, // The spy has fooled me
		DETECTION_SUSPICIOUS, // The spy is acting suspicious, monitor them
		DETECTION_BLOWN, // Enemy spy has been detected, attack!

		MAX_DETECTION_TYPES
	};

	/**
	 * @brief Represents the data the bot knowns about a spy
	 */
	class KnownSpy
	{
	public:
		KnownSpy(edict_t* spy);
		KnownSpy(CBaseEntity* spy);
		KnownSpy(int spy);

		static constexpr auto FORGET_SPY_TIME = 25.0f;
		static constexpr auto SUSPICIOUS_TIME = 10.0f;
		static constexpr auto LOST_LOS_TIME = 2.5f;

		void Update(CTF2Bot* me);

		bool operator==(const KnownSpy& other);
		bool operator()(edict_t* spy);
		bool operator()(CBaseEntity* spy);
		edict_t* GetEdict() const { return m_handle.ToEdict(); }
		bool IsObsolete() const { return m_handle.Get() == nullptr || m_detectionLevel == DETECTION_INVALID || (m_forgetTime.HasStarted() && m_forgetTime.IsElapsed()); }
		void Invalidate() { m_handle.Term(); }
		bool IsHostile() const { return m_detectionLevel == DETECTION_BLOWN; }
		SpyDetectionLevel GetDetectionLevel() const { return m_detectionLevel; }
		void NotifyTouch() { OnDetected(); }
		void ForceDetection() { OnDetected(); }

	private:
		CHandle<CBaseEntity> m_handle;
		SpyDetectionLevel m_detectionLevel;
		IntervalTimer m_initialDetectionTime; // Time since the spy has been added to the known database
		IntervalTimer m_blownTime; // Time since the spy has been detected as an enemy spy
		CountdownTimer m_forgetTime; // Time to forget about this spy

		void OnDetected() { m_detectionLevel = DETECTION_BLOWN; m_forgetTime.Start(FORGET_SPY_TIME); m_blownTime.Start(); }
		void OnNoticed() { m_forgetTime.Start(FORGET_SPY_TIME); }
	};

	// Reset the interface to it's initial state
	void Reset() override;
	// Called at intervals
	void Update() override;
	// Called every server frame
	void Frame() override;

	const KnownSpy& GetKnownSpy(edict_t* spy);
	const KnownSpy& GetKnownSpy(CBaseEntity* spy);
	// Immediately recognize and detect a spy
	const KnownSpy* DetectSpy(CBaseEntity* spy);

private:
	std::vector<KnownSpy> m_knownspylist;
};

#endif // !NAVBOT_TF2BOT_SPY_MONITOR_INTERFACE_H_
