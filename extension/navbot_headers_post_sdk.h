#ifndef NAVBOT_HEADERS_POST_SDK_H_
#define NAVBOT_HEADERS_POST_SDK_H_

/* Pre compiled header file included AFTER the headers from the Source SDK */

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


#endif