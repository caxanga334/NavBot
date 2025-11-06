#include NAVBOT_PCH_FILE
#include <algorithm>
#include <extension.h>
#include <manager.h>
#include <bot/basebot.h>
#include "squads.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

ISquad::SquadMember::SquadMember()
{
	m_player = nullptr;
	m_bot = nullptr;
	m_squadinterface = nullptr;
	m_joinTime = -1.0f;
}

ISquad::SquadMember::SquadMember(CBaseBot* bot) :
	SquadMember()
{
	Init(nullptr, bot);
}

ISquad::SquadMember::SquadMember(CBaseEntity* player) :
	SquadMember()
{
	Init(player, nullptr);
}

float ISquad::SquadMember::GetTimeSinceMember() const
{
	return gpGlobals->curtime - m_joinTime;
}

Vector ISquad::SquadMember::GetPosition() const
{
	return UtilHelpers::getEntityOrigin(m_player.Get());
}

void ISquad::SquadMember::Init(CBaseEntity* pPlayer, CBaseBot* bot)
{
	if (pPlayer)
	{
		m_player = pPlayer;
		m_bot = nullptr;
	}
	else
	{
		m_bot = bot;
		m_squadinterface = bot->GetSquadInterface();
		m_player = bot->GetEntity();
	}

	m_joinTime = gpGlobals->curtime;
}

ISquad::SquadData::SquadData() :
	m_botleader(), m_humanleader()
{
	m_destroyed = false;
	m_humanled = false;
	m_createdTime = gpGlobals->curtime;
	m_teamNum = 0;
}

ISquad::SquadData::~SquadData()
{
	if (m_teamNum >= 0 && m_teamNum <= MAX_TEAMS)
	{
		ISquad::DecrementSquadCount(m_teamNum);
	}
}

Vector ISquad::SquadData::GetSquadLeaderPosition() const
{
	if (m_humanled && m_humanleader.IsValid())
	{
		return m_humanleader.GetPosition();
	}

	if (m_botleader.IsValid())
	{
		return m_botleader.GetPosition();
	}

	return vec3_origin;
}

bool ISquad::SquadData::IsSquadLeaderAlive() const
{
	CBaseEntity* human = m_humanleader.GetPlayerEntity();

	if (human)
	{
		return UtilHelpers::IsPlayerAlive(m_humanleader.GetPlayerIndex());
	}

	if (m_botleader.IsValid())
	{
		return m_botleader.GetBot()->IsAlive();
	}

	return false;
}

float ISquad::SquadData::GetTimeSinceCreation() const
{
	return m_createdTime - gpGlobals->curtime;
}

std::size_t ISquad::SquadData::GetSquadSize() const
{
	std::size_t size = 1U;

	if (m_humanled)
	{
		size++;
	}

	return size + m_members.size();
}

void ISquad::SquadData::RemoveInvalidMembers()
{
	m_members.erase(std::remove_if(m_members.begin(), m_members.end(), [](const SquadMember& member) {
		return !member.IsValid();
	}), m_members.end());
}

void ISquad::SquadData::DoFullValidation(ISensor* sensor)
{
	if (m_humanled)
	{
		if (!m_humanleader.IsValid())
		{
			Destroy();
			return;
		}

		if (!sensor->IsFriendly(m_humanleader.GetPlayerEntity()))
		{
			Destroy();
			return;
		}
	}

	m_members.erase(std::remove_if(m_members.begin(), m_members.end(), [sensor](const SquadMember& member) {
		if (!member.IsValid()) { return true; }

		if (!sensor->IsFriendly(member.GetPlayerEntity()))
		{
			return true;
		}

		if (member.IsBot())
		{
			if (!member.GetSquadInterface()->IsInASquad())
			{
				return true;
			}
		}

		return false;
	}), m_members.end());
}

void ISquad::SquadData::Destroy()
{
	for (SquadMember& member : m_members)
	{
		if (member.IsValid() && member.IsBot())
		{
			member.GetSquadInterface()->NotifySquadDestruction();
		}
	}

	m_destroyed = true;
	m_members.clear();

	if (m_botleader.IsValid())
	{
		m_botleader.GetSquadInterface()->NotifySquadDestruction();
	}
}

void ISquad::SquadData::AddMember(CBaseBot* bot)
{
	auto it = std::find_if(m_members.begin(), m_members.end(), [&bot](const ISquad::SquadMember& member) {
		return member.GetBot() == bot;
	});

	if (it != m_members.end())
	{
		return;
	}

	m_members.emplace_back(bot);
}

void ISquad::SquadData::RemoveMember(CBaseBot* bot)
{
	m_members.erase(std::remove_if(m_members.begin(), m_members.end(), [&bot](const SquadMember& member) {
		return member.GetBot() == bot;
	}), m_members.end());
}

void ISquad::SquadData::Init()
{
	if (m_teamNum >= 0 && m_teamNum < MAX_TEAMS)
	{
		ISquad::IncrementSquadCount(m_teamNum);
	}
}

ISquad::ISquad(CBaseBot* bot) :
	IBotInterface(bot)
{
	m_squaddata = nullptr;
}

ISquad::~ISquad()
{
	if (m_squaddata)
	{
		if (IsSquadLeader())
		{
			m_squaddata->Destroy();
		}
		else
		{
			const ISquad::SquadMember* leader = m_squaddata->GetBotLeader();

			if (leader && leader->IsValid())
			{
				leader->GetSquadInterface()->NotifyMemberLeftSquad(GetBot<CBaseBot>(), true);
			}
		}
	}
}

void ISquad::Reset()
{
	m_fullValidationTimer.Invalidate();
	m_shareInfoTimer.Invalidate();

	if (m_squaddata)
	{
		if (IsSquadLeader())
		{
			m_squaddata->Destroy();
		}
		else
		{
			if (IsSquadValid())
			{
				m_squaddata->GetBotLeader()->GetSquadInterface()->NotifyMemberLeftSquad(GetBot<CBaseBot>(), true);
			}
		}

		m_squaddata = nullptr;
	}
}

void ISquad::Update()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISquad::Update", "NavBot");
#endif // EXT_VPROF_ENABLED
	
	CBaseBot* me = GetBot<CBaseBot>();

	if (IsInASquad())
	{
		if (!m_squaddata->IsValid())
		{
			NotifySquadDestruction();
		}
		else
		{
			if (IsSquadLeader())
			{
				m_squaddata->RemoveInvalidMembers();

				if (m_squaddata->IsHumanLedSquad() && !m_squaddata->IsSquadLeaderAlive())
				{
					LeaveSquad();
				}

				if (m_fullValidationTimer.IsElapsed())
				{
					m_fullValidationTimer.Start(0.5f);
					// Removes any members that doesn't match the bot's current team
					m_squaddata->DoFullValidation(me->GetSensorInterface());
				}

				// Squad leader, share known entity list with the rest of the squad
				if (m_shareInfoTimer.IsElapsed())
				{
					m_shareInfoTimer.StartRandom(3.0f, 9.0f);

					ISensor* mysensor = me->GetSensorInterface();

					for (const ISquad::SquadMember& member : m_squaddata->GetMembers())
					{
						if (member.IsValid() && member.IsBot())
						{
							member.GetBot()->GetSensorInterface()->ShareKnownEntityList(mysensor);
						}
					}
				}
			}
			else
			{
				// Squad member, share the known entity list with my squad leader
				if (m_shareInfoTimer.IsElapsed())
				{
					m_shareInfoTimer.StartRandom(3.0f, 9.0f);

					m_squaddata->GetBotLeader()->GetBot()->GetSensorInterface()->ShareKnownEntityList(me->GetSensorInterface());
				}
			}
		}
	}
}

bool ISquad::OnInvitedToJoinSquad(CBaseBot* leader, const SquadData* squaddata) const
{
	CBaseBot* me = GetBot<CBaseBot>();

	if (me == leader) { return false; }

	CBaseEntity* leaderEnt = leader->GetEntity();

	return me->GetSensorInterface()->IsFriendly(leaderEnt);
}

void ISquad::CreateSquad(CBaseEntity* humanLeader)
{
	if (!CanLeadSquads()) { return; }

	if (IsInASquad()) { return; }

	m_squaddata.reset(AllocateSquadData());
	CBaseBot* me = GetBot<CBaseBot>();

	m_squaddata->m_botleader.Init(nullptr, me);
	m_squaddata->m_teamNum = me->GetCurrentTeamIndex();
	m_squaddata->Init();

	if (humanLeader)
	{
		m_squaddata->m_humanleader.Init(humanLeader, nullptr);
		m_squaddata->m_humanled = true;
	}

	me->OnSquadEvent(IEventListener::SquadEventType::SQUAD_EVENT_FORMED);

	if (me->IsDebugging(BOTDEBUG_SQUADS))
	{
		me->DebugPrintToConsole(136, 231, 136, "%s SQUAD CREATED!\n", me->GetDebugIdentifier());
	}
}

void ISquad::AddToSquad(CBaseBot* member)
{
	if (IsSquadLeader())
	{
		m_squaddata->AddMember(member);

		ISquad* other = member->GetSquadInterface();
		other->m_squaddata = m_squaddata; // share a copy of my squad data with my member
		member->OnSquadEvent(IEventListener::SquadEventType::SQUAD_EVENT_JOINED);

		CBaseBot* me = GetBot<CBaseBot>();

		if (me->IsDebugging(BOTDEBUG_SQUADS))
		{
			me->DebugPrintToConsole(0, 180, 0, "%s BOT \"%s\" WAS ADDED TO MY SQUAD!\n", me->GetDebugIdentifier(), member->GetClientName());
		}
	}
}

bool ISquad::TryToAddToSquad(CBaseBot* member)
{
	ISquad* other = member->GetSquadInterface();

	if (other->CanJoinSquads() && other->OnInvitedToJoinSquad(GetBot<CBaseBot>(), m_squaddata.get()))
	{
		this->AddToSquad(member);
		return true;
	}

	return false;
}

void ISquad::LeaveSquad()
{
	if (!IsInASquad()) { return; }

	if (IsSquadLeader())
	{
		m_squaddata->Destroy();
		NotifySquadDestruction();
		return;
	}

	const ISquad::SquadMember* leader = m_squaddata->GetBotLeader();

	if (leader && leader->IsValid())
	{
		leader->GetSquadInterface()->NotifyMemberLeftSquad(GetBot<CBaseBot>());
	}

	CBaseBot* me = GetBot<CBaseBot>();
	if (me->IsDebugging(BOTDEBUG_SQUADS))
	{
		me->DebugPrintToConsole(255, 165, 0, "%s LEFT SQUAD!\n", me->GetDebugIdentifier());
	}
}

void ISquad::NotifySquadDestruction()
{
	if (m_squaddata)
	{
		CBaseBot* me = GetBot<CBaseBot>();
		me->OnSquadEvent(IEventListener::SquadEventType::SQUAD_EVENT_DISBANDED);
		m_squaddata = nullptr;

		if (me->IsDebugging(BOTDEBUG_SQUADS))
		{
			me->DebugPrintToConsole(255, 165, 0, "%s SQUAD DESTROYED!\n", me->GetDebugIdentifier());
		}
	}
}

void ISquad::NotifyMemberLeftSquad(CBaseBot* member, const bool is_deleting)
{
	if (!IsInASquad()) { return; }

	if (IsSquadLeader())
	{
		CBaseBot* me = GetBot<CBaseBot>();
		me->OnSquadEvent(IEventListener::SquadEventType::SQUAD_EVENT_LEFT);
		
		if (!is_deleting)
		{
			member->OnSquadEvent(IEventListener::SquadEventType::SQUAD_EVENT_REMOVED_FROM);
		}

		m_squaddata->RemoveMember(member);

		if (me->IsDebugging(BOTDEBUG_SQUADS))
		{
			me->DebugPrintToConsole(255, 165, 0, "%s MEMBER REMOVED FROM SQUAD!\n", me->GetDebugIdentifier());
		}
	}
}

ISquad* ISquad::GetSquadInterfaceOfHumanLeader(CBaseEntity* humanLeader)
{
	ISquad* iface = nullptr;

	auto func = [&iface, humanLeader](CBaseBot* bot) {

		ISquad* squad = bot->GetSquadInterface();

		if (squad->IsSquadValid() && squad->IsSquadLeader() && squad->IsSquadLedByAHuman())
		{
			const ISquad::SquadMember* leader = squad->GetSquad()->GetSquadLeader();

			if (leader->IsHuman() && leader->GetPlayerEntity() == humanLeader)
			{
				iface = squad;
			}
		}
	};

	return iface;
}

void ISquad::DestroyAllSquadsFunc::operator()(CBaseBot* bot)
{
	ISquad* squad = bot->GetSquadInterface();

	if (squad->IsInASquad() && squad->IsSquadLeader())
	{
		squad->LeaveSquad();
	}
}

void ISquad::DestroyAllSquads()
{
	DestroyAllSquadsFunc func;
	extmanager->ForEachBot(func);
}

void ISquad::InviteBotsToSquadFunc::operator()(CBaseBot* bot)
{
	if (bot == m_me) { return; }

	if (m_me->GetSquadInterface()->GetSquad()->GetMembersCount() >= static_cast<size_t>(m_max)) { return; }

	if (bot->IsAlive() && bot->GetCurrentTeamIndex() == m_me->GetCurrentTeamIndex() && !bot->GetSquadInterface()->IsInASquad())
	{
		m_me->GetSquadInterface()->TryToAddToSquad(bot);
	}
}
