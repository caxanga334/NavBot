#ifndef NAVBOT_BOT_SQUAD_INTERFACE_H_
#define NAVBOT_BOT_SQUAD_INTERFACE_H_

#include <vector>
#include <sdkports/sdk_ehandle.h>
#include "base_interface.h"

/**
 * @brief Bot interface for squads
 */
class ISquad : public IBotInterface
{
public:

	struct SquadMember
	{
		SquadMember() :
			entity(nullptr)
		{
			bot = nullptr;
		}

		SquadMember(CBaseBot* follower);

		CHandle<CBaseEntity> entity; // leader ent (bot or human)
		CBaseBot* bot; // leader bot ptr

		void clear()
		{
			this->entity.Term();
			this->bot = nullptr;
		}
	};

	/**
	 * @brief Given a human player, find a squad interface from the bot who is the sub leader of the human squad.
	 * @param humanleader Human player to search.
	 * @return Squad interface pointer or NULL if no bots formed a squad with this human player.
	 */
	static ISquad* GetSquadLeaderInterfaceForHumanLeader(CBaseEntity* humanleader);

	ISquad(CBaseBot* bot);
	~ISquad() override;

	// Reset the interface to it's initial state
	void Reset() override;
	// Called at intervals
	void Update() override;
	// Called every server frame
	void Frame() override;
	// true if the bot can create new squads
	virtual bool CanFormSquads() const { return true; }
	// true if the bot can join existing squads
	virtual bool CanJoinSquads() const { return !InSquad(); }
	// is this bot a squad leader?
	bool IsSquadLeader() const { return m_squadleader.bot == GetBot(); }
	// Is this bot squad leader a human player?
	bool IsTheLeaderHuman() const { return m_leaderishuman; }
	// Is this bot in a squad?
	bool InSquad() const { return m_squadleader.bot != nullptr; }
	// Gets the squad leader
	const SquadMember& GetMySquadLeader() const { return m_squadleader; }
	// Returns the human leader entity.
	CBaseEntity* GetHumanSquadLeader() const { return m_humanleader.Get(); }
	// Number of members in my squad (including myself)
	size_t GetSquadSize() const;

	/**
	 * @brief Creates a new squad. This bot becomes the leader.
	 * @param follower Bot that will become a squad member.
	 * @return true if the squad was created.
	 */
	virtual bool FormSquad(CBaseBot* follower);
	/**
	 * @brief Creates a new squad. This bot becomes the virtual leader while the real leader is a human player.
	 * @param leader Human player that will become the squad leader.
	 * @return true if the squad was created.
	 */
	virtual bool FormSquad(CBaseEntity* leader);
	/**
	 * @brief Joins this squad (this must be called on the leader).
	 * @param follower Bot that will join.
	 * @return true if the bot joined the squad.
	 */
	virtual bool JoinSquad(CBaseBot* follower);
	/**
	 * @brief Creates a new squad if the given bot is not leading one already or joins an existing squad.
	 * @param follower 
	 * @param onlyifLeader If true, the bot will only join an existing squad if this bot is the leader. False to allow joining even if this bot is not a leader.
	 * @return true if the bot joined a squad or created one.
	 */
	virtual bool FormOrJoinSquad(CBaseBot* follower, bool onlyifLeader = true);

	virtual const std::vector<SquadMember>& GetSquadMembers() const;

	// Called when the bot is added to a squad
	virtual void OnAddedToSquad(const SquadMember* leader);
	// Called when the bot is removed from the squad
	virtual void OnRemovedFromSquad(const SquadMember* leader);
	// Called for the leader when a bot leaves the squad
	virtual void OnMemberLeftSquad(CBaseBot* member);
	// Returns true if the squad is valid
	virtual bool IsSquadValid() const;
	// Returns the squad leader position
	virtual Vector GetSquadLeaderPosition() const;
	// Returns the squad leader entity
	virtual CBaseEntity* GetSquadLeaderEntity() const;

protected:
	void AddSquadMember(CBaseBot* follower);
	void RemoveSquadMember(CBaseBot* follower);
	void RemoveInvalidMembers();
	void DestroySquad(bool notifyLeader = false);
	void SetHumanLeader(CBaseEntity* leader)
	{
		m_humanleader = leader;
		m_leaderishuman = true;
	}

private:
	std::vector<SquadMember> m_squadmembers;
	SquadMember m_squadleader;
	CHandle<CBaseEntity> m_humanleader; // the human leader
	bool m_leaderishuman; // true if the squad leader is not a navbot
};

#endif // !NAVBOT_BOT_SQUAD_INTERFACE_H_