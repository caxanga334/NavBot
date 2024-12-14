#include <algorithm>

#include <extension.h>
#include <entities/tf2/tf_entities.h>
#include <util/helpers.h>
#include <mods/tf2/tf2lib.h>
#include "tf2bot.h"
#include "tf2bot_spymonitor.h"

CTF2BotSpyMonitor::CTF2BotSpyMonitor(CBaseBot* bot) :
	IBotInterface(bot)
{
	m_knownspylist.reserve(100);
}

CTF2BotSpyMonitor::~CTF2BotSpyMonitor()
{
}

void CTF2BotSpyMonitor::Reset()
{
	m_knownspylist.clear();
}

void CTF2BotSpyMonitor::Update()
{
	CTF2Bot* me = static_cast<CTF2Bot*>(GetBot());
	CTF2BotSensor* vision = me->GetSensorInterface();
	const CKnownEntity* known = nullptr;

	// Remove obsolete spies from the list
	m_knownspylist.erase(std::remove_if(m_knownspylist.begin(), m_knownspylist.end(), [](const KnownSpy& object) {
		return object.IsObsolete();
	}), m_knownspylist.end());

	for (auto& knownspy : m_knownspylist)
	{
		knownspy.Update(me);
	}
}

void CTF2BotSpyMonitor::Frame()
{
}

const CTF2BotSpyMonitor::KnownSpy& CTF2BotSpyMonitor::GetKnownSpy(edict_t* spy)
{
	for (auto& known : m_knownspylist)
	{
		if (known(spy))
		{
			return known;
		}
	}

	// not found
	auto& ret = m_knownspylist.emplace_back(spy);
	ret.Update(static_cast<CTF2Bot*>(GetBot())); // New spy, update it
	return ret;
}

CTF2BotSpyMonitor::KnownSpy::KnownSpy(edict_t* spy) :
	m_handle(spy)
{
	m_detectionLevel = DETECTION_NEW;
	m_initialDetectionTime.Start();
}

CTF2BotSpyMonitor::KnownSpy::KnownSpy(int spy)
{
	m_handle.Set(gamehelpers->EdictOfIndex(spy));
	m_detectionLevel = DETECTION_NEW;
	m_initialDetectionTime.Start();
}

void CTF2BotSpyMonitor::KnownSpy::Update(CTF2Bot* me)
{
	edict_t* spy = m_handle.Get();

	if (spy == nullptr)
	{
		return;
	}

	if (!me->GetSensorInterface()->IsAbleToSee(spy))
	{
		if (m_detectionLevel == DETECTION_BLOWN && m_blownTime.IsGreaterThen(LOST_LOS_TIME))
		{
			// line of sight was lost, become suspicious
			m_detectionLevel = DETECTION_SUSPICIOUS;
		}
		else if (m_detectionLevel == DETECTION_SUSPICIOUS && m_blownTime.IsLessThen(SUSPICIOUS_TIME))
		{
			// Major loss of line of sight
			m_detectionLevel = DETECTION_NEW;
		}

		return;
	}

	// Already detected and visible, maintain detection status
	if (m_detectionLevel == DETECTION_BLOWN)
	{
		OnDetected();
		return;
	}

	int client = gamehelpers->IndexOfEdict(spy);

	// Detection: Not disguised
	if (!tf2lib::IsPlayerInCondition(client, TeamFortress2::TFCond::TFCond_Disguised))
	{
		OnDetected();
		return;
	}

	const int skill = me->GetDifficultyProfile()->GetGameAwareness();

	// Detection: Recent memory of known spy
	if (skill >= 10 && m_detectionLevel == DETECTION_SUSPICIOUS && m_blownTime.IsLessThen(SUSPICIOUS_TIME))
	{
		OnDetected();
		return;
	}

	edict_t* pTarget = tf2lib::GetDisguiseTarget(client);

	if (pTarget == nullptr)
	{
		return;
	}

	int target = gamehelpers->IndexOfEdict(pTarget);

	// Detection: Disguised as me
	if (skill >= 5 && target == me->GetIndex())
	{
		OnDetected();
		return;
	}

	// Detection: Target is dead
	if (skill >= 15 && !UtilHelpers::IsPlayerAlive(target))
	{
		OnDetected();
		return;
	}

	auto disguiseClass = tf2lib::GetDisguiseClass(client);

	// Detection: Spy disguise class doesn't match the target current class
	if (skill >= 50 && tf2lib::GetPlayerClassType(target) != disguiseClass)
	{
		OnDetected();
		return;
	}

	// Detection: Disguised as a scout while moving
	if (skill >= 25 && disguiseClass == TeamFortress2::TFClass_Scout)
	{
		tfentities::HTFBaseEntity be(spy);
		Vector velocity = be.GetAbsVelocity();
		float speed = velocity.Length();

		if (speed > 100.0f)
		{
			OnDetected();
			return;
		}
	}

	// Detection: Medics should be healing
	if (skill >= 75 && disguiseClass == TeamFortress2::TFClass_Medic)
	{
		OnDetected();
		return;
	}

	OnNoticed();
}

bool CTF2BotSpyMonitor::KnownSpy::operator==(const KnownSpy& other)
{
	if (m_handle.Get() == nullptr || other.GetEdict() == nullptr)
	{
		return false;
	}

	return gamehelpers->IndexOfEdict(m_handle.Get()) == gamehelpers->IndexOfEdict(other.GetEdict());
}

bool CTF2BotSpyMonitor::KnownSpy::operator()(edict_t* spy)
{
	if (m_handle.Get() == nullptr)
	{
		return false;
	}

	return gamehelpers->IndexOfEdict(m_handle.Get()) == gamehelpers->IndexOfEdict(spy);
}

