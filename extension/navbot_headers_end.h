#ifndef NAVBOT_HEADERS_END_H_
#define NAVBOT_HEADERS_END_H_

/* The very last file in the pre compiled headers. AFTER everything, include some of NavBot own headers */

// Fix Windows shit
#ifdef GetClassName
 #undef GetClassName
#endif

#ifdef GetProp
 #undef GetProp
#endif

#endif