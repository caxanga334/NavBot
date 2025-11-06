#ifndef NAVBOT_BOT_SQUAD_INTERFACE_H_
#define NAVBOT_BOT_SQUAD_INTERFACE_H_

#include <array>
#include <vector>
#include <memory>
#include <sdkports/sdk_ehandle.h>
#include "base_interface.h"

class ISquad;
class ISensor;

/**
 * @brief Bot interface for squads
 */
class ISquad : public IBotInterface
{
public:
	class SquadMember
	{
	public:
		SquadMember(); // Default constructor
		SquadMember(CBaseBot* bot); // Constructor for bots
		SquadMember(CBaseEntity* player); // Constructor for humans

		// Returns the player entity of this squad member, NULL if the player is no longer valid.
		CBaseEntity* GetPlayerEntity() const { return m_player.Get(); }
		// Returns the player entity index.
		int GetPlayerIndex() const { return m_player.GetEntryIndex(); }
		// True if the squad member has a valid player entity.
		bool IsValid() const { return m_player.Get() != nullptr; }
		// Returns true if this is a human member. (Note: Also returns true for non NavBot bots).
		bool IsHuman() const { return m_bot == nullptr; }
		// Returns true if this is a NavBot member. (Will be false for non NavBot bots).
		bool IsBot() const { return m_bot != nullptr; }
		// Returns how many seconds have passed since this member joined the squad.
		float GetTimeSinceMember() const;
		// Returns the NavBot instance of this squad member. NULL if human.
		CBaseBot* GetBot() const { return m_bot; }
		// Returns the squad interface of this member. NULL on humans.
		ISquad* GetSquadInterface() const { return m_squadinterface; }
		// Returns the squad member entity's abs origin.
		Vector GetPosition() const;

		void Init(CBaseEntity* pPlayer, CBaseBot* bot);
	private:
		CHandle<CBaseEntity> m_player; // CBasePlayer entity of this squad member.
		CBaseBot* m_bot; // Bot pointer of this squad member.
		ISquad* m_squadinterface; // Squad interface pointer of the bot.
		float m_joinTime; // Timestamp of when this member joined the squad.
	};

	class SquadData
	{
	public:
		SquadData();

		virtual ~SquadData();

		// Is this squad data instance valid?
		bool IsValid() const
		{
			if (m_destroyed) { return false; }

			if (m_humanled) // human squad, both the bot and the human leader must be valid
			{
				return m_botleader.IsValid() && m_humanleader.IsValid();
			}

			return m_botleader.IsValid();
		}
		// Returns true if the squad is led by a human player.
		bool IsHumanLedSquad() const { return m_humanleader.IsValid(); }
		// Returns the SquadMember instance of the squad leader. The leader may be a human or bot.
		const SquadMember* GetSquadLeader() const
		{
			if (m_humanled) { return &m_humanleader; }

			return &m_botleader;
		}
		// Returns the SquadMember instance of the squad's bot leader.
		const SquadMember* GetBotLeader() const { return &m_botleader; }
		// Returns the squad leader's position.
		Vector GetSquadLeaderPosition() const;
		// Returns true if the squad leader is alive.
		bool IsSquadLeaderAlive() const;
		// Returns how many seconds have passed since this squad was created.
		float GetTimeSinceCreation() const;
		// Returns the number of members in this squad.
		std::size_t GetMembersCount() const { return m_members.size(); }
		// Returns the total squad size (members + leader)
		std::size_t GetSquadSize() const;
		// Returns the squad member vector
		const std::vector<SquadMember>& GetMembers() const { return m_members; }
		// Returns the team index of this squad.
		int GetTeamNumber() const { return m_teamNum; }
	private:
		friend class ISquad;

		bool m_destroyed; // this squad was destroyed
		bool m_humanled; // the actual squad leader is a human player
		float m_createdTime; // timestamp of when this squad was created
		SquadMember m_botleader;
		SquadMember m_humanleader;
		std::vector<SquadMember> m_members;
		int m_teamNum;

		void RemoveInvalidMembers(); // purges any members with a NULL player entity
		void DoFullValidation(ISensor* sensor); // complex squad validation, destroyed if invalid
		void Destroy(); // marks the squad as destroyed, notifies all members about it
		void AddMember(CBaseBot* bot);
		void RemoveMember(CBaseBot* bot);
		virtual void Init(); // called when a new squaddata instance is allocated
	};

	ISquad(CBaseBot* bot);
	~ISquad() override;

	void Reset() override;
	void Update() override;
	void Frame() override {}

	// Returns true if this bot can join squads as a member.
	virtual bool CanJoinSquads() const { return !IsInASquad(); }
	// Returns true if this bot can create and lead squads.
	virtual bool CanLeadSquads() const { return true; }
	/**
	 * @brief Invoked when a bot gets invited into a squad.
	 * @param leader The bot leading the squad.
	 * @return Return true to allow the bot to join the squad, false to deny.
	 */
	virtual bool OnInvitedToJoinSquad(CBaseBot* leader, const SquadData* squaddata) const;
	/**
	 * @brief Creates a new squad.
	 * @param humanLeader If this is a human led squad, the CBaseEntity pointer of the human's player entity should be passed. NULL for a bot led squad.
	 */
	virtual void CreateSquad(CBaseEntity* humanLeader = nullptr);
	/**
	 * @brief Adds a bot to the current squad. Does nothing unless the bot is a squad leader.
	 * @param member Bot to add to the squad.
	 */
	virtual void AddToSquad(CBaseBot* member);
	/**
	 * @brief Tries to add the given bot the current squad.
	 * @param member Bot to add to the squad.
	 * @return True if the bot was added, false otherwise.
	 */
	bool TryToAddToSquad(CBaseBot* member);
	// Removes the bot from the current squad, if the bot is a squad leader, the squad is destroyed.
	virtual void LeaveSquad();

	// Returns true if this bot is in a squad.
	bool IsInASquad() const { return m_squaddata.get() != nullptr; }
	// Returns true if the current squad is valid.
	bool IsSquadValid() const { return IsInASquad() && m_squaddata->IsValid(); }
	// Returns the squad data. NULL if not in a squad.
	const SquadData* GetSquad() const { return m_squaddata.get(); }
	// Returns true if this bot is a squad leader.
	bool IsSquadLeader() const { return IsSquadValid() && m_squaddata->GetBotLeader()->GetSquadInterface() == this; }
	// Returns true if the bot current squad is led by a human
	bool IsSquadLedByAHuman() const { return IsSquadValid() && m_squaddata->IsHumanLedSquad(); }
	// Notifies this bot squad was destroyed.
	virtual void NotifySquadDestruction();
	/**
	 * @brief Notifies this bot a member left the squad.
	 * @param member Bot that is leaving the squad.
	 * @param is_deleting If true, the bot instance is getting deleted.
	 */
	virtual void NotifyMemberLeftSquad(CBaseBot* member, const bool is_deleting = false);
	/**
	 * @brief Searches for the squad interface currently led by the given player.
	 * @param humanLeader Player pointer.
	 * @return Pointer to a squad interface led by the given human player. NULL if none.
	 */
	static ISquad* GetSquadInterfaceOfHumanLeader(CBaseEntity* humanLeader);

	class InviteBotsToSquadFunc
	{
	public:
		InviteBotsToSquadFunc(CBaseBot* me, int max) : m_max(max)
		{
			m_me = me;
		}

		void operator()(CBaseBot* bot);

	private:
		CBaseBot* m_me;
		int m_max;
	};

	class DestroyAllSquadsFunc
	{
	public:
		void operator()(CBaseBot* bot);

	private:

	};

	static void DestroyAllSquads();

	static void IncrementSquadCount(int team)
	{
		s_numsquads[team] += 1;
	}

	static void DecrementSquadCount(int team)
	{
		s_numsquads[team] -= 1;

		if (s_numsquads[team] < 0)
		{
			s_numsquads[team] = 0;
		}
	}

	static int GetNumSquadsOnTeam(int team)
	{
		if (team >= 0 && team < MAX_TEAMS)
		{
			return s_numsquads[team];
		}

		return 0;
	}

	static void ResetSquadCounts()
	{
		std::fill(s_numsquads.begin(), s_numsquads.end(), 0);
	}

	static CountdownTimer& GetSquadTeamTimer(int team)
	{
		if (team >= 0 && team < MAX_TEAMS)
		{
			return s_squadtimer[team];
		}

		return s_squadtimer[0];
	}

	static void ResetSquadTeamTimers()
	{
		std::for_each(s_squadtimer.begin(), s_squadtimer.end(), [](CountdownTimer& timer) { timer.Invalidate(); });
	}

protected:
	// Allocates a new SquadData object. Override if using a mod specific squad data
	virtual SquadData* AllocateSquadData() const { return new SquadData; }
	// gets a copy of the squad data smart pointer
	std::shared_ptr<SquadData> GetSquadDataPtr() const { return m_squaddata; }
private:
	std::shared_ptr<SquadData> m_squaddata;
	CountdownTimer m_fullValidationTimer;
	CountdownTimer m_shareInfoTimer;

	static inline std::array<int, MAX_TEAMS> s_numsquads{};
	static inline std::array<CountdownTimer, MAX_TEAMS> s_squadtimer{}; // generic per team shared timer for squad creation cooldowns
};

// Utility bot squad stuff
namespace botsquads
{

}

#endif // !NAVBOT_BOT_SQUAD_INTERFACE_H_