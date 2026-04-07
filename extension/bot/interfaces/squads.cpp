#include NAVBOT_PCH_FILE
#include <algorithm>
#include <extension.h>
#include <manager.h>
#include <mods/basemod.h>
#include <mods/modhelpers.h>
#include <bot/basebot.h>
#include "squads.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

ISquad::Member::Member(CBaseBot* bot)
{
	m_player = bot->GetEntity();
	m_bot = bot;
	m_squad = bot->GetSquadInterface();
}

void ISquad::Member::operator=(CBaseBot* bot)
{
	m_player = bot->GetEntity();
	m_bot = bot;
	m_squad = bot->GetSquadInterface();
}

Vector ISquad::Member::GetPosition() const
{
	return UtilHelpers::getEntityOrigin(m_player.Get());
}

const ISquad::Member* ISquad::SquadData::AddMember(CBaseBot* bot)
{
	const Member* member = FindMember(bot->GetEntity());

	if (member)
	{
		return member;
	}

	auto& newmember = m_members.emplace_back(bot);
	return &newmember;
}

const ISquad::Member* ISquad::SquadData::AddMember(CBaseEntity* player)
{
	const Member* member = FindMember(player);

	if (member)
	{
		return member;
	}

	auto& newmember = m_members.emplace_back(player);
	return &newmember;
}

bool ISquad::SquadData::RemoveMember(CBaseEntity* player)
{
	if (!player) { return false; }

	for (auto it = m_members.begin(); it != m_members.end(); it++)
	{
		if (it->GetPlayerEntity() == player)
		{
			if (IsSquadLeader(player))
			{
				m_leader = nullptr;
				m_humanleader = nullptr;
				MarkForDestruction();
			}

			m_members.erase(it);
			return true;
		}
	}

	return false;
}

void ISquad::SquadData::PurgeInvalidMembers()
{
	// already invalid, don't bother
	if (!IsValid()) { return; }

	// validate leaders

	if (m_humanleader)
	{
		// human leader no longer exists
		if (!m_humanleader->IsValid())
		{
			MarkForDestruction();
			return;
		}

		// human leader is dead!
		if (!modhelpers->IsAlive(m_humanleader->GetPlayerEntity()))
		{
			MarkForDestruction();
			return;
		}
	}

	if (!m_leader || !m_leader->IsValid())
	{
		MarkForDestruction();
		return;
	}

	// remove invalid members
	m_members.erase(std::remove_if(m_members.begin(), m_members.end(), [](const ISquad::Member& member) {
		return !member.IsValid();
	}), m_members.end());
}

void ISquad::SquadData::UpdateLeaderPosition()
{
	const ISquad::Member* leader = GetSquadLeader();

	if (leader && leader->IsValid())
	{
		m_leaderpos = UtilHelpers::getEntityOrigin(leader->GetPlayerEntity());
	}
}

ISquad::ISquad(CBaseBot* bot) :
	IBotInterface(bot), m_data(nullptr)
{
}

ISquad::~ISquad()
{
	if (IsInASquad())
	{
		if (IsSquadLeader())
		{
			// mark for destructions, squad members will handle clean up on their Update call
			m_data->MarkForDestruction();
			m_data = nullptr;
		}
	}
}

void ISquad::Reset()
{
	DestroySquad();
}

void ISquad::Update()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("ISquad::Update", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (!IsInASquad())
	{
		return; // not in a squad, do nothing
	}

	if (!Validate())
	{
		// validation failed, remove squad
		DestroySquad();
		return;
	}

	if (IsSquadLeader())
	{
		// do a clean up
		PurgeInvalidSquadData();
		m_data->UpdateLeaderPosition();
	}
}

void ISquad::Frame()
{
}

void ISquad::OnKilled(const CTakeDamageInfo& info)
{
	if (!IsSquadValid()) { return; }

	if (IsSquadLeader())
	{
		OnLeaderKilled();
	}
	else
	{
		m_data->GetBotLeader()->GetSquadInterface()->NotifyBotLeftSquad(GetBot<CBaseBot>());
	}
}

void ISquad::OnSquadEvent(IEventListener::SquadEventType evtype)
{
	// Prevents infinite recursion
	if (!IsInASquad()) { return; }

	switch (evtype)
	{
	case IEventListener::SquadEventType::SQUAD_EVENT_DESTROYED:
		DestroySquad();
		break;
	default:
		break;
	}
}

bool ISquad::CanJoinSquads() const
{
	if (IsInASquad()) { return false; }

	return true;
}

bool ISquad::CanLeadSquads() const
{
	if (IsInASquad()) { return false; }

	return true;
}

bool ISquad::OnInvitedToSquad(CBaseBot* leader) const
{
	const float maxdist = extmanager->GetMod()->GetModSettings()->GetSquadMaxInviteDistance();

	if (GetBot<CBaseBot>()->GetRangeTo(leader->GetAbsOrigin()) > maxdist)
	{
		return false;
	}

	return true;
}

bool ISquad::CreateSquad(CBaseEntity* human)
{
	if (!CanLeadSquads()) { return false; }

	CBaseBot* bot = GetBot<CBaseBot>();
	m_data.reset(CreateSquadData());
	m_data->AssignLeaders(bot, human);
	bot->OnSquadEvent(IEventListener::SquadEventType::SQUAD_EVENT_CREATED);

	if (bot->IsDebugging(BOTDEBUG_SQUADS))
	{
		bot->DebugPrintToConsole(34, 139, 34, "%s SQUAD CREATED. HUMAN LEADER IS %s\n", bot->GetDebugIdentifier(), UtilHelpers::textformat::FormatEntity(human));
	}

	m_data->UpdateLeaderPosition();
	return true;
}

void ISquad::DestroySquad()
{
	if (!IsInASquad()) { return; }

	CBaseBot* bot = GetBot<CBaseBot>();

	if (bot->IsDebugging(BOTDEBUG_SQUADS))
	{
		bot->DebugPrintToConsole(255, 69, 0, "%s SQUAD DESTROYED. \n", bot->GetDebugIdentifier());
	}

	if (IsSquadLeader())
	{
		m_data->MarkForDestruction();
	}
	else
	{
		const ISquad::Member* leader = m_data->GetBotLeader();

		if (leader && leader->IsValid())
		{
			leader->GetSquadInterface()->NotifyBotLeftSquad(GetBot<CBaseBot>());
		}
	}

	// m_data must be set to NULL before firing the destroyed event to avoid an infinite recursion
	m_data = nullptr;
	bot->OnSquadEvent(IEventListener::SquadEventType::SQUAD_EVENT_DESTROYED);
}

bool ISquad::AddMemberToSquad(CBaseBot* bot)
{
	if (!IsSquadValid()) { return false; }
	if (!IsSquadLeader()) { return false; }
	if (!bot->GetSquadInterface()->CanJoinSquads()) { return false; }
	// already a member, just report as true.
	if (m_data->FindMember(bot->GetEntity()) != nullptr) { return true; }

	bool added = m_data->AddMember(bot) != nullptr;

	if (added)
	{
		if (GetBot<CBaseBot>()->IsDebugging(BOTDEBUG_SQUADS))
		{
			GetBot<CBaseBot>()->DebugPrintToConsole(34, 139, 34, "%s BOT \"%s\" WAS ADDED TO MY SQUAD! \n", GetBot<CBaseBot>()->GetDebugIdentifier(), bot->GetClientName());
		}

		bot->GetSquadInterface()->CopySquadData(this); // the data must be copied before the event is fired so behavior can access the squad data.
		bot->OnSquadEvent(IEventListener::SquadEventType::SQUAD_EVENT_JOINED);
	}

	return added;
}

bool ISquad::AddMemberToSquad(CBaseEntity* player)
{
#ifdef EXT_DEBUG
	// this function should only be called on humans and non NavBot bots.
	if (extmanager->GetBotFromEntity(player) != nullptr)
	{
		smutils->LogError(myself, "ISquad::AddMemberToSquad(CBaseEntity* player) called on a NavBot instance!");
		return false;
	}
#endif // EXT_DEBUG

	if (!IsSquadValid()) { return false; }
	if (!IsSquadLeader()) { return false; }
	// already a member, just report as true.
	if (m_data->FindMember(player) != nullptr) { return true; }
	return m_data->AddMember(player) != nullptr;
}

void ISquad::RemoveMemberFromSquad(CBaseEntity* player)
{
	if (!player || !IsSquadValid() || !IsSquadLeader()) { return; }

	const ISquad::Member* member = m_data->FindMember(player);

	if (!member || !member->IsValid()) { return; }

	CBaseBot* other = member->GetBot();

	if (m_data->RemoveMember(player))
	{
		if (other)
		{
			other->OnSquadEvent(IEventListener::SquadEventType::SQUAD_EVENT_DESTROYED);
		}

		CBaseBot* bot = GetBot<CBaseBot>();

		if (bot->IsDebugging(BOTDEBUG_SQUADS))
		{
			bot->DebugPrintToConsole(238, 130, 238, "%s MEMBER \"%s\" WAS REMOVED FROM THE SQUAD! \n", bot->GetDebugIdentifier(), UtilHelpers::textformat::FormatEntity(player));
		}
	}
}

ISquad* ISquad::GetHumanPlayerSquad(CBaseEntity* player, const bool leaderonly)
{
	if (!player) { return nullptr; }

	ISquad* squad = nullptr;

	auto func = [&squad, player, leaderonly](CBaseBot* bot) {
		// found one already
		if (squad != nullptr) { return; }

		ISquad* other = bot->GetSquadInterface();

		// the bot must be the leader
		if (!other->IsSquadValid() || !other->IsSquadLeader())
		{
			return;
		}

		const ISquad::Member* leader = other->GetSquadData()->GetSquadLeader();

		if (!leader || !leader->IsValid())
		{
			return;
		}

		if (leader->IsHuman())
		{
			if (player == leader->GetPlayerEntity())
			{
				squad = other;
				return;
			}
		}

		// if leader only, the human player must be the squad's leader
		if (leaderonly) { return; }

		const ISquad::Member* member = other->GetSquadData()->FindMember(player);

		if (member)
		{
			squad = other;
		}
	};

	extmanager->ForEachBot(func);

	return nullptr;
}

void ISquad::NotifyBotLeftSquad(CBaseBot* other)
{
	if (!IsSquadValid()) { return; }

	if (IsSquadLeader())
	{
		RemoveMemberFromSquad(other->GetEntity());
	}
#ifdef EXT_DEBUG
	else
	{
		smutils->LogError(myself, "ISquad::NotifyBotLeftSquad called on a non squad leader bot!");
	}
#endif // EXT_DEBUG
}

void ISquad::PurgeInvalidSquadData()
{
	m_data->PurgeInvalidMembers();
}

bool ISquad::Validate()
{
	if (m_data->IsValid())
	{
		// destroy single member squads
		if (m_data->GetMemberCount() <= 1U)
		{
			return false;
		}

		return true;
	}

	return false;
}

bool ISquad::PromoteNewLeader()
{
	if (m_data->IsMarkedForDestruction()) { return false; }
	
	const ISquad::Member* current_leader = m_data->GetBotLeader();

	if (!current_leader || !current_leader->IsValid()) { return false; }

	CBaseBot* old_leader = current_leader->GetBot();
	CBaseBot* next_leader = nullptr;

	for (auto& member : m_data->m_members)
	{
		if (member && member.IsBot())
		{
			CBaseBot* second = member.GetBot();

			if (second == old_leader) { continue; }

			if (!second->GetSquadInterface()->CanBePromoted()) { continue; }

			if (!next_leader)
			{
				next_leader = member.GetBot();
				continue;
			}

			next_leader = SelectNextLeader(next_leader, second);
		}
	}

	if (next_leader)
	{
		CBaseEntity* humanleader = nullptr;

		if (m_data->m_humanleader)
		{
			humanleader = m_data->m_humanleader->GetPlayerEntity();
		}

		m_data->AssignLeaders(next_leader, humanleader);
		next_leader->OnSquadEvent(IEventListener::SquadEventType::SQUAD_EVENT_PROMOTED);
		next_leader->GetSquadInterface()->RemoveMemberFromSquad(old_leader->GetEntity());
		
		if (old_leader->IsDebugging(BOTDEBUG_SQUADS))
		{
			old_leader->DebugPrintToConsole(34, 139, 34, "%s BOT \"%s\" WAS PROMOTED TO SQUAD LEADER! \n", old_leader->GetDebugIdentifier(), next_leader->GetClientName());
		}

		return true;
	}

	return false;
}

CBaseBot* ISquad::SelectNextLeader(CBaseBot* first, CBaseBot* second) const
{
	if (first->GetHealth() > second->GetHealth())
	{
		return first;
	}

	return second;
}

void ISquad::OnLeaderKilled()
{
	CBaseBot* bot = GetBot<CBaseBot>();

	for (auto& member : m_data->m_members)
	{
		if (member && member.IsBot())
		{
			// skip self
			if (member.GetBot() == bot) { continue; }

			// notify my squad mates that I am dead.
			member.GetSquadInterface()->OnSquadEvent(IEventListener::SquadEventType::SQUAD_EVENT_LEADER_KILLER);
		}
	}

	// If promotions are allowed, select a new leader, else destroy the squad.
	if (AllowLeaderPromotions())
	{
		if (!PromoteNewLeader())
		{
			DestroySquad();
		}
	}
	else
	{
		DestroySquad();
	}
}

botsquadutils::TryFormSquadFunc::TryFormSquadFunc(CBaseBot* leader, CBaseEntity* human, int max_squad_size)
{
	m_max_size = max_squad_size;
	m_leader = leader;
	m_squad = leader->GetSquadInterface();
	m_player = human;
	m_candidates.reserve(32);

	EXT_ASSERT(m_squad->CanLeadSquads(), "TryFormSquadFunc leader bot cannot lead squads!");
}

void botsquadutils::TryFormSquadFunc::operator()(CBaseBot* bot)
{
	if (!bot->IsAlive()) { return; }

	// must be a teammate
	if (!m_leader->GetSensorInterface()->IsFriendly(bot->GetEntity())) { return; }

	ISquad* other = bot->GetSquadInterface();

	// must be allowed to join squads and must not be in one already.
	if (other->IsInASquad() || !other->CanJoinSquads()) { return; }
	// must accept my invitation.
	if (!other->OnInvitedToSquad(m_leader)) { return; }

	m_candidates.push_back(bot);
}

void botsquadutils::TryFormSquadFunc::CreateSquad()
{
	// no teammates found :(
	if (m_candidates.empty()) { return; }
	// failed to create the squad
	if (!m_squad->CreateSquad(m_player)) { return; }
	
	std::sort(m_candidates.begin(), m_candidates.end(), UtilHelpers::algorithm::SortBotVectorNearest(m_leader->GetAbsOrigin()));

	int c = 0;

	for (CBaseBot* candidate : m_candidates)
	{
		if (m_squad->AddMemberToSquad(candidate))
		{
			if (++c >= m_max_size)
			{
				return;
			}
		}
	}
}
