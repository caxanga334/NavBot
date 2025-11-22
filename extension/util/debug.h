#ifndef __NAVBOT_UTIL_DEBUG_H_
#define __NAVBOT_UTIL_DEBUG_H_

#if defined(COMPILER_MSVC)
	#define EXT_DBGBREAK()			__debugbreak();
#elif defined(COMPILER_GCC)		
	#define EXT_DBGBREAK()			asm( "int3" )
#endif // defined(COMPILER_MSVC)


#ifdef EXT_DEBUG
// Debug enabled

#define EXT_ASSERT(_exp, _msg)																		\
	if (!(_exp))																					\
	{																								\
		smutils->LogMessage(myself, "Assertion failed <%s:%d>: %s", __FILE__, __LINE__, _msg);		\
		EXT_DBGBREAK();																				\
	}																								\
																									\

#else
// Debug disabled

#define EXT_ASSERT(_exp, _msg)			((void)0)

#endif // EXT_DEBUG
#endif // __NAVBOT_UTIL_DEBUG_H_