#ifndef __NAVBOT_INSMIC_LIB_H_
#define __NAVBOT_INSMIC_LIB_H_

#include "insmic_shareddefs.h"

namespace insmiclib
{
	inline int EncodeSquadData(int squad, int slot)
	{
		int i = 0;
		int* ptr = &i;
		insmic::InsMICEncodedSquadInfoData_t* data = reinterpret_cast<insmic::InsMICEncodedSquadInfoData_t*>(ptr);

		data->squad = squad + 1;
		data->slot = slot + 1;

		return i;
	}

	inline void DecodeSquadData(int encoded, int& squad, int& slot)
	{
		int* ptr = &encoded;
		insmic::InsMICEncodedSquadInfoData_t* data = reinterpret_cast<insmic::InsMICEncodedSquadInfoData_t*>(ptr);

		squad = data->squad - 1;
		slot = data->slot - 1;
	}
	/**
	 * @brief Gets the Squad ID of a CINSSquad entity.
	 * @param pEntity Pointer to a CINSSquad entity.
	 * @return Squad ID of the squad manager entity.
	 */
	int GetSquadID(CBaseEntity* pEntity);
	/**
	 * @brief Counts the number of clients in each team.
	 * @param[out] teamone Clients in Team 1 (USMC)
	 * @param[out] teamtwo Clients in Team 2 (Insurgents)
	 */
	void GetTeamClientCount(int& teamone, int& teamtwo);
	/**
	 * @brief Gets a random free squad slot from a given squad manager entity.
	 * @param[in] pEntity Pointer to a CINSSquad entity.
	 * @param[out] slot A random free slot will be stored here.
	 * @return True if a free slot was found. False if not.
	 */
	bool GetRandomFreeSquadSlot(CBaseEntity* pEntity, int& slot);
	/**
	 * @brief Converts a string to a game type.
	 * @param[in] str String to convert.
	 * @return Converted game type.
	 */
	insmic::InsMICGameTypes_t StringToGameType(const char* str);
	/**
	 * @brief Returns the amount of reserve ammo left in a ballistic weapon.
	 * @param pEntity Entity pointer of the weapon.
	 * @return Amount of reserve ammo left.
	 */
	int GetBallisticWeaponReserveAmmoLeft(CBaseEntity* pEntity);
	/**
	 * @brief Checks if the given PLAYERFLAG_* flag is set. See incmic_shareddefs.h
	 * @param pPlayer Player to read the flags from.
	 * @param flags Flag bits to check.
	 * @return True if the given flags are set.
	 */
	bool IsPlayerFlagSet(CBaseEntity* pPlayer, const int flags);
	/**
	 * @brief Gets a player's current stance.
	 * @param pPlayer Player to read the stance from.
	 * @return Player's stance.
	 */
	insmic::Stance_t GetPlayerStance(CBaseEntity* pPlayer);
	/**
	 * @brief Gets the owner team of an objective.
	 * @param pEntity Objective entity pointer.
	 * @return Team that owns the given objective.
	 */
	insmic::InsMICTeam GetObjectiveOwnerTeam(CBaseEntity* pEntity);
}


#endif // !__NAVBOT_INSMIC_LIB_H_
