#ifndef UTILS_H
#define UTILS_H

#include <assert.h>

#define force_inline __forceinline
#define StaticAssert(e) extern char (*ct_assert(void)) [sizeof(char[1 - 2*!(e)])]
#define ArrayCount(arr) (sizeof(arr) / sizeof((arr)[0]))

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

#endif
