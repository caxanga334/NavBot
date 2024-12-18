#include <algorithm>
#include <extension.h>
#include <manager.h>
#include <bot/basebot.h>
#include "squads.h"

ISquad::SquadMember::SquadMember(CBaseBot* follower)
{
	this->entity = follower->GetEntity();
	this->bot = follower;
}

ISquad* ISquad::GetSquadLeaderInterfaceForHumanLeader(CBaseEntity* humanleader)
{
	ISquad* iface = nullptr;

	extmanager->ForEachBot([&humanleader, &iface](CBaseBot* bot) {
		// stop on the first one found
		if (iface != nullptr) { return; }

		ISquad* squad = bot->GetSquadInterface();

		if (squad->InSquad() && squad->IsSquadLeader() && squad->IsTheLeaderHuman() && squad->GetHumanSquadLeader() == humanleader)
		{
			iface = squad;
		}
	});

	return iface;
}

ISquad::ISquad(CBaseBot* bot) :
	IBotInterface(bot)
{
	m_squadmembers.reserve(8);
	m_leaderishuman = false;
}

ISquad::~ISquad()
{
}

void ISquad::Reset()
{
	DestroySquad(true);
}

void ISquad::Update()
{
	if (InSquad())
	{
		if (IsSquadLeader())
		{
			RemoveInvalidMembers();

			if (m_leaderishuman && m_humanleader.Get() == nullptr)
			{
				DestroySquad(false); // human leader is NULL, destroy
			}
		}
		else
		{
			// My squad leader is NULL
			if (m_squadleader.entity.Get() == nullptr)
			{
				DestroySquad(false); // destroy squad
			}
		}
	}
}

void ISquad::Frame()
{
	CBaseBot* me = GetBot();

	if (me->IsDebugging(BOTDEBUG_SQUADS))
	{
		if (InSquad())
		{
			std::unique_ptr<char[]> text = std::make_unique<char[]>(128);
			ke::SafeSprintf(text.get(), 128, "Squad Size: %i  Is Leader: %s <#%i> Human Squad: %s", 
				static_cast<int>(GetSquadSize()), IsSquadLeader() ? "YES" : "NO", m_squadleader.entity.GetEntryIndex(), m_leaderishuman ? "YES" : "NO");
			me->DebugDisplayText(text.get());
		}
	}
}

size_t ISquad::GetSquadSize() const
{
	if (!InSquad())
	{
		return 0;
	}

	size_t size = 1; // the leader is not part of the squad members vector

	if (m_leaderishuman)
	{
		size++; // also count the human leader
	}

	if (IsSquadLeader())
	{
		return size + m_squadmembers.size();
	}
	else
	{
		return size + m_squadleader.bot->GetSquadInterface()->m_squadmembers.size();
	}
}

bool ISquad::FormSquad(CBaseBot* follower)
{
	if (!CanFormSquads() || !CanJoinSquads() || InSquad())
	{
		return false;
	}

	m_squadleader.entity = GetBot()->GetEntity();
	m_squadleader.bot = GetBot();

	AddSquadMember(follower);

	follower->GetSquadInterface()->OnAddedToSquad(&m_squadleader);

	return true;
}

bool ISquad::FormSquad(CBaseEntity* leader)
{
	if (!CanFormSquads() || !CanJoinSquads() || InSquad())
	{
		return false;
	}

	m_squadleader.entity = GetBot()->GetEntity();
	m_squadleader.bot = GetBot();
	SetHumanLeader(leader);

	return true;
}

bool ISquad::JoinSquad(CBaseBot* follower)
{
	if (!IsSquadLeader() || !IsSquadValid())
	{
		return false;
	}

	AddSquadMember(follower);

	follower->GetSquadInterface()->OnAddedToSquad(&m_squadleader);

	if (m_leaderishuman)
	{
		follower->GetSquadInterface()->SetHumanLeader(m_humanleader.Get());
	}

	return true;
}

bool ISquad::FormOrJoinSquad(CBaseBot* follower, bool onlyifLeader)
{
	if (!CanFormSquads() || !CanJoinSquads())
	{
		return false;
	}

	if (InSquad())
	{
		if (IsSquadLeader()) // join: this bot is a leader
		{
			return JoinSquad(follower);
		}
		else if (!onlyifLeader) // join: this bot is a member but we allowing it
		{
			 // redirect the request
			return m_squadleader.bot->GetSquadInterface()->FormOrJoinSquad(follower, onlyifLeader);
		}
	}
	else
	{
		return FormSquad(follower);
	}

	return false;
}

const std::vector<ISquad::SquadMember>& ISquad::GetSquadMembers() const
{
	if (IsSquadLeader())
	{
		return m_squadmembers;
	}
	else
	{
		return m_squadleader.bot->GetSquadInterface()->m_squadmembers;
	}
}

void ISquad::OnAddedToSquad(const SquadMember* leader)
{
	m_squadleader.entity = leader->entity.Get();
	m_squadleader.bot = leader->bot;
}

void ISquad::OnRemovedFromSquad(const SquadMember* leader)
{
	if (m_squadleader.bot == leader->bot || m_squadleader.bot == nullptr)
	{
		DestroySquad(false);
	}
}

void ISquad::OnMemberLeftSquad(CBaseBot* member)
{
	RemoveSquadMember(member);
}

bool ISquad::IsSquadValid() const
{
	if (!InSquad())
	{
		return false;
	}

	if (m_leaderishuman && m_humanleader.Get() == nullptr)
	{
		return false;
	}

	return m_squadleader.entity.Get() != nullptr;
}

Vector ISquad::GetSquadLeaderPosition() const
{
	if (!IsSquadValid())
	{
		return vec3_origin;
	}

	if (m_leaderishuman)
	{
		return UtilHelpers::getEntityOrigin(m_humanleader.Get());
	}
	else
	{
		return m_squadleader.bot->GetAbsOrigin();
	}
}

CBaseEntity* ISquad::GetSquadLeaderEntity() const
{
	if (m_leaderishuman)
	{
		return m_humanleader.Get();
	}

	return m_squadleader.entity.Get();
}

void ISquad::AddSquadMember(CBaseBot* follower)
{
	if (!IsSquadLeader())
	{
		return;
	}

	bool is_member = std::any_of(std::begin(m_squadmembers), std::end(m_squadmembers), [&follower](SquadMember& member) {
		return member.bot == follower;
	});

	if (!is_member)
	{
		m_squadmembers.emplace_back(follower);
	}
}

void ISquad::RemoveSquadMember(CBaseBot* follower)
{
	m_squadmembers.erase(std::remove_if(m_squadmembers.begin(), m_squadmembers.end(), [&follower](SquadMember& member) {
		return member.bot == follower;
	}), m_squadmembers.end());
}

void ISquad::RemoveInvalidMembers()
{
	m_squadmembers.erase(std::remove_if(m_squadmembers.begin(), m_squadmembers.end(), [](SquadMember& member) {
		return member.entity.Get() == nullptr;
	}), m_squadmembers.end());
}

void ISquad::DestroySquad(bool notifyLeader)
{
	if (!InSquad())
		return;

	if (IsSquadLeader())
	{
		for (auto& member : m_squadmembers)
		{
			if (member.entity.Get() != nullptr)
			{
				member.bot->GetSquadInterface()->OnRemovedFromSquad(&m_squadleader);
			}
		}
	}
	else
	{
		if (notifyLeader)
		{
			if (m_squadleader.entity.Get() != nullptr)
			{
				m_squadleader.bot->GetSquadInterface()->OnMemberLeftSquad(GetBot());
			}
		}
	}

	m_squadleader.clear();
	m_squadmembers.clear();
	m_leaderishuman = false;
	m_humanleader.Term();
}
