#include "Memory.h"

#include <format>
#include <Psapi.h>
#include <immintrin.h>

std::vector<byte> CMemory::PatternToByte(const char* szPattern)
{
	std::vector<byte> vPattern = {};

	const auto pStart = const_cast<char*>(szPattern);
	const auto pEnd = const_cast<char*>(szPattern) + strlen(szPattern);
	for (char* pCurrent = pStart; pCurrent < pEnd; ++pCurrent)
		vPattern.push_back(byte(std::strtoul(pCurrent, &pCurrent, 16)));

	return vPattern;
}

std::vector<int> CMemory::PatternToInt(const char* szPattern)
{
	std::vector<int> vPattern = {};

	const auto pStart = const_cast<char*>(szPattern);
	const auto pEnd = const_cast<char*>(szPattern) + strlen(szPattern);
	for (char* pCurrent = pStart; pCurrent < pEnd; ++pCurrent)
	{
		if (*pCurrent == '?') // Is current byte a wildcard? Simply ignore that that byte later
		{
			++pCurrent;
			if (*pCurrent == '?') // Check if following byte is also a wildcard
				++pCurrent;

			vPattern.push_back(-1);
		}
		else
			vPattern.push_back(std::strtoul(pCurrent, &pCurrent, 16));
	}

	return vPattern;
}

uintptr_t CMemory::FindSignature(const char* szModule, const char* szPattern)
{
	if (const auto hModule = GetModuleHandle(szModule))
	{
		// Get module information to search in the given module
		MODULEINFO lpModuleInfo;
		if (!GetModuleInformation(GetCurrentProcess(), hModule, &lpModuleInfo, sizeof(MODULEINFO)))
			return 0x0;

		// The region where we will search for the byte sequence
		const auto dwImageSize = lpModuleInfo.SizeOfImage;

		// Check if the image is faulty
		if (!dwImageSize)
			return 0x0;

		// Convert IDA-Style signature to a byte sequence
		const auto vPattern = PatternToInt(szPattern);
		const auto iPatternSize = vPattern.size();
		const int* iPatternBytes = vPattern.data();

		if (!iPatternSize)
			return 0x0;

		const auto pImageBytes = reinterpret_cast<byte*>(hModule);
		const size_t dwScanLimit = dwImageSize > iPatternSize ? dwImageSize - iPatternSize : 0;

		// Inline full-pattern check at offset i.
		auto checkAt = [&](size_t i) -> bool
		{
			for (size_t j = 0; j < iPatternSize; ++j)
			{
				if (pImageBytes[i + j] != iPatternBytes[j] && iPatternBytes[j] != -1)
					return false;
			}
			return true;
		};

		// SIMD first-byte prefilter: when first byte is concrete, only run the
		// full pattern check at offsets where that byte matches.  When the
		// first byte is a wildcard, fall back to the original byte-by-byte
		// scan (still O(N*M), but no candidates-vector allocation).
		if (iPatternBytes[0] != -1)
		{
			const __m128i needle = _mm_set1_epi8(char(iPatternBytes[0]));
			size_t i = 0;
			const size_t dwSimdEnd = dwScanLimit >= 16 ? dwScanLimit - 16 : 0;
			for (; i < dwSimdEnd; i += 16)
			{
				const __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pImageBytes + i));
				const int mask = _mm_movemask_epi8(_mm_cmpeq_epi8(chunk, needle));
				if (!mask)
					continue;
				for (int k = 0; k < 16; ++k)
				{
					if (!(mask & (1 << k)))
						continue;
					const size_t offset = i + k;
					if (offset >= dwScanLimit)
						break;
					if (checkAt(offset))
						return uintptr_t(&pImageBytes[offset]);
				}
			}
			for (; i < dwScanLimit; ++i)
			{
				if (pImageBytes[i] == iPatternBytes[0] && checkAt(i))
					return uintptr_t(&pImageBytes[i]);
			}
		}
		else
		{
			for (size_t i = 0; i < dwScanLimit; ++i)
			{
				if (checkAt(i))
					return uintptr_t(&pImageBytes[i]);
			}
		}

		return 0x0;
	}

	return 0x0;
}

using CreateInterfaceFn = void*(*)(const char* pName, int* pReturnCode);

PVOID CMemory::FindInterface(const char* szModule, const char* szObject)
{
	const auto CreateInterface = GetModuleExport<CreateInterfaceFn>(szModule, "CreateInterface");
	return CreateInterface(szObject, nullptr);
}

std::string CMemory::GetModuleOffset(uintptr_t uAddress)
{
	HMODULE hModule;
	if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, LPCSTR(uAddress), &hModule))
		return std::format("{:#x}", uAddress);

	uintptr_t uBase = uintptr_t(hModule);
	if (char buffer[MAX_PATH]; GetModuleBaseName(GetCurrentProcess(), hModule, buffer, sizeof(buffer) / sizeof(char)))
		return std::format("{}+{:#x}", buffer, uAddress - uBase);

	return std::format("{:#x}+{:#x}", uBase, uAddress - uBase);
}

uintptr_t CMemory::GetOffsetFromBase(uintptr_t uAddress)
{
	HMODULE hModule;
	if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, LPCSTR(uAddress), &hModule))
		return -1;

	uintptr_t uBase = uintptr_t(hModule);
	return uAddress - uBase;
}