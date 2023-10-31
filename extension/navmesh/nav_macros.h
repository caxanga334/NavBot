#ifndef NAV_MESH_EXT_CUSTOM_MACROS
#define NAV_MESH_EXT_CUSTOM_MACROS
#pragma once

// Finds a value from an std::vector and removes it from the vector. The object is NOT deleted!
#define NAV_VEC_REMOVE_NO_DELETE(vector, value) \
		auto start = vector.begin(); \
									\
		auto end = vector.end(); \
									\
		auto pos = std::find(start, end, value); \
													\
		if (pos != end) \
		{ \
			\
			vector.erase(pos); \
			\
		} \

#endif // !NAV_MESH_EXT_CUSTOM_MACROS

