#ifndef SMNAV_DODS_MOD_H_
#define SMNAV_DODS_MOD_H_
#pragma once

#include <mods/basemod.h>

class CDayOfDefeatSourceMod : public CBaseMod
{
public:
	CDayOfDefeatSourceMod() : CBaseMod() {}
	~CDayOfDefeatSourceMod() override {}

	const char* GetModName() override { return "Day of Defeat: Source"; }
	Mods::ModType GetModType() override { return Mods::ModType::MOD_DODS; }
};

#endif // !SMNAV_DODS_MOD_H_
