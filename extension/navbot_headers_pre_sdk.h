#ifndef NAVBOT_HEADERS_PRE_SDK_H_
#define NAVBOT_HEADERS_PRE_SDK_H_

/* Pre compiled header file included BEFORE the headers from the Source SDK */

// Fix Windows shit
#ifdef GetClassName
 #undef GetClassName
#endif

#ifdef GetProp
 #undef GetProp
#endif

#ifdef COMPILER_MSVC
// Silence certain warnings
#pragma warning(disable : 4244)		// int or float down-conversion
#pragma warning(disable : 4305)		// int or float data truncation
#pragma warning(disable : 4201)		// nameless struct/union
#pragma warning(disable : 4511)     // copy constructor could not be generated
#pragma warning(disable : 4675)     // resolved overload was found by argument dependent lookup
#endif

// saverestore.h declarations
class ISave;
class IRestore;

// maximum number of targets a single multi_manager entity may be assigned.
// #define MAX_MULTI_TARGETS	16 

class CBaseEdict;
struct edict_t;

// NPCEvent.h declarations
struct animevent_t;

struct studiohdr_t;
class CStudioHdr;

class CBaseEntity;
class CBasePlayer;

#endif