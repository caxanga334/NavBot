#ifndef NAVBOT_BOT_SQUAD_INTERFACE_H_
#define NAVBOT_BOT_SQUAD_INTERFACE_H_

#include <array>
#include <vector>
#include <memory>
#include <sdkports/sdk_ehandle.h>
#include "base_interface.h"

/**
 * @brief Bot interface for squads
 */
class ISquad : public IBotInterface
{
public:
	/**
	 * @brief Squad Member
	 */
	class Member
	{
	public:
		Member() :
			m_player(nullptr), m_bot(nullptr), m_squad(nullptr)
		{
		}
		Member(CBaseBot* bot);
		Member(CBaseEntity* player) :
			m_player(player), m_bot(nullptr), m_squad(nullptr)
		{
		}

		bool operator==(const Member& other) const { return this->m_player == other.m_player; }
		bool operator!=(const Member& other) const { return this->m_player != other.m_player; }
		void operator=(CBaseBot* bot);
		void operator=(CBaseEntity* player)
		{
			m_player = player;
			m_bot = nullptr;
			m_squad = nullptr;
		}
		explicit operator bool() const { return IsValid(); }
		
		// Returns true if the reference player entity exists.
		bool IsValid() const { return m_player.Get() != nullptr; }
		// Returns true if this instance is a human (or a non NavBot bot).
		bool IsHuman() const { return m_bot == nullptr; }
		// Returns true if this member instance is a NavBot bot.
		bool IsBot() const { return m_bot != nullptr;  }
		// Returns the player entity of this member. (Can be NULL if the player no longer exists).
		CBaseEntity* GetPlayerEntity() const { return m_player.Get(); }
		// Returns the NavBot instance of this member. NULL if this member is a human or non NavBot bot.
		CBaseBot* GetBot() const { return m_bot; }
		// Returns the Squad instance of this member. NULL if this member is a human or non NavBot bot.
		ISquad* GetSquadInterface() const { return m_squad; }
		// Returns the abs origin of this squad member.
		Vector GetPosition() const;

	private:
		CHandle<CBaseEntity> m_player; // handle to the player entity.
		CBaseBot* m_bot; // bot itself
		ISquad* m_squad; // pointer to the bot's squad interface.
	};
	/**
	 * @brief Class containing the squad data shared between all bots on the same squad.
	 */
	class SquadData
	{
	public:
		SquadData() :
			m_leaderpos(0.0f, 0.0f, 0.0f)
		{
			m_members.reserve(16);
			m_leader = nullptr;
			m_humanleader = nullptr;
			m_destroyed = false;
		}
		virtual ~SquadData() {}
		/**
		 * @brief Assings the squad leaders.
		 * @param bot The bot that will lead the squad.
		 * @param player Optional entity of a human player, if set, the squad will be led by the human player.
		 */
		void AssignLeaders(CBaseBot* bot, CBaseEntity* player = nullptr)
		{
			m_leader = AddMember(bot);

			if (player)
			{
				m_humanleader = AddMember(player);
			}
			else
			{
				m_humanleader = nullptr;
			}
		}
		// Returns the squad leader.
		const Member* GetSquadLeader() const
		{
			if (m_humanleader)
			{
				return m_humanleader;
			}

			return m_leader;
		}
		const Vector& GetSquadLeaderPosition() const { return m_leaderpos; }
		// Returns the squad's bot leader.
		const Member* GetBotLeader() const
		{
			return m_leader;
		}
		/**
		 * @brief Adds a bot to this squad.
		 * @param bot Bot to add.
		 * @return Pointer to the member or NULL on failure.
		 */
		const Member* AddMember(CBaseBot* bot);
		/**
		 * @brief Adds a human to this squad.
		 * @param player Entity of the human player.
		 * @return Pointer to the member or NULL on failure.
		 */
		const Member* AddMember(CBaseEntity* player);
		/**
		 * @brief Finds a squad member by their entity.
		 * @param player Entity to search for.
		 * @return Pointer to member or NULL if not found.
		 */
		const Member* FindMember(CBaseEntity* player) const
		{
			if (!player) { return nullptr; }

			for (auto& member : m_members)
			{
				CBaseEntity* ent = member.GetPlayerEntity();

				if (ent == player)
				{
					return &member;
				}
			}

			return nullptr;
		}
		/**
		 * @brief Removes a member from the squad.
		 * @param player Pointer to the player entity of the member to remove.
		 * @return True if the member was found and removed.
		 */
		bool RemoveMember(CBaseEntity* player);
		/**
		 * @brief Checks if the given player is the squad's leader.
		 * @param player Player to check.
		 * @return True if the given player is the squad's current leader. False otherwise.
		 */
		bool IsSquadLeader(CBaseEntity* player)
		{
			if (!player) { return false; }

			if (m_humanleader && m_humanleader->GetPlayerEntity() == player)
			{
				return true;
			}

			if (m_leader && m_leader->GetPlayerEntity() == player)
			{
				return true;
			}

			return false;
		}
		// Returns the number of members in the squad. (May count invalid members).
		std::size_t GetMemberCount() const { return m_members.size(); }
		// Returns true if this squad is led by a human player (or a non NavBot bot).
		bool IsHumanLedSquad() const { return m_humanleader != nullptr; }
		// Returns true if the squad is valid, false if invalid.
		bool IsValid() const
		{
			if (m_destroyed) { return false; }
			// the squad must have a valid bot leading it.
			if (!m_leader || !m_leader->IsValid()) { return false; }
			// if led by a human, the human must also be valid.
			if (m_humanleader && !m_humanleader->IsValid()) { return false; }

			return true;
		}
		// Returns true if this squad was marked for destruction.
		bool IsMarkedForDestruction() const { return m_destroyed; }
		/**
		 * @brief Runs a function on each squad member.
		 * @tparam F Function to run. void (const ISquad::Member& member)
		 * @param func Function to run.
		 */
		template <typename F>
		void ForEachMember(const F& func) const
		{
			for (auto& member : m_members)
			{
				if (member)
				{
					func(member);
				}
			}
		}

	private:
		friend class ISquad;

		std::vector<Member> m_members; // vector of squad members
		const Member* m_leader; // Pointer to the squad leader
		const Member* m_humanleader; // Pointer to the human leader
		bool m_destroyed; // stores if this squad is marked for destruction.
		Vector m_leaderpos; // cached squad leader position.

		// Marks the squad data for destruction
		void MarkForDestruction() { m_destroyed = true; }
		// Removes any member instance that points to a NULL player.
		void PurgeInvalidMembers();
		void UpdateLeaderPosition();
	};

	ISquad(CBaseBot* bot);
	~ISquad() override;

	void Reset() override;
	void Update() override;
	void Frame() override;
	void OnKilled(const CTakeDamageInfo& info) override;
	void OnSquadEvent(SquadEventType evtype) override;

	// Returns true if the bot is in a squad. (The squad may be in an invalid state).
	bool IsInASquad() const { return static_cast<bool>(m_data); }
	// Returns true if the bot is in a squad and in a valid state. (May contain invalid members).
	bool IsSquadValid() const { return IsInASquad() && m_data->IsValid(); }
	// Gets the squad data. Will be NULL if the bot is not in a squad.
	const SquadData* GetSquadData() const { return m_data.get(); }
	// Returns true if this bot is a squad leader.
	bool IsSquadLeader() const
	{
		if (!IsSquadValid()) { return false; }

		const ISquad::Member* leader = m_data->GetBotLeader();

		if (!leader || !leader->IsValid()) { return false; }

		return leader->GetBot() == GetBot<CBaseBot>();
	}
	// Returns true if this squad is led by a human player.
	bool IsHumanLedSquad() const
	{
		if (!IsSquadValid()) { return false; }

		const ISquad::Member* leader = m_data->GetSquadLeader();

		if (!leader || !leader->IsValid()) { return false; }

		return leader->IsHuman();
	}
	// Returns true if this bot can join squads.
	virtual bool CanJoinSquads() const;
	// Returns true if this bot can create and lead squads.
	virtual bool CanLeadSquads() const;
	// Returns true if this bot can replace the squad leader.
	virtual bool CanBePromoted() const { return true; }
	// Returns true if this squad allows promotions.
	virtual bool AllowLeaderPromotions() const { return true; }
	// Returns true if this bot should use squad behavior when in a squad.
	virtual bool UsesSquadBehavior() const { return true; }
	/**
	 * @brief Called when a bot is getting invited to a squad.
	 * @param leader The leader of the squad.
	 * @return Returns true this bot should join the squad, false otherwise.
	 */
	virtual bool OnInvitedToSquad(CBaseBot* leader) const;
	/**
	 * @brief Creates a squad. The interface owner bot will lead the squad.
	 * @param human Optional human player to set as the squad's human leader.
	 * @return True if the squad was created, false otherwise.
	 */
	virtual bool CreateSquad(CBaseEntity* human = nullptr);
	/**
	 * @brief If a member, leaves the squad. If a leader, disbands and destroy the squad.
	 */
	virtual void DestroySquad();
	/**
	 * @brief Adds a NavBot to this squad.
	 * @param bot NavBot to be added.
	 * @return True if added to the squad. Falseotherwise
	 */
	virtual bool AddMemberToSquad(CBaseBot* bot);
	/**
	 * @brief Adds a human to this squad.
	 * @param player Human to be added.
	 * @return True if added to the squad. Falseotherwise
	 */
	virtual bool AddMemberToSquad(CBaseEntity* player);
	/**
	 * @brief Removes a member from the squad.
	 * @param player Member to be removed (NavBot or Human).
	 */
	virtual void RemoveMemberFromSquad(CBaseEntity* player);
	/**
	 * @brief Gets the squad leader's interface of the squad the player is in.
	 * @param player Entity of the player to retrieve the squad of.
	 * @param leaderonly If true, only search for squads the player is the leader of.
	 * @return Squad interface pointer or NULL if not found.
	 */
	static ISquad* GetHumanPlayerSquad(CBaseEntity* player, const bool leaderonly = false);

protected:
	// Allocates a new SquadData
	virtual SquadData* CreateSquadData() const { return new SquadData; }
	// Copies the squad data shared ptr from the 'from' squad to this.
	void CopySquadData(ISquad* from)
	{
		if (!from->m_data) { return; }

		m_data = from->m_data;
	}
	/**
	 * @brief Called by squad members to notify their leader that they're leaving the squad.
	 * @param other Bot leaving the squad.
	 */
	virtual void NotifyBotLeftSquad(CBaseBot* other);
	/**
	 * @brief Called to clean up invalid squad data.
	 */
	virtual void PurgeInvalidSquadData();
	/**
	 * @brief Validates the squad state.
	 * @return True if the squad is in a valid state, false otherwise.
	 */
	virtual bool Validate();
	/**
	 * @brief Selects a new bot to be the leader of this squad.
	 * @return Returns true if a bot was promoted, false otherwise.
	 */
	bool PromoteNewLeader();
	/**
	 * @brief Called when selecting a new bot to promote as the squad leader.
	 * @param first First bot.
	 * @param second Second bot.
	 * @return Return which bot should become leader.
	 */
	virtual CBaseBot* SelectNextLeader(CBaseBot* first, CBaseBot* second) const;
	/**
	 * @brief Called when the squad leader is killed.
	 */
	virtual void OnLeaderKilled();

private:
	std::shared_ptr<SquadData> m_data;

};

namespace botsquadutils
{
	class TryFormSquadFunc
	{
	public:
		TryFormSquadFunc(CBaseBot* leader, CBaseEntity* human, int max_squad_size);

		void operator()(CBaseBot* bot);
		
		void CreateSquad();

	private:
		int m_max_size;
		CBaseBot* m_leader;
		ISquad* m_squad;
		CBaseEntity* m_player;
		std::vector<CBaseBot*> m_candidates;

	};
}

#endif // !NAVBOT_BOT_SQUAD_INTERFACE_H_