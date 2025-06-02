#ifndef NAVBOT_BOT_SQUAD_INTERFACE_H_
#define NAVBOT_BOT_SQUAD_INTERFACE_H_

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
	class SquadMember
	{
	public:
		// Default constructor
		SquadMember() :
			handle(nullptr)
		{
			botptr = nullptr;
		}

		// Constructor for humans
		SquadMember(CBaseEntity* player) :
			handle(player)
		{
			botptr = nullptr;
		}
		
		// Constructor for bots
		SquadMember(CBaseBot* bot);

		bool operator==(const SquadMember& other) const;
		bool operator==(CBaseBot* bot) const;
		bool operator==(CBaseEntity * player) const;

		void AssignBot(CBaseBot* bot);
		void AssignHuman(CBaseEntity* player)
		{
			handle = player;
			botptr = nullptr;
		}

		// Returns true if this squad member is valid
		bool IsValid() const { return handle.Get() != nullptr; }
		bool IsHuman() const { return botptr == nullptr; }
		bool IsBot() const { return botptr != nullptr; }

		CHandle<CBaseEntity> handle; // player entity handle
		CBaseBot* botptr; // bot pointer
	};

	class SquadData
	{
	public:
		SquadData()
		{
			members.reserve(16);
			destroyed = false;
		}

		std::vector<SquadMember> members;
		SquadMember leader;
		bool destroyed;

		// returns true if this squad is valid (leader exists)
		bool IsValid() const { return leader.handle.Get() != nullptr; }

		// Total number of players on this squad, including the leader
		std::size_t GetSquadSize() const { return members.size() + 1; /* + 1 because the leader isn't stored in the members vector */ }
		// Number of squad members, doesn't include the leader
		std::size_t GetSquadMemberCount() const { return members.size(); }
	};


	ISquad(CBaseBot* bot);
	~ISquad() override;

	// Reset the interface to it's initial state
	void Reset() override;
	// Called at intervals
	void Update() override;
	// Called every server frame
	void Frame() override;
	// Returns true if this bot can form and lead a squad.
	virtual bool CanFormSquads() const { return true; }
	// Returns true if this bot can join squads.
	virtual bool CanJoinSquads() const { return true; }
	// If true, this bot behavior's will be overriden with the squad behavior.
	virtual bool UseSquadBehavior() const { return true; }
	void OnKilled(const CTakeDamageInfo& info) override; // when the bot is killed
	void OnOtherKilled(CBaseEntity* pVictim, const CTakeDamageInfo& info) override; // when another player gets killed
	// true if the bot is in a squad
	bool IsInASquad() const { return m_squaddata.get() != nullptr; }
	// true if the bot is a squad leader
	bool IsSquadLeader() const;
	// Creates a new squad and becomes its leader
	void CreateSquad();
	// Destroys the current squad
	void DestroySquad();
	// Adds a new bot to this squad
	void AddSquadMember(CBaseBot* bot);
	// Joins a squad
	void JoinSquad(CBaseBot* leader);
	// Called when the squad gets destroyed
	virtual void OnSquadDestroyed() const {}
	// Called when a squad member leaves the squad.
	virtual void OnSquadMemberLeftSquad(CBaseEntity* member);
	// When a new member is added to the squad
	virtual void OnNewSquadMember(const SquadMember* member) {}
	// Gets a pointer to the squad data. Will be NULL if not in a squad.
	const SquadData* GetSquadData() const { return m_squaddata.get(); }
	// Gets the squad leader.
	const SquadMember* GetSquadLeader() const
	{
		if (m_squaddata)
		{
			return &m_squaddata->leader;
		}

		return nullptr;
	}
	// Tell the rest of the squad about a visible enemy
	void NotifyVisibleEnemy(CBaseEntity* enemy) const;
protected:
	std::shared_ptr<SquadData>& GetSquadDataPtr() { return m_squaddata; }
	// Copies this squad data to the other squad data.
	void CopySquadData(ISquad* other) const
	{
		other->m_squaddata = m_squaddata;
	}

private:
	std::shared_ptr<SquadData> m_squaddata;
};

#endif // !NAVBOT_BOT_SQUAD_INTERFACE_H_