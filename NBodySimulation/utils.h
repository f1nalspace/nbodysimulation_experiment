#ifndef UTILS_H
#define UTILS_H

#include <assert.h>
#include <stdio.h>
#include <algorithm>

#define force_inline __forceinline
#define StaticAssert(e) extern char (*ct_assert(void)) [sizeof(char[1 - 2*!(e)])]
#define ArrayCount(arr) (sizeof(arr) / sizeof((arr)[0]))

const float nanosToMilliseconds = 1.0f / 1000000;

template <typename T>
inline void UpdateMin(T &value, T a) {
	value = std::min(value, a);
}

template <typename T>
inline void UpdateMax(T &value, T a) {
	value = std::max(value, a);
}

template <typename T>
inline void Accumulate(T &value, T a) {
	value += a;
}

inline std::string GetProcessorName()
{
	int CPUInfo[4] = { -1 };
	char CPUBrandString[0x40];
	__cpuid(CPUInfo, 0x80000000);
	unsigned int nExIds = CPUInfo[0];

	memset(CPUBrandString, 0, sizeof(CPUBrandString));

	// Get the information associated with each extended ID.
	for (unsigned int i = 0x80000000; i <= nExIds; ++i)
	{
		__cpuid(CPUInfo, i);
		// Interpret CPU brand string.
		if (i == 0x80000002)
			memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
		else if (i == 0x80000003)
			memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
		else if (i == 0x80000004)
			memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
	}
	return(std::string(CPUBrandString));
}

inline std::string StringFormat(const std::string &format, const size_t bufferCount, ...) {
	std::string strBuf;
	strBuf.resize(bufferCount);
	va_list vaList;
	va_start(vaList, bufferCount);
	int characterCount = vsprintf_s((char *)strBuf.c_str(), bufferCount, format.c_str(), vaList);
	va_end(vaList);
	strBuf.resize(characterCount);
	return(strBuf);
}

#endif
