#include NAVBOT_PCH_FILE
#include <algorithm>
#include <extension.h>
#include <manager.h>
#include <bot/basebot.h>
#include "squads.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

ISquad::SquadMember::SquadMember(CBaseBot* bot) :
	handle(bot->GetEntity())
{
	this->botptr = bot;
}

bool ISquad::SquadMember::operator==(const SquadMember& other) const
{
	return this->handle.Get() == other.handle.Get();
}

bool ISquad::SquadMember::operator==(CBaseBot* bot) const
{
	return this->handle.Get() == bot->GetEntity();
}

bool ISquad::SquadMember::operator==(CBaseEntity* player) const
{
	return this->handle.Get() == player;
}

void ISquad::SquadMember::AssignBot(CBaseBot* bot)
{
	handle = bot->GetEntity();
	this->botptr = bot;
}

ISquad::ISquad(CBaseBot* bot) :
	IBotInterface(bot)
{
}

ISquad::~ISquad()
{
	if (IsSquadLeader())
	{
		m_squaddata->destroyed = true;
		m_squaddata = nullptr;
	}
}

void ISquad::Reset()
{
}

void ISquad::Update()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISquad::Update", "NavBot");
#endif // EXT_VPROF_ENABLED

	CBaseBot* me = GetBot<CBaseBot>();

	if (IsInASquad())
	{
		if (GetSquadData()->destroyed || !GetSquadData()->leader.IsValid())
		{
			if (me->IsDebugging(BOTDEBUG_SQUADS))
			{
				me->DebugPrintToConsole(255, 0, 0, "%s INVALID SQUAD! \n", me->GetDebugIdentifier());
			}

			DestroySquad();
			return;
		}

		if (IsSquadLeader())
		{
			// Purge invalid members
			std::vector<SquadMember>& members = m_squaddata->members;

			members.erase(std::remove_if(members.begin(), members.end(), [](const SquadMember& object) {
				return !object.IsValid();
			}), members.end());

			const CKnownEntity* threat = me->GetSensorInterface()->GetPrimaryKnownThreat(false);

			// Pass the squad leader's primary threat to members
			if (threat)
			{
				for (auto& member : members)
				{
					if (member.IsBot())
					{
						CKnownEntity* known = member.botptr->GetSensorInterface()->AddKnownEntity(threat->GetEntity());
						known->UpdatePosition();
					}
				}
			}
		}
	}
}

void ISquad::Frame()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISquad::Frame", "NavBot");
#endif // EXT_VPROF_ENABLED
}

void ISquad::OnKilled(const CTakeDamageInfo& info)
{
	if (IsInASquad() && IsSquadLeader())
	{
		for (auto& member : GetSquadData()->members)
		{
			if (member.IsValid() && member.IsBot())
			{
				member.botptr->OnSquadEvent(IEventListener::SquadEventType::SQUAD_EVENT_LEADER_DIED);
			}
		}

		DestroySquad();
	}
}

void ISquad::OnOtherKilled(CBaseEntity* pVictim, const CTakeDamageInfo& info)
{
}

bool ISquad::IsSquadLeader() const
{
	if (m_squaddata)
	{
		return m_squaddata->leader.botptr == GetBot<CBaseBot>();
	}

	return false;
}

void ISquad::CreateSquad()
{
	m_squaddata.reset(new SquadData);
	m_squaddata->leader.AssignBot(GetBot<CBaseBot>());

	if (GetBot<CBaseBot>()->IsDebugging(BOTDEBUG_SQUADS))
	{
		GetBot<CBaseBot>()->DebugPrintToConsole(0, 150, 0, "%s SQUAD CREATED! \n", GetBot<CBaseBot>()->GetDebugIdentifier());
	}

	GetBot<CBaseBot>()->OnSquadEvent(IEventListener::SquadEventType::SQUAD_EVENT_FORMED);
}

void ISquad::DestroySquad()
{
	if (!IsInASquad())
	{
		return;
	}

	if (GetBot<CBaseBot>()->IsDebugging(BOTDEBUG_SQUADS))
	{
		GetBot<CBaseBot>()->DebugPrintToConsole(255, 0, 0, "%s SQUAD DESTROYED! \n", GetBot<CBaseBot>()->GetDebugIdentifier());
	}

	GetBot<CBaseBot>()->OnSquadEvent(IEventListener::SquadEventType::SQUAD_EVENT_DISBANDED);

	if (IsSquadLeader())
	{
		// leaders notify the squad destruction

		for (auto& member : m_squaddata->members)
		{
			if (member.botptr)
			{
				member.botptr->GetSquadInterface()->DestroySquad();
			}
		}
		
		m_squaddata->destroyed = true;
		m_squaddata = nullptr;
	}
	else
	{
		m_squaddata = nullptr;
	}
}

void ISquad::AddSquadMember(CBaseBot* bot)
{
	if (IsSquadLeader())
	{
		auto& member = m_squaddata->members.emplace_back(bot);

		for (auto& members : m_squaddata->members)
		{
			if (members.IsBot())
			{
				members.botptr->GetSquadInterface()->OnNewSquadMember(&member);
			}
		}

		CopySquadData(bot->GetSquadInterface());
		bot->OnSquadEvent(IEventListener::SquadEventType::SQUAD_EVENT_JOINED);

		if (GetBot<CBaseBot>()->IsDebugging(BOTDEBUG_SQUADS))
		{
			GetBot<CBaseBot>()->DebugPrintToConsole(0, 160, 0, "%s BOT %s JOINED MY SQUAD! WE NOW HAVE %zu MEMBERS\n", 
				GetBot<CBaseBot>()->GetDebugIdentifier(), bot->GetClientName(), m_squaddata->GetSquadMemberCount());
		}
	}
#ifdef EXT_DEBUG
	else
	{
		smutils->LogError(myself, "ISquad::AddSquadMember called on a non squad leader bot!");
	}
#endif // EXT_DEBUG
}

void ISquad::JoinSquad(CBaseBot* leader)
{
	ISquad* leadersquad = leader->GetSquadInterface();

	leadersquad->AddSquadMember(GetBot<CBaseBot>());

	if (GetBot<CBaseBot>()->IsDebugging(BOTDEBUG_SQUADS))
	{
		GetBot<CBaseBot>()->DebugPrintToConsole(65, 105, 225, "%s I HAVE JOINED %s SQUAD! \n",
			GetBot<CBaseBot>()->GetDebugIdentifier(), leader->GetClientName());
	}
}

void ISquad::OnSquadMemberLeftSquad(CBaseEntity* member)
{
	if (IsSquadLeader())
	{
		if (GetBot<CBaseBot>()->IsDebugging(BOTDEBUG_SQUADS))
		{
			GetBot<CBaseBot>()->DebugPrintToConsole(255, 165, 0, "%s SQUAD MEMBER LEFT SQUAD! \n", GetBot<CBaseBot>()->GetDebugIdentifier());
		}

		std::vector<SquadMember>& members = m_squaddata->members;
		members.erase(std::remove_if(members.begin(), members.end(), [&member](const SquadMember& object) {
			return object == member;
		}), members.end());
	}
}

void ISquad::NotifyVisibleEnemy(CBaseEntity* enemy) const
{
	if (!IsInASquad())
	{
		return;
	}

	if (!IsSquadLeader())
	{
		if (m_squaddata->leader.IsValid() && m_squaddata->leader.IsBot())
		{
			CKnownEntity* known = m_squaddata->leader.botptr->GetSensorInterface()->AddKnownEntity(enemy);
			known->UpdatePosition();
		}
	}

	CBaseBot* me = GetBot<CBaseBot>();

	for (auto& member : m_squaddata->members)
	{
		if (member.IsValid() && member.IsBot() && member.botptr != me)
		{
			CKnownEntity* known = member.botptr->GetSensorInterface()->AddKnownEntity(enemy);
			known->UpdatePosition();
		}
	}
}
