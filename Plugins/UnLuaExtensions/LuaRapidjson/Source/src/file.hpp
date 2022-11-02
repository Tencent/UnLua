
#ifndef __LUA_RAPIDJSION_FILE_HPP__
#define __LUA_RAPIDJSION_FILE_HPP__

#include <cstdio>

namespace file {
	inline FILE* open(const char* filename, const char* mode)
	{
#if WIN32
		FILE* fp = NULL;
		fopen_s(&fp, filename, mode);
		return fp;
#else
		return fopen(filename, mode);
#endif
	}
}

#endif

