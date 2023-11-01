#ifndef NAV_MESH_EXT_CUSTOM_MACROS
#define NAV_MESH_EXT_CUSTOM_MACROS
#pragma once

// Finds a value from an std::vector and removes it from the vector. The object is NOT deleted!
#define NAV_VEC_REMOVE_NO_DELETE(vector, value) \
		auto __start = vector.begin(); \
									\
		auto __end = vector.end(); \
									\
		auto __pos = std::find(__start, __end, value); \
													\
		if (__pos != __end) \
		{ \
			\
			vector.erase(__pos); \
			\
		} \

#endif // !NAV_MESH_EXT_CUSTOM_MACROS

