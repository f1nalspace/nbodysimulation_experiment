#ifndef UTILS_H
#define UTILS_H

#include <final_platform_layer.hpp>
#include <assert.h>
#include <stdio.h>
#include <algorithm>
#include <intrin.h>
#include <varargs.h>

#define force_inline __forceinline

template <typename T>
inline T PointerToValue(void *ptr) {
	T result = (T)(uintptr_t)(ptr);
	return(result);
}
template <typename T>
inline void *ValueToPointer(T value) {
	void *result = (T *)(uintptr_t)(value);
	return(result);
}

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

static uint8_t *LoadFileContent(const char *filename) {
	uint8_t *result = nullptr;
	fpl::files::FileHandle handle = fpl::files::OpenBinaryFile(filename);
	if (handle.isValid) {
		fpl::files::SetFilePosition32(handle, 0, fpl::files::FilePositionMode::End);
		uint32_t fileSize = fpl::files::GetFilePosition32(handle);
		fpl::files::SetFilePosition32(handle, 0, fpl::files::FilePositionMode::Beginning);
		result = (uint8_t *)fpl::memory::MemoryAllocate(fileSize);
		fpl::files::ReadFileBlock32(handle, fileSize, result, fileSize);
		fpl::files::CloseFile(handle);
	}
	return(result);
}

#endif
