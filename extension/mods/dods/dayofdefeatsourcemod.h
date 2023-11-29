#ifndef SMNAV_DODS_MOD_H_
#define SMNAV_DODS_MOD_H_
#pragma once

#include <mods/basemod.h>

class CDayOfDefeatSourceMod : public CBaseMod
{
public:
	CDayOfDefeatSourceMod() : CBaseMod() {}
	virtual ~CDayOfDefeatSourceMod() {}

	virtual bool UserCommandNeedsHook() override { return true; }
};

#endif // !SMNAV_DODS_MOD_H_
