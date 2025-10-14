#ifndef NAVBOT_HEADERS_POST_SDK_H_
#define NAVBOT_HEADERS_POST_SDK_H_

/* Pre compiled header file included AFTER the headers from the Source SDK */

#if SOURCE_ENGINE == SE_EPISODEONE
#include <studio.h>
#endif

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
#if SOURCE_ENGINE == SE_EPISODEONE

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

#endif // SOURCE_ENGINE == SE_EPISODEONE

#if SOURCE_ENGINE >= SE_ORANGEBOX

#define CONVAR_REGISTER(object) ConVar_Register(0, object)

#else

#define CONVAR_REGISTER(object) ConCommandBaseMgr::OneTimeInit(object)
#define CVAR_INTERFACE_VERSION VENGINE_CVAR_INTERFACE_VERSION

#endif // SOURCE_ENGINE >= SE_ORANGEBOX

#endif