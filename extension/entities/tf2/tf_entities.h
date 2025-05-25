#ifndef SMNAV_TFENTITIES_H_
#define SMNAV_TFENTITIES_H_
#pragma once

#include <entities/baseentity.h>
#include <mods/tf2/teamfortress2_shareddefs.h>

namespace tfentities
{
	class HTFBaseEntity : public entities::HBaseEntity
	{
	public:
		HTFBaseEntity(edict_t* entity);
		HTFBaseEntity(CBaseEntity* entity);

		TeamFortress2::TFTeam GetTFTeam() const;
		// checks if this entity can be used by the given team, assumes unassigned is accessible to all teams.
		inline bool CanBeUsedByTeam(TeamFortress2::TFTeam otherteam) const
		{
			if (GetTFTeam() == TeamFortress2::TFTeam_Unassigned)
				return true;

			return GetTFTeam() == otherteam;
		}
	};

	class HCaptureFlag : public HTFBaseEntity
	{
	public:
		HCaptureFlag(edict_t* entity);
		HCaptureFlag(CBaseEntity* entity);

		bool IsDisabled() const;
		TeamFortress2::TFFlagType GetType() const;
		int GetStatus() const;
		bool IsHome() const { return GetStatus() == TeamFortress2::TF_FLAGINFO_HOME; }
		bool IsStolen() const { return GetStatus() == TeamFortress2::TF_FLAGINFO_STOLEN; }
		bool IsDropped() const { return GetStatus() == TeamFortress2::TF_FLAGINFO_DROPPED; }
		Vector GetPosition() const;
		Vector GetReturnPosition() const;
		// How many points this capture flag is worth
		int GetPointValue() const;
	};

	class HCaptureZone : public HTFBaseEntity
	{
	public:
		HCaptureZone(edict_t* entity);
		HCaptureZone(CBaseEntity* entity);

		bool IsDisabled() const;
	};

	class HFuncRegenerate : public HTFBaseEntity
	{
	public:
		HFuncRegenerate(edict_t* entity);
		HFuncRegenerate(CBaseEntity* entity);

		bool IsDisabled() const;
	};

	class HBaseObject : public HTFBaseEntity
	{
	public:
		HBaseObject(edict_t* entity);
		HBaseObject(CBaseEntity* entity);

		int GetHealth() const;
		int GetMaxHealth() const;
		inline float GetHealthPercentage() const { return static_cast<float>(GetHealth()) / static_cast<float>(GetMaxHealth()); }
		bool IsDisabled() const;
		float GetPercentageConstructed() const;
		TeamFortress2::TFObjectType GetType() const;
		TeamFortress2::TFObjectMode GetMode() const;
		edict_t* GetBuilder() const;
		int GetBuilderIndex() const;
		int GetLevel() const;
		int GetMaxLevel() const;
		inline bool IsAtMaxLevel() const { return GetLevel() == GetMaxLevel(); }
		bool IsPlacing() const;
		bool IsBuilding() const;
		bool IsBeingCarried() const;
		bool IsSapped() const;
		bool IsRedeploying() const;
		bool IsMiniBuilding() const;
		bool IsDisposableBuilding() const;
		int GetMaxUpgradeMetal() const; // amount required to upgrade to next level (generally 200)
		int GetUpgradeMetal() const; // amount of metal added to the current upgrade level
		// Amount of metal needed to upgrade to the next level. 0 if at max level
		inline int GetMetalNeededToUpgrade() const
		{
			if (IsAtMaxLevel()) { return 0; }
			return GetMaxUpgradeMetal() - GetUpgradeMetal();
		}
		inline float GetUpgradeProgress() const { return static_cast<float>(GetUpgradeMetal()) / static_cast<float>(GetMaxUpgradeMetal()); }
	};

	class HObjectTeleporter : public HBaseObject
	{
	public:
		HObjectTeleporter(edict_t* entity);
		HObjectTeleporter(CBaseEntity* entity);

		TeamFortress2::TeleporterState GetState() const;
	};

	class HObjectDispenser : public HBaseObject
	{
	public:
		HObjectDispenser(edict_t* entity) : HBaseObject(entity) {}
		HObjectDispenser(CBaseEntity* entity) : HBaseObject(entity) {}

		TeamFortress2::DispenserState GetState() const;
		int GetStoredMetal() const;
		// Default maximum stored metal
		static constexpr int GetDefaultMaxStoredMetal() { return 400; }
	};

	class HTeamControlPoint : public HTFBaseEntity
	{
	public:
		HTeamControlPoint(edict_t* entity) : HTFBaseEntity(entity) {}
		HTeamControlPoint(CBaseEntity* entity) : HTFBaseEntity(entity) {}

		int GetGroup() const;
		// Control Point index
		int GetPointIndex() const;
		// Same as GetPointIndex, for RCBot2 compat
		int GetArea() const { return GetPointIndex(); }
		TeamFortress2::TFTeam GetDefaultOwner() const;
		bool IsLocked() const;
		std::string GetPrintName() const;
	};

	class HTeamRoundTimer : public HTFBaseEntity
	{
	public:
		HTeamRoundTimer(edict_t* entity) : HTFBaseEntity(entity) {}
		HTeamRoundTimer(CBaseEntity* entity) : HTFBaseEntity(entity) {}

		bool IsTimerPaused() const;
		bool IsTimerDisabled() const;
		bool IsDisabled() const { return IsTimerDisabled(); }
		TeamFortress2::RoundTimerStates GetState() const;
		bool IsWatchingTimeStamps() const;
		bool IsStopWatchTimer() const;
		float GetStopWatchTotalTime() const;
		bool IsRoundMaxTimerSet() const;
		int GetTimerInitialLength() const;
		int GetSetupTimeLength() const;
		// Purpose: Gets the seconds left on the timer, paused or not.
		float GetTimeRemaining() const;


	};
}

#endif // !SMNAV_TFENTITIES_CAPTUREFLAG_H_

