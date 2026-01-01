#ifndef NAVBOT_HEADERS_POST_SDK_H_
#define NAVBOT_HEADERS_POST_SDK_H_

/* Pre compiled header file included AFTER the headers from the Source SDK */

#if SOURCE_ENGINE <= SE_DARKMESSIAH
#include <studio.h>
#endif // SOURCE_ENGINE <= SE_DARKMESSIAH

// Additional SDK stuff
class CTakeDamageInfo;

// forward declaration for some navbot stuff
class CBaseBot;

// Fix Windows shit
#ifdef GetClassName
 #undef GetClassName
#endif

#ifdef GetProp
 #undef GetProp
#endif

// Undef some stuff defined by the SDK's mathlib that conflicts with STL

#ifdef min
 #undef min
#endif // min

#ifdef max
 #undef max
#endif // max

#ifdef clamp
 #undef clamp
#endif // clamp

// EP1 SDK compat stuff
#if SOURCE_ENGINE <= SE_DARKMESSIAH

// fsel is missing from ep1

template <typename T>
inline T fsel(T comparand, T val, T lt)
{
	return comparand >= static_cast<T>(0) ? val : lt;
}

#define MASK_BLOCKLOS MASK_VISIBLE
#define MASK_BLOCKLOS_AND_NPCS MASK_VISIBLE

#define CON_COMMAND_F_COMPLETION( name, description, flags, completion ) \
	static void name( void ); \
	static ConCommand name##_command( #name, name, description, flags, completion ); \
	static void name( void )

#endif // SOURCE_ENGINE <= SE_DARKMESSIAH

#if SOURCE_ENGINE >= SE_ORANGEBOX

#define CONVAR_REGISTER(object) ConVar_Register(0, object)

#else

#define CONVAR_REGISTER(object) ConCommandBaseMgr::OneTimeInit(object)
#define CVAR_INTERFACE_VERSION VENGINE_CVAR_INTERFACE_VERSION

#endif // SOURCE_ENGINE >= SE_ORANGEBOX

#if SOURCE_ENGINE <= SE_DARKMESSIAH
// shareddefs stuff missing in old engine

//-----------------------------------------------------------------------------
// Multiplayer specific defines
//-----------------------------------------------------------------------------
#define MAX_CONTROL_POINTS			8
#define MAX_CONTROL_POINT_GROUPS	8

// Maximum number of points that a control point may need owned to be cappable
#define MAX_PREVIOUS_POINTS			3

// The maximum number of teams the control point system knows how to deal with
#define MAX_CONTROL_POINT_TEAMS		8

// Maximum length of the cap layout string
#define MAX_CAPLAYOUT_LENGTH		32

// Maximum length of the current round printname
#define MAX_ROUND_NAME				32

// Maximum length of the current round name
#define MAX_ROUND_IMAGE_NAME		64

// Score added to the team score for a round win
#define TEAMPLAY_ROUND_WIN_SCORE	1

#endif // SOURCE_ENGINE <= SE_DARKMESSIAH

// Stuff that we need but doesn't include the header file from the SDK directly due to reasons
#include <sdkports/sdk_defs.h>

#endif