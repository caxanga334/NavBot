#ifndef SMNAV_TFENTITIES_H_
#define SMNAV_TFENTITIES_H_
#pragma once

#include <entities/baseentity.h>
#include <mods/tf2/teamfortress2_shareddefs.h>

namespace tfentities
{

	class HCaptureFlag : public entities::HBaseEntity
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
		TeamFortress2::TFTeam GetTFTeam() const;
	};

	class HCaptureZone : public entities::HBaseEntity
	{
	public:
		HCaptureZone(edict_t* entity);
		HCaptureZone(CBaseEntity* entity);

		bool IsDisabled() const;
		TeamFortress2::TFTeam GetTFTeam() const;
	};
}

#endif // !SMNAV_TFENTITIES_CAPTUREFLAG_H_

